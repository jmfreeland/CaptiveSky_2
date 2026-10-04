"""Bounded CSV CPU/render/GPU profile of the existing editor-only viewpoint test.

Launch UnrealEditor-Cmd with -ExecutePythonScript=<this path> -RenderOffscreen
-csvGpuStats -csvCompression=0 and normal -Viewpoint* flags. Do NOT specify
-TestExit: this controller stops and flushes the profiler before quitting the editor.
Use -ProfileRepeats=2 for a same-process cold/warmed comparison (bounded to 1..3).
Use -ProfileDisableDistanceFields only to verify a startup diagnostic ablation,
not acceptance. r.DistanceFields is read-only at runtime; supply a startup override.
The existing test still owns all safety/performance gates and pass/fail results.
No map saves, gameplay, resident turns or persistent settings changes.
"""

import time
import re
import traceback
import unreal


TIME_LIMIT_SECONDS = 180.0
command_line = unreal.SystemLibrary.get_command_line()
disable_distance_fields = bool(re.search(r"(?:^|\s)-ProfileDisableDistanceFields(?=\s|$)", command_line))
match = re.search(r"(?:^|\s)-ProfileRepeats=(\d+)(?=\s|$)", command_line)
REPEATS = int(match.group(1)) if match else 1
if not 1 <= REPEATS <= 3:
    raise ValueError("ProfileRepeats must be between 1 and 3")
state = {"started": time.monotonic(), "last_tick": time.monotonic(), "seen_running": False,
         "stopped": None, "handle": None, "run": 1}


def console(command):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, command)


def finish(reason):
    console("csvprofile stop")
    state["stopped"] = time.monotonic()
    unreal.log("[RenderProfile] STOP frame={} ".format(unreal.SystemLibrary.get_frame_count()) + reason)


def on_tick(delta_seconds):
    try:
        now = time.monotonic()
        wall_gap = now - state["last_tick"]
        state["last_tick"] = now
        if state["stopped"] is not None:
            if now - state["stopped"] >= 2.0:
                unreal.unregister_slate_post_tick_callback(state["handle"])
                unreal.log("[RenderProfile] COMPLETE: inspect automation result and Saved/Profiling/CSV separately")
                unreal.EditorPythonScripting.set_keep_python_script_alive(False)
                unreal.SystemLibrary.quit_editor()
            return
        running = unreal.AutomationLibrary.are_automated_tests_running()
        state["seen_running"] |= running
        if wall_gap >= 0.1:
            unreal.log("[RenderProfile] STALL frame={} elapsed={:.3f}s wall_gap={:.3f}s engine_delta={:.3f}s tests_running={}".format(
                unreal.SystemLibrary.get_frame_count(), now - state["started"], wall_gap, delta_seconds, running))
        if now - state["started"] > TIME_LIMIT_SECONDS:
            unreal.log_error("[RenderProfile] TIMEOUT: profile incomplete; not a passing test")
            finish("180 second editor-only watchdog")
        elif state["seen_running"] and not running:
            if state["run"] < REPEATS:
                state["run"] += 1
                state["seen_running"] = False
                unreal.log("[RenderProfile] REPEAT run={} frame={}; retain previous test result".format(
                    state["run"], unreal.SystemLibrary.get_frame_count()))
                console("Automation RunTests CaptiveSky2.Visual.Viewpoints")
            else:
                finish("automation became terminal; this does not imply success")
    except Exception:
        unreal.log_error("[RenderProfile] FAILED " + traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(state["handle"])
        try:
            console("csvprofile stop")
        finally:
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.SystemLibrary.quit_editor()


try:
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    if disable_distance_fields:
        actual = unreal.SystemLibrary.get_console_variable_int_value("r.DistanceFields")
        if actual != 0:
            raise RuntimeError("Distance-field ablation refused: startup r.DistanceFields is {}, expected 0".format(actual))
        unreal.log_warning("[RenderProfile] ABLATION VERIFIED: startup r.DistanceFields=0; not a quality acceptance run")
    console("r.DistanceFields")
    console("csvprofile start")
    state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
    console("Automation RunTests CaptiveSky2.Visual.Viewpoints")
    unreal.log("[RenderProfile] START frame={}: existing viewpoint automation, 180 second tick watchdog".format(
        unreal.SystemLibrary.get_frame_count()))
except Exception:
    unreal.log_error("[RenderProfile] FAILED " + traceback.format_exc())
    if state["handle"] is not None:
        unreal.unregister_slate_post_tick_callback(state["handle"])
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()
    raise
