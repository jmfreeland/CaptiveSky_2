#!/usr/bin/env python3
"""Build a read-only, single-file atlas from explicitly supplied Island snapshots.

The atlas is a lens on saved evidence. It never opens Unreal, reads live actors, runs a model,
or writes WorldState. Source files should be copied while the game is stopped.

    python Scripts/Build-WorldAtlas.py --world-state Saved/AtlasInput/Island.json \
        --trails Saved/AtlasInput/Island.trails.json --chronicle Saved/AtlasInput/chronicle.jsonl \
        --out Saved/WorldAtlas/island.html
"""
from __future__ import annotations

import argparse
import html
import json
import math
import sys
from pathlib import Path
from typing import Any

MAX_COORD_CM = 5_000_000.0
MAX_WORLD_STATE_BYTES = 16 * 1024 * 1024
MAX_TRAIL_BYTES = 8 * 1024 * 1024
MAX_CHRONICLE_BYTES = 32 * 1024 * 1024
MAX_TRAIL_CELLS = 6_000
MAX_CHRONICLE_EVENTS = 50_000
MAX_MAP_EVENTS = 1_200
MAX_TIMELINE_EVENTS = 250
TRAIL_CELL_SIZE_CM = 150.0
TRAIL_REGION_SPAN = 8
KNOWN_EVENT_TYPES = {
    "arrangement", "arrangement_response", "basin", "decision", "day", "forage", "guest_book",
    "lightning", "nest", "rainbow", "session", "sky", "social_commitment", "storm_mark", "trail",
    "weather", "wrack_forage", "wrack_turned", "curio", "sea_state", "wildlife_home",
}


class AtlasInputError(ValueError):
    """Source data is unreadable, unsupported, or exceeds an explicit bound."""


def _read_json(path: Path, maximum_bytes: int) -> dict[str, Any]:
    try:
        if path.stat().st_size > maximum_bytes:
            raise AtlasInputError(f"{path.name} exceeds the {maximum_bytes:,}-byte input limit")
        value = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise AtlasInputError(f"cannot read {path}: {exc}") from exc
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise AtlasInputError(f"{path.name} is not valid UTF-8 JSON") from exc
    if not isinstance(value, dict):
        raise AtlasInputError(f"{path.name} must contain a JSON object")
    return value


def _finite_number(value: Any) -> float | None:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None
    number = float(value)
    return number if math.isfinite(number) else None


def _bounded_xy(x: Any, y: Any) -> tuple[float, float] | None:
    fx, fy = _finite_number(x), _finite_number(y)
    if fx is None or fy is None or abs(fx) > MAX_COORD_CM or abs(fy) > MAX_COORD_CM:
        return None
    return fx, fy


def _bounded_int(value: Any, minimum: int, maximum: int) -> int | None:
    number = _finite_number(value)
    if number is None or not number.is_integer() or number < minimum or number > maximum:
        return None
    return int(number)


def _chronicle_int(value: Any, minimum: int, maximum: int) -> int | None:
    # Chronicle extras are written as strings by UIslandChronicleSubsystem.
    if isinstance(value, str) and len(value) <= 12:
        try:
            value = int(value.strip())
        except ValueError:
            return None
    return _bounded_int(value, minimum, maximum)


def _point_from_array(value: Any) -> tuple[float, float] | None:
    if not isinstance(value, list) or len(value) < 2:
        return None
    return _bounded_xy(value[0], value[1])


def _saved_points(world: dict[str, Any]) -> tuple[list[dict[str, Any]], int]:
    points: list[dict[str, Any]] = []
    skipped = 0
    categories = (
        ("curios", "id", "location", "curio"),
        ("nests", "site", "location", "nest"),
        ("arrangement_sites", "id", "location", "arrangement site"),
        ("wildlife_homes", "id", "entrance_location", "wildlife home"),
    )
    for collection_name, identity_key, location_key, label in categories:
        collection = world.get(collection_name, [])
        if not isinstance(collection, list):
            raise AtlasInputError(f"world-state field {collection_name!r} must be an array")
        if len(collection) > MAX_TRAIL_CELLS:
            raise AtlasInputError(f"world-state field {collection_name!r} exceeds the record limit")
        for record in collection:
            if not isinstance(record, dict):
                skipped += 1
                continue
            xy = _point_from_array(record.get(location_key))
            identity = record.get(identity_key)
            if xy is None or not isinstance(identity, str) or not identity.strip() or len(identity) > 100:
                skipped += 1
                continue
            points.append({"x": xy[0], "y": xy[1], "id": identity.strip(), "kind": label})
    return points, skipped


