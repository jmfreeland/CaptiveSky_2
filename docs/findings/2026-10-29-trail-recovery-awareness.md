# Trail ledger: resident awareness of recovery

Trail cells now keep the Island day of their most recent valid footfall. Residents'
existing underfoot context can distinguish a well-used track from one that has
been left alone long enough for grass to begin creeping back. The notice appears
after three Island days without a recorded step and only while the track is still
visibly flattened. Recent prints, worn-ground descriptions, and decay behavior
remain unchanged.

The trail JSON schema is version 2. Version-1 ledgers still load, with their
unknown use dates left unknown; no history is invented for existing paths. New
footfalls gradually replace those unknown dates as cells are crossed again. This
does not yet aggregate cells into canonical named routes or polylines, nor does it
change the trail's visual material. Those remain useful next steps from the
worldbuilding proposal.

## Verification

- UE 5.8 isolated scratch editor target built and linked successfully.
- `CaptiveSky2.Agent.IslandTrail` and `CaptiveSky2.Agent.IslandTrailVisitor`
  passed under NullRHI. Coverage includes date updates, v2 save/load, v1
  compatibility, malformed date rejection, three-day recovery wording, and the
  existing read-only visitor interaction.
- Agent thinking was disabled, the model-request cap was zero, and the data root
  was isolated under the ignored scratch directory.
- No map, Content asset, or live editor state was changed. In-world wording and
  visual transition remain to be reviewed in a bounded Island playtest.
