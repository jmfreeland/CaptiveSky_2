"""Orchestrator (venv python):  python -I make.py <asset_script> [<asset_script> ...]

build+bake+export (Blender) -> pack stone textures (PIL) -> verify by re-import + render (Blender)
Logs: <export root>/_work/logs/<asset>_{build,verify}.txt
"""
import os
import re
import subprocess
import sys

PIPE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, PIPE)
from config import BLENDER, WORK  # noqa: E402

PY = sys.executable  # the venv python you launched make.py with (needs Pillow + numpy)
LOGS = f"{WORK}/logs"
os.makedirs(LOGS, exist_ok=True)


def run(cmd, log):
    with open(log, "w") as f:
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)
    return r.returncode, open(log, errors="replace").read()


for name in sys.argv[1:]:
    src = open(f"{PIPE}/assets/{name}.py").read()
    folder = re.search(r'NAME\s*=\s*"(\w+)"', src).group(1)
    rc, out = run([BLENDER, "-b", "--factory-startup", "-P", f"{PIPE}/run_asset.py", "--", name], f"{LOGS}/{name}_build.txt")
    ok = "EXPORT DONE" in out
    print(f"[{name}] build rc={rc} exported={ok}")
    for line in out.splitlines():
        if re.search(r"EXPORT DONE|zmin|Traceback|Error|Assertion|baked", line) and "HIPEW" not in line:
            print("   ", line.strip())
    if not ok:
        continue
    r = subprocess.run([PY, "-I", f"{PIPE}/pack_asset.py", folder], capture_output=True, text=True)
    print("   ", r.stdout.strip() or r.stderr.strip())
    rc, out = run([BLENDER, "-b", "--factory-startup", "-P", f"{PIPE}/verify_asset.py", "--", folder], f"{LOGS}/{name}_verify.txt")
    for line in out.splitlines():
        if re.search(r"IMPORTED|UV LAYERS|ZMIN|PROBLEMS|VERIFY DONE|Traceback|Error", line) and "HIPEW" not in line:
            print("   ", line.strip())