def load_world_state(path: Path) -> dict[str, Any]:
    world = _read_json(path, MAX_WORLD_STATE_BYTES)
    version = world.get("version", 1)
    if _bounded_int(version, 1, 3) is None:
        raise AtlasInputError("world-state version must be 1, 2, or 3")
    points, skipped = _saved_points(world)
    return {"version": int(version), "points": points, "skipped_points": skipped}


def load_trails(path: Path | None) -> dict[str, Any]:
    if path is None:
        return {"version": None, "cells": [], "skipped_cells": 0}
    root = _read_json(path, MAX_TRAIL_BYTES)
    version = _bounded_int(root.get("version"), 1, 2)
    if version is None:
        raise AtlasInputError("trail ledger version must be 1 or 2")
    rows = root.get("cells")
    if not isinstance(rows, list) or len(rows) > MAX_TRAIL_CELLS:
        raise AtlasInputError(f"trail ledger must have an array of at most {MAX_TRAIL_CELLS:,} cells")
    expected_length = 8 if version == 1 else 9
    cells: list[dict[str, Any]] = []
    keys: set[tuple[int, int]] = set()
    for index, row in enumerate(rows):
        if not isinstance(row, list) or len(row) != expected_length:
            raise AtlasInputError(f"trail cell {index} does not match version {version} row layout")
        cell_x = _bounded_int(row[0], -100_000, 100_000)
        cell_y = _bounded_int(row[1], -100_000, 100_000)
        steps = _bounded_int(row[2], 0, 2_147_483_647)
        if version == 1:
            last_use_day = -1
            pos_index = 3
        else:
            last_use_day = _bounded_int(row[3], -1, 10_000_000)
            pos_index = 4
        xy = _bounded_xy(row[pos_index], row[pos_index + 1])
        z = _finite_number(row[pos_index + 2])
        normal_x = _finite_number(row[pos_index + 3])
        normal_y = _finite_number(row[pos_index + 4])
        if (cell_x is None or cell_y is None or steps is None or steps < 1 or last_use_day is None or xy is None
                or z is None or abs(z) > MAX_COORD_CM or normal_x is None or normal_y is None
                or abs(normal_x) > 1 or abs(normal_y) > 1 or normal_x * normal_x + normal_y * normal_y > 1.0001):
            raise AtlasInputError(f"trail cell {index} contains invalid or out-of-bounds values")
        if math.floor(xy[0] / TRAIL_CELL_SIZE_CM) != cell_x or math.floor(xy[1] / TRAIL_CELL_SIZE_CM) != cell_y:
            raise AtlasInputError(f"trail cell {index} position does not match its stable cell key")
        key = (cell_x, cell_y)
        if key in keys:
            raise AtlasInputError(f"trail ledger repeats cell key {key}")
        keys.add(key)
        cells.append({
            "cell_x": cell_x, "cell_y": cell_y, "steps": steps, "last_use_day": last_use_day,
            "x": xy[0], "y": xy[1], "region": [math.floor(cell_x / TRAIL_REGION_SPAN), math.floor(cell_y / TRAIL_REGION_SPAN)],
        })
    return {"version": version, "cells": cells, "skipped_cells": 0}


