"""Dev-time Tripo text-to-3D tool: budgeted generation that downloads a GLB for review.

  python Scripts/Tripo.py balance
  python Scripts/Tripo.py generate "weathered driftwood bench, mossy, game prop" --name DriftwoodBench
  python Scripts/Tripo.py usage

The API key is read from the TRIPO_API_KEY environment variable (override the name with --key-env). On Windows a
process started before the variable was set falls back to the user-level value in the registry. The key is never
printed, logged or written anywhere.

Every generation is checked against a daily ledger (Saved/CaptiveSky/TripoBudget.json) and the account balance
before it is submitted. Defaults: 10 tasks and 300 credits per day; override with --max-tasks/--max-credits or the
TRIPO_DAILY_TASKS / TRIPO_DAILY_CREDITS variables. Results land in Saved/CaptiveSky/Tripo/<name>/ (model.glb,
preview.png, meta.json) for a human to review before anything is imported into Content/.

Tripo v3 API: https://developers.tripo3d.ai/en/docs/quick-start. Model URLs expire after about 5 minutes, so the
download happens as soon as the task succeeds.
"""

import argparse
import datetime
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request

BASE_URL = "https://openapi.tripo3d.ai/v3"
DEFAULT_KEY_ENV = "TRIPO_API_KEY"
DEFAULT_MODEL = "v3.1-20260211"
DEFAULT_MAX_TASKS = 10
DEFAULT_MAX_CREDITS = 300.0
POLL_SECONDS = 3.0
TIMEOUT_SECONDS = 600.0
TERMINAL_FAILURES = ("failed", "cancelled", "banned")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LEDGER = os.path.join(ROOT, "Saved", "CaptiveSky", "TripoBudget.json")
OUT_DIR = os.path.join(ROOT, "Saved", "CaptiveSky", "Tripo")


class TripoError(Exception):
    pass


def read_key(env_name):
    key = os.environ.get(env_name, "").strip()
    if not key and sys.platform == "win32":
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as hive:
                key = str(winreg.QueryValueEx(hive, env_name)[0]).strip()
        except OSError:
            key = ""
    if not key:
        raise TripoError("{} is not set. Set it as a user environment variable (never in the repo).".format(env_name))
    return key


def request(key, method, path, body=None):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    req = urllib.request.Request(BASE_URL + path, data=data, method=method)
    req.add_header("Authorization", "Bearer " + key)
    req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=60) as response:
            payload = json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as error:
        detail = error.read().decode("utf-8", "replace")[:500]
        raise TripoError("HTTP {} from {} {}: {}".format(error.code, method, path, detail))
    except urllib.error.URLError as error:
        raise TripoError("Could not reach Tripo: {}".format(error.reason))
    if payload.get("code", 0) != 0:
        raise TripoError("Tripo error on {} {}: {}".format(method, path, json.dumps(payload)[:500]))
    return payload.get("data") or {}


def download(url, destination):
    # Output URLs are pre-signed; do not send the API key to them.
    with urllib.request.urlopen(url, timeout=120) as response, open(destination, "wb") as handle:
        handle.write(response.read())


def today():
    return datetime.date.today().isoformat()


def load_ledger():
    try:
        with open(LEDGER, "r", encoding="utf-8") as handle:
            ledger = json.load(handle)
    except (OSError, ValueError):
        ledger = {}
    if ledger.get("date") != today():
        ledger = {"date": today(), "tasks": 0, "credits": 0.0, "history": []}
    return ledger


def save_ledger(ledger):
    os.makedirs(os.path.dirname(LEDGER), exist_ok=True)
    with open(LEDGER, "w", encoding="utf-8") as handle:
        json.dump(ledger, handle, indent=2)


def limit(arg_value, env_name, default, cast):
    if arg_value is not None:
        return cast(arg_value)
    raw = os.environ.get(env_name)
    return cast(raw) if raw else default


def safe_name(name):
    cleaned = re.sub(r"[^A-Za-z0-9_]+", "_", name).strip("_")
    if not cleaned:
        raise TripoError("--name needs at least one letter or digit")
    return cleaned[:48]


def cmd_balance(args, key):
    data = request(key, "GET", "/account/balance")
    print("balance {:.2f}  frozen {:.2f}".format(float(data.get("balance", 0)), float(data.get("frozen", 0))))


def cmd_usage(args, key):
    ledger = load_ledger()
    print("{}: {} tasks, {:.2f} credits (ledger {})".format(ledger["date"], ledger["tasks"], ledger["credits"], LEDGER))
    for entry in ledger["history"]:
        print("  {}  {}  {:.2f} credits  {}".format(entry["time"], entry["task_id"], entry["credits"], entry["name"]))


