# Minnow response to a common-frog hop

The Tideglass minnow school now briefly scatters away from a nearby Common Frog while it is actively hopping. The response is sampled on the existing 0.4-second rat-disturbance cadence, is limited to 700 cm horizontally and 350 cm vertically, and rearms after the frog settles or leaves the wider 900 cm ring. Each frog encounter is latched so one hop does not repeatedly restart the school response. The behavior is local and deterministic; it does not call an LLM or persist world changes.

This is a visual startle cue, not evidence that the frog entered the water: it intentionally creates no ripple or splash. A short cooldown and the existing scatter state prevent overlapping cues from stacking.

## Validation

The isolated UE 5.8.3 editor target completed UHT, compiled the changed frog/minnow and automation translation units, and linked its scratch `UnrealEditor-CaptiveSky_2.dll` successfully (19 actions). The scratch project has separate binaries; the main editor's loaded DLL was not touched.

The focused NullRHI automation launch exited without creating the requested test log or reporting test discovery/results. Treat `CaptiveSky2.Agent.MinnowFrogCue` as not run, not passed; similar scratch commandlet launches have failed at project pre-init. The interactive editor was left untouched, and no map or Content asset was changed. A live-world visual check remains pending.