def _event_xy(entry: dict[str, Any]) -> tuple[float, float] | None:
    xy = _point_from_array(entry.get("location"))
    if xy is not None:
        return xy
    at = entry.get("at")
    if isinstance(at, str) and len(at) <= 128:
        parts = at.split(",")
        if len(parts) >= 2:
            try:
                return _bounded_xy(float(parts[0]), float(parts[1]))
            except ValueError:
                pass
    if "region_x" in entry and "region_y" in entry:
        region_x = _chronicle_int(entry.get("region_x"), -20_000, 20_000)
        region_y = _chronicle_int(entry.get("region_y"), -20_000, 20_000)
        if region_x is not None and region_y is not None:
            span_cm = TRAIL_CELL_SIZE_CM * TRAIL_REGION_SPAN
            return ((region_x + 0.5) * span_cm, (region_y + 0.5) * span_cm)
    if "x" in entry and "y" in entry:
        return _bounded_xy(entry.get("x"), entry.get("y"))
    return None


def _region_for_xy(x: float, y: float) -> tuple[int, int]:
    return (math.floor(x / (TRAIL_CELL_SIZE_CM * TRAIL_REGION_SPAN)),
            math.floor(y / (TRAIL_CELL_SIZE_CM * TRAIL_REGION_SPAN)))


def load_chronicle(path: Path | None) -> dict[str, Any]:
    if path is None:
        return {"events": [], "invalid_lines": 0, "unsupported_types": 0, "last_recorded_at": None}
    try:
        if path.stat().st_size > MAX_CHRONICLE_BYTES:
            raise AtlasInputError(f"{path.name} exceeds the {MAX_CHRONICLE_BYTES:,}-byte input limit")
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise AtlasInputError(f"cannot read {path}: {exc}") from exc
    except UnicodeError as exc:
        raise AtlasInputError(f"{path.name} is not valid UTF-8 JSONL") from exc
    if len(lines) > MAX_CHRONICLE_EVENTS:
        raise AtlasInputError(f"chronicle exceeds the {MAX_CHRONICLE_EVENTS:,}-line event limit")
    events: list[dict[str, Any]] = []
    invalid_lines = 0
    unsupported_types = 0
    last_recorded_at = None
    for line in lines:
        if not line.strip():
            continue
        try:
            entry = json.loads(line)
        except json.JSONDecodeError:
            invalid_lines += 1
            continue
        if not isinstance(entry, dict):
            invalid_lines += 1
            continue
        kind = entry.get("type")
        if not isinstance(kind, str) or kind not in KNOWN_EVENT_TYPES:
            unsupported_types += 1
            continue
        day = _bounded_int(entry.get("day", 0), 0, 10_000_000)
        if day is None:
            invalid_lines += 1
            continue
        text = entry.get("text", "")
        agent = entry.get("agent", "")
        if not isinstance(text, str) or len(text) > 2_000 or not isinstance(agent, str) or len(agent) > 100:
            invalid_lines += 1
            continue
        point = _event_xy(entry)
        timestamp = entry.get("t", "")
        clock = entry.get("clock", "")
        if not isinstance(timestamp, str) or len(timestamp) > 64 or not isinstance(clock, str) or len(clock) > 16:
            invalid_lines += 1
            continue
        if timestamp:
            last_recorded_at = timestamp
        event = {"type": kind, "day": day, "clock": clock, "timestamp": timestamp,
                 "agent": agent, "text": text.strip(), "event": str(entry.get("event", ""))[:80]}
        if point is not None:
            event["x"], event["y"] = point
            event["region"] = list(_region_for_xy(*point))
        elif "region_x" in entry and "region_y" in entry:
            rx = _chronicle_int(entry.get("region_x"), -20_000, 20_000)
            ry = _chronicle_int(entry.get("region_y"), -20_000, 20_000)
            if rx is not None and ry is not None:
                event["region"] = [rx, ry]
        events.append(event)
    return {"events": events, "invalid_lines": invalid_lines,
            "unsupported_types": unsupported_types, "last_recorded_at": last_recorded_at}