def cmd_generate(args, key):
    prompt = args.prompt.strip()
    if not prompt or len(prompt) > 1024:
        raise TripoError("Prompt must be 1-1024 characters (got {})".format(len(prompt)))
    name = safe_name(args.name or prompt[:32])
    max_tasks = limit(args.max_tasks, "TRIPO_DAILY_TASKS", DEFAULT_MAX_TASKS, int)
    max_credits = limit(args.max_credits, "TRIPO_DAILY_CREDITS", DEFAULT_MAX_CREDITS, float)

    ledger = load_ledger()
    if ledger["tasks"] >= max_tasks:
        raise TripoError("Daily task cap reached ({}/{}). Raise --max-tasks to continue.".format(ledger["tasks"], max_tasks))
    if ledger["credits"] >= max_credits:
        raise TripoError("Daily credit cap reached ({:.2f}/{:.2f}). Raise --max-credits to continue.".format(ledger["credits"], max_credits))

    balance = float(request(key, "GET", "/account/balance").get("balance", 0))
    if balance <= 0:
        raise TripoError("Tripo balance is {:.2f} credits; top up at platform.tripo3d.ai before generating.".format(balance))

    body = {"prompt": prompt, "model": args.model}
    if args.negative_prompt:
        body["negative_prompt"] = args.negative_prompt
    if args.face_limit:
        body["face_limit"] = args.face_limit
    if args.no_texture:
        body["texture"] = False
        body["pbr"] = False
    if args.dry_run:
        print("dry run: would submit {} (balance {:.2f}, today {} tasks / {:.2f} credits)".format(json.dumps(body), balance, ledger["tasks"], ledger["credits"]))
        return

    task_id = request(key, "POST", "/generation/text-to-model", body)["task_id"]
    print("submitted {} ({})".format(task_id, name))
    # Count the task before polling so a crash mid-run cannot hide a spend.
    entry = {"time": datetime.datetime.now().isoformat(timespec="seconds"), "task_id": task_id, "name": name, "credits": 0.0}
    ledger["tasks"] += 1
    ledger["history"].append(entry)
    save_ledger(ledger)

    deadline = time.time() + TIMEOUT_SECONDS
    while True:
        task = request(key, "GET", "/tasks/" + task_id)
        status = task.get("status")
        if status == "success":
            break
        if status in TERMINAL_FAILURES:
            raise TripoError("Task {} ended as {} (failed tasks are not charged).".format(task_id, status))
        if time.time() > deadline:
            raise TripoError("Task {} still {} after {:.0f}s; check it later with the Tripo dashboard.".format(task_id, status, TIMEOUT_SECONDS))
        print("  {} {}%".format(status, task.get("progress", "?")))
        time.sleep(POLL_SECONDS)

    credits = float(task.get("credits_consumed") or 0)
    entry["credits"] = credits
    ledger["credits"] += credits
    save_ledger(ledger)

    folder = os.path.join(OUT_DIR, name)
    os.makedirs(folder, exist_ok=True)
    output = task.get("output") or {}
    if not output.get("model_url"):
        raise TripoError("Task {} succeeded but returned no model_url: {}".format(task_id, json.dumps(output)[:300]))
    download(output["model_url"], os.path.join(folder, "model.glb"))
    if output.get("rendered_image_url"):
        download(output["rendered_image_url"], os.path.join(folder, "preview.png"))
    with open(os.path.join(folder, "meta.json"), "w", encoding="utf-8") as handle:
        json.dump({"prompt": prompt, "model": args.model, "task_id": task_id, "credits": credits, "created": entry["time"], "request": body}, handle, indent=2)
    print("saved {} ({:.2f} credits; today {} tasks / {:.2f} credits)".format(folder, credits, ledger["tasks"], ledger["credits"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--key-env", default=DEFAULT_KEY_ENV, help="name of the environment variable holding the API key")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("balance", help="show the account credit balance")
    sub.add_parser("usage", help="show today's ledger")
    gen = sub.add_parser("generate", help="text-to-model; downloads model.glb + preview.png")
    gen.add_argument("prompt")
    gen.add_argument("--name", help="folder name under Saved/CaptiveSky/Tripo (default: from the prompt)")
    gen.add_argument("--model", default=DEFAULT_MODEL)
    gen.add_argument("--negative-prompt")
    gen.add_argument("--face-limit", type=int, help="cap on triangles; lower is lighter for the game")
    gen.add_argument("--no-texture", action="store_true", help="geometry only (cheaper)")
    gen.add_argument("--max-tasks", help="daily task cap (default {})".format(DEFAULT_MAX_TASKS))
    gen.add_argument("--max-credits", help="daily credit cap (default {})".format(int(DEFAULT_MAX_CREDITS)))
    gen.add_argument("--dry-run", action="store_true", help="run every check, print the request, submit nothing")
    args = parser.parse_args()

    try:
        if args.command == "usage":
            cmd_usage(args, None)
            return 0
        key = read_key(args.key_env)
        {"balance": cmd_balance, "generate": cmd_generate}[args.command](args, key)
    except TripoError as error:
        print("error: " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
