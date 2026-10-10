"""Village notice board: two posts, framed plank board, little gable roof, pinned parchment notes.
Residents (agents) can 'post' to it, so it is a story prop as much as a prop."""
import math

NAME = "NoticeBoard"


def build(A):
    for sx in (-0.7, 0.7):
        A.box("wood", (0.12, 0.12, 1.95), loc=(sx, 0, 0.975), bevel=0.008, name="post")
    # board: horizontal planks, grain along X
    A.box("wood", (1.5, 0.04, 0.95), loc=(0, 0.02, 1.3), grain="X", name="board")
    for z in (0.805, 1.795):
        A.box("wood", (1.62, 0.07, 0.07), loc=(0, -0.005, z), grain="X", bevel=0.006, name="frame_h")
    for x in (-0.78, 0.78):
        A.box("wood", (0.07, 0.07, 1.0), loc=(x, -0.005, 1.3), bevel=0.006, name="frame_v")
    # shelf under the board
    A.box("wood", (1.4, 0.2, 0.04), loc=(0, -0.09, 0.78), grain="X", bevel=0.005, name="shelf")
    A.box("wood", (1.4, 0.03, 0.06), loc=(0, -0.185, 0.815), grain="X", name="lip")
    # roof: two slabs + ridge
    for s in (1, -1):
        A.box("wood", (1.95, 0.045, 0.62), loc=(0, s * 0.27, 2.02), rot=(s * 38, 0, 0), grain="Z", bevel=0.004, name="roof")
    A.box("wood", (1.98, 0.1, 0.05), loc=(0, 0, 2.185), grain="X", name="ridge")
    # pinned notes
    notes = [(-0.5, 1.45, 4, 0.3, 0.38), (-0.08, 1.52, -3, 0.27, 0.34), (0.38, 1.42, 6, 0.3, 0.4),
             (-0.35, 1.08, -5, 0.26, 0.3), (0.12, 1.06, 2, 0.32, 0.26), (0.55, 1.06, -4, 0.2, 0.28)]
    for i, (x, z, rot, w, h) in enumerate(notes):
        A.box("parchment", (w, 0.006, h), loc=(x, -0.025 - i * 0.0008, z), rot=(0, rot, 0), grain="Z", tile=0.5, name="note")
        px = x + math.sin(math.radians(rot)) * (h / 2 - 0.03)
        A.cyl("iron", 0.011, 0.012, loc=(px, -0.034 - i * 0.0008, z + h / 2 - 0.03), rot=(90, 0, 0), segs=8, uv="box", name="pin")
    # a bell on a bracket to call residents to read the board
    A.box("wood", (0.4, 0.06, 0.06), loc=(0.9, -0.03, 1.72), bevel=0.004, name="bracket")
    A.box("wood", (0.06, 0.06, 0.3), loc=(0.78, -0.03, 1.57), rot=(0, 40, 0), name="bracketBrace")
    A.cyl("rope", 0.006, 0.08, loc=(1.02, -0.03, 1.64), segs=5, uv="box", name="bellRope")
    A.lathe("brass", [(0, 0.0), (0.075, 0.0), (0.07, 0.025), (0.055, 0.075), (0.04, 0.11), (0.045, 0.125), (0, 0.13)], loc=(1.02, -0.03, 1.47), segs=20, name="bell")
    A.rock("iron", (0.015, 0.015, 0.015), loc=(1.02, -0.03, 1.455), sub=1, rough=0.0, flat_bottom=0.9, name="clapper")
    for (x, z, rot, w, h) in ((-0.64, 1.1, 6, 0.16, 0.22), (0.62, 1.52, -5, 0.15, 0.2)):
        A.box("parchment", (w, 0.006, h), loc=(x, -0.032, z), rot=(0, rot, 0), name="note")
    A.collide_box((1.6, 0.25, 2.0), loc=(0, 0, 1.0))