def load_anchors(path: Path | None) -> tuple[list[dict[str, Any]], int]:
    if path is None:
        return [], 0
    root = _read_json(path, 256 * 1024)
    records = root.get("anchors")
    if not isinstance(records, list) or len(records) > 500:
        raise AtlasInputError("anchor file must have an 'anchors' array of at most 500 records")
    anchors = []
    skipped = 0
    for record in records:
        if not isinstance(record, dict):
            skipped += 1
            continue
        xy = _bounded_xy(record.get("x"), record.get("y"))
        anchor_id = record.get("id")
        label = record.get("label", anchor_id)
        source = record.get("source", "user-supplied anchor")
        if (xy is None or not isinstance(anchor_id, str) or not anchor_id.strip() or len(anchor_id) > 80
                or not isinstance(label, str) or len(label) > 120 or not isinstance(source, str) or len(source) > 200):
            skipped += 1
            continue
        anchors.append({"id": anchor_id.strip(), "label": label.strip(), "source": source.strip(), "x": xy[0], "y": xy[1]})
    return anchors, skipped


def _fmt(value: float) -> str:
    return f"{value:,.0f} cm"


def render(data: dict[str, Any], captured_at: str | None = None) -> str:
    cells = data["trails"]["cells"]
    points = data["world"]["points"]
    anchors = data["anchors"]
    events = data["chronicle"]["events"]
    all_points = [(row["x"], row["y"]) for row in cells + points + anchors]
    all_points += [(event["x"], event["y"]) for event in events if "x" in event]
    if all_points:
        min_x = min(point[0] for point in all_points)
        max_x = max(point[0] for point in all_points)
        min_y = min(point[1] for point in all_points)
        max_y = max(point[1] for point in all_points)
        span_x = max(max_x - min_x, TRAIL_CELL_SIZE_CM)
        span_y = max(max_y - min_y, TRAIL_CELL_SIZE_CM)
        pad_x, pad_y = span_x * 0.06, span_y * 0.06
        min_x -= pad_x
        max_x += pad_x
        min_y -= pad_y
        max_y += pad_y
    else:
        min_x, min_y, max_x, max_y = -1.0, -1.0, 1.0, 1.0
    width, height, margin = 1000.0, 680.0, 48.0
    span_x, span_y = max_x - min_x, max_y - min_y
    scale = min((width - margin * 2) / span_x, (height - margin * 2) / span_y)
    content_w, content_h = span_x * scale, span_y * scale
    offset_x = (width - content_w) / 2
    offset_y = (height - content_h) / 2

    def xy(x: float, y: float) -> tuple[float, float]:
        return offset_x + (x - min_x) * scale, height - offset_y - (y - min_y) * scale

    regions = sorted({tuple(cell["region"]) for cell in cells} |
                     {tuple(_region_for_xy(point["x"], point["y"])) for point in points + anchors} |
                     {tuple(event["region"]) for event in events if "region" in event})
    days = sorted({event["day"] for event in events})
    svg_parts = ['<svg viewBox="0 0 1000 680" role="img" aria-label="Island trail and event evidence map">']
    svg_parts.append('<rect width="1000" height="680" fill="#edf1ed"/>')
    if not all_points:
        svg_parts.append('<text x="500" y="330" text-anchor="middle" class="empty">No spatial evidence in the supplied snapshot.</text>')
    else:
        for cell in cells:
            px, py = xy(cell["x"], cell["y"])
            cell_px = max(3.0, min(18.0, TRAIL_CELL_SIZE_CM * scale * 0.82))
            opacity = 0.24 + 0.72 * math.log1p(cell["steps"]) / math.log1p(40)
            title = f"Trail cell • {_fmt(cell['x'])}, {_fmt(cell['y'])} • {cell['steps']} recorded steps • last use day {cell['last_use_day'] if cell['last_use_day'] >= 0 else 'unknown'}"
            region = f"{cell['region'][0]},{cell['region'][1]}"
            svg_parts.append(
                f'<rect class="trail-cell" data-region="{region}" x="{px-cell_px/2:.2f}" y="{py-cell_px/2:.2f}" '
                f'width="{cell_px:.2f}" height="{cell_px:.2f}" rx="1" opacity="{min(opacity, 0.96):.2f}"><title>{html.escape(title)}</title></rect>')
        for point in points:
            px, py = xy(point["x"], point["y"])
            region = _region_for_xy(point["x"], point["y"])
            title = f"Saved {point['kind']}: {point['id']} • {_fmt(point['x'])}, {_fmt(point['y'])}"
            svg_parts.append(f'<circle class="saved-point" data-region="{region[0]},{region[1]}" cx="{px:.2f}" cy="{py:.2f}" r="5"><title>{html.escape(title)}</title></circle>')
        for anchor in anchors:
            px, py = xy(anchor["x"], anchor["y"])
            region = _region_for_xy(anchor["x"], anchor["y"])
            title = f"Anchor: {anchor['label']} • source: {anchor['source']} • {_fmt(anchor['x'])}, {_fmt(anchor['y'])}"
            svg_parts.append(f'<path class="anchor" data-region="{region[0]},{region[1]}" d="M{px:.2f} {py-8:.2f} L{px+7:.2f} {py+6:.2f} L{px-7:.2f} {py+6:.2f} Z"><title>{html.escape(title)}</title></path>')
        map_events = [event for event in events if "x" in event][-MAX_MAP_EVENTS:]
        for event in map_events:
            px, py = xy(event["x"], event["y"])
            region = f"{event['region'][0]},{event['region'][1]}"
            title = f"Day {event['day']} {event['clock']} • {event['type']}: {event['text']}"
            svg_parts.append(f'<circle class="event-point type-{html.escape(event["type"], quote=True)}" data-day="{event["day"]}" data-region="{region}" cx="{px:.2f}" cy="{py:.2f}" r="6"><title>{html.escape(title)}</title></circle>')
    svg_parts.append('</svg>')

    day_options = ''.join(f'<option value="{day}">Day {day}</option>' for day in days)
    region_options = ''.join(f'<option value="{x},{y}">Region {x}, {y}</option>' for x, y in regions)
    timeline = []
    shown_events = events[-MAX_TIMELINE_EVENTS:]
    for event in reversed(shown_events):
        label = event["agent"].replace("Agent_", "").replace("_01", "").replace("_", " ") or "World"
        when = f'Day {event["day"]} {event["clock"]}'.strip()
        region = f'{event["region"][0]},{event["region"][1]}' if "region" in event else ""
        location = "mapped" if "x" in event else ("region located" if region else "no spatial provenance")
        text = event["text"] or event["event"] or event["type"].replace("_", " ")
        timeline.append(f'<li data-day="{event["day"]}" data-region="{region}"><time>{html.escape(when)}</time> <b>{html.escape(label)}</b> <span class="kind">{html.escape(event["type"])}</span><p>{html.escape(text)}</p><small>{location}</small></li>')
    timeline_html = '\n'.join(timeline) or '<li class="no-events">No accepted chronicle events in this snapshot. The Atlas does not infer activity.</li>'
    freshness = f"Snapshot label: {html.escape(captured_at)}" if captured_at else "Snapshot time not supplied; current runtime state is not queried."
    latest = data["chronicle"]["last_recorded_at"]
    latest_text = f"Latest accepted chronicle timestamp: {html.escape(latest)}." if latest else "No accepted chronicle timestamp was found."
    omitted_markers = max(0, sum(1 for event in events if "x" in event) - MAX_MAP_EVENTS)
    omitted_timeline = max(0, len(events) - MAX_TIMELINE_EVENTS)
    notes = []
    if data["chronicle"]["invalid_lines"]:
        notes.append(f"Skipped {data['chronicle']['invalid_lines']} malformed chronicle line(s).")
    if data["chronicle"]["unsupported_types"]:
        notes.append(f"Omitted {data['chronicle']['unsupported_types']} event(s) with unsupported types.")
    if data["world"]["skipped_points"]:
        notes.append(f"Omitted {data['world']['skipped_points']} world-state place record(s) with invalid coordinates or IDs.")
    if omitted_markers:
        notes.append(f"Showing the latest {MAX_MAP_EVENTS:,} spatial event markers; {omitted_markers:,} earlier markers omitted.")
    if omitted_timeline:
        notes.append(f"Showing the latest {MAX_TIMELINE_EVENTS:,} chronicle lines; {omitted_timeline:,} earlier lines omitted.")
    if data["anchors_skipped"]:
        notes.append(f"Omitted {data['anchors_skipped']} invalid supplied anchor(s).")
    if not cells:
        notes.append("No trail cells were supplied; the map contains only located place and event records.")
    if notes:
        note_html = '<ul>' + ''.join(f'<li>{html.escape(note)}</li>' for note in notes) + '</ul>'
    else:
        note_html = '<p>All supplied bounded records are shown.</p>'
    embedded_data = json.dumps({"days": days, "regions": [list(region) for region in regions]}, ensure_ascii=False).replace("<", "\\u003c").replace(">", "\\u003e").replace("&", "\\u0026")
    return f'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Island Atlas — saved evidence</title>
<style>
:root{{--ink:#172728;--muted:#536866;--paper:#f2f4ee;--line:#cbd4ca;--moss:#526f50;--water:#416e78;--amber:#b77837}}
*{{box-sizing:border-box}}body{{margin:0;background:var(--paper);color:var(--ink);font:15px/1.5 system-ui,Segoe UI,sans-serif}}
header{{padding:1.25rem clamp(1rem,4vw,3rem);background:#223b3d;color:#edf0e8}}h1{{margin:.1rem 0;font:500 clamp(1.6rem,3vw,2.5rem)/1.15 Georgia,serif}}header p{{margin:.45rem 0;color:#c5d2ca}}
main{{max-width:1500px;margin:auto;padding:1rem clamp(.75rem,2vw,1.5rem)}}.notice{{border-left:4px solid #d1a469;padding:.65rem .85rem;background:#fff9ef;margin-bottom:1rem}}
.filters{{display:flex;gap:1rem;flex-wrap:wrap;align-items:end;margin:1rem 0}}label{{display:grid;gap:.25rem;font-weight:650}}select{{min-width:11rem;padding:.5rem;border:1px solid var(--line);border-radius:.35rem;background:white;color:var(--ink)}}
.layout{{display:grid;grid-template-columns:minmax(0,1.4fr) minmax(320px,.8fr);gap:1rem;align-items:start}}section{{background:#fff;border:1px solid var(--line);border-radius:.55rem;overflow:hidden}}h2{{font:500 1.25rem Georgia,serif;margin:0;padding:.75rem 1rem;border-bottom:1px solid var(--line)}}.map-wrap{{padding:.5rem;background:#e8ede8}}svg{{display:block;width:100%;height:auto;min-height:330px;background:#edf1ed;border:1px solid #d2dbd2}}.trail-cell{{fill:#675d43}}.saved-point{{fill:var(--water);stroke:white;stroke-width:1.5}}.anchor{{fill:#d39552;stroke:#653e1d;stroke-width:1.5}}.event-point{{fill:#a65b46;stroke:white;stroke-width:1.5}}.empty{{fill:#5e706e;font:18px Georgia,serif}}.legend{{display:flex;gap:1rem;flex-wrap:wrap;padding:.65rem 1rem;color:var(--muted);font-size:.9rem}}.key::before{{content:"";display:inline-block;width:.7rem;height:.7rem;border-radius:50%;margin-right:.35rem;background:var(--water)}}.key.trail::before{{border-radius:1px;background:#675d43}}.key.anchor::before{{background:#d39552}}.key.event::before{{background:#a65b46}}
.timeline{{max-height:680px;overflow:auto;padding:.6rem 1rem}}ol{{list-style:none;padding:0;margin:0}}li{{padding:.65rem 0;border-bottom:1px solid #e4e9e1}}li:last-child{{border:0}}time,small,.kind{{color:var(--muted);font-size:.8rem}}.kind{{padding:.08rem .35rem;background:#edf2ed;border-radius:.25rem}}li p{{margin:.25rem 0}}.muted{{color:var(--muted)}}.notes{{padding:.5rem 1rem 1rem;color:var(--muted)}}.notes ul{{padding-left:1.2rem}}footer{{padding:1rem;color:var(--muted);font-size:.85rem}}
@media(max-width:900px){{.layout{{grid-template-columns:1fr}}.timeline{{max-height:480px}}}}
@media(prefers-color-scheme:dark){{:root{{--ink:#dce6df;--muted:#aabbb2;--paper:#151d1d;--line:#344442}}section{{background:#202c2b}}select{{background:#202c2b;color:var(--ink)}}.map-wrap{{background:#25302e}}svg{{background:#283331;border-color:#344442}}.trail-cell{{fill:#c0a56f}}.saved-point{{stroke:#283331}}.event-point{{stroke:#283331}}.notice{{background:#302a20}}}}
</style></head><body>
<header><h1>Island Atlas</h1><p>A read-only lens on saved world history</p></header>
<main><div class="notice"><b>Static snapshot.</b> This page reads only the files supplied to the builder. It does not connect to a live Unreal session or infer events that were not recorded.</div>
<p class="muted">{freshness} {latest_text} World-state schema {data['world']['version']}; trail schema {data['trails']['version'] if data['trails']['version'] is not None else 'not supplied'}.</p>
<div class="filters"><label>Chronicle day<select id="dayFilter"><option value="all">All recorded days</option>{day_options}</select></label><label>12 m trail region<select id="regionFilter"><option value="all">All regions</option>{region_options}</select></label><span class="muted">Filters affect recorded event markers and the event list. Trail cells show the supplied snapshot, not a reconstructed past.</span></div>
<div class="layout"><section><h2>Trails and located evidence</h2><div class="map-wrap">{''.join(svg_parts)}</div><div class="legend"><span class="key trail">Wear cell (1.5 m)</span><span class="key">Saved place</span><span class="key anchor">Supplied anchor</span><span class="key event">Chronicle event</span></div><div class="notes">{note_html}</div></section>
<section><h2>Recorded events</h2><div class="timeline"><ol id="eventList">{timeline_html}</ol><p id="filterEmpty" class="muted" hidden>No recorded events match these filters.</p></div></section></div>
<footer>World XY coordinates are preserved from the snapshot; positive Y is shown north/up. Exact-source provenance appears in map markers. Events without coordinates remain in the timeline only.</footer></main>
<script type="application/json" id="atlasData">{embedded_data}</script>
<script>(()=>{{const day=document.getElementById('dayFilter'),region=document.getElementById('regionFilter'),items=[...document.querySelectorAll('#eventList li[data-day]')],mapped=[...document.querySelectorAll('[data-region]')],empty=document.getElementById('filterEmpty');function apply(){{let shown=0;const dayMatches=e=>!e.dataset.day||day.value==='all'||e.dataset.day===day.value;const regionMatches=e=>region.value==='all'||e.dataset.region===region.value;for(const item of items){{const visible=dayMatches(item)&&regionMatches(item);item.hidden=!visible;if(visible)shown++}}for(const point of mapped)point.hidden=!regionMatches(point)||(!point.classList.contains('trail-cell')&&!point.classList.contains('saved-point')&&!point.classList.contains('anchor')&&!dayMatches(point));empty.hidden=shown!==0||items.length===0}}day.addEventListener('change',apply);region.addEventListener('change',apply);apply()}})();</script>
</body></html>'''


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--world-state", required=True, type=Path, help="explicit copied WorldState/<Map>.json snapshot")
    parser.add_argument("--trails", type=Path, help="explicit copied <Map>.trails.json trail ledger")
    parser.add_argument("--chronicle", type=Path, help="explicit copied chronicle.jsonl")
    parser.add_argument("--anchors", type=Path, help="optional JSON object with an 'anchors' array of sourced x/y points")
    parser.add_argument("--captured-at", help="human-supplied label for when the source snapshot was copied")
    parser.add_argument("--out", required=True, type=Path, help="write the standalone HTML here")
    args = parser.parse_args()
    try:
        world = load_world_state(args.world_state)
        trails = load_trails(args.trails)
        chronicle = load_chronicle(args.chronicle)
        anchors, anchors_skipped = load_anchors(args.anchors)
        page = render({"world": world, "trails": trails, "chronicle": chronicle,
                       "anchors": anchors, "anchors_skipped": anchors_skipped}, args.captured_at)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(page, encoding="utf-8")
    except AtlasInputError as exc:
        print(f"World Atlas: {exc}", file=sys.stderr)
        return 2
    print(f"Wrote read-only snapshot atlas to {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
