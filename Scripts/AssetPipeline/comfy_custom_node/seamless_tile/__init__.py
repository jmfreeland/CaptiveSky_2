"""SeamlessTile: switch a model's and VAE's Conv2d layers to circular padding so
generated images wrap at the edges and tile without visible seams.

Note: this edits the loaded modules in place (the model weights are shared), so
the setting stays until another SeamlessTile node switches it back.
"""
from typing_extensions import override

import torch
from comfy_api.latest import ComfyExtension, io


def _set_padding(module, circular):
    mode = "circular" if circular else "zeros"
    for m in module.modules():
        if isinstance(m, torch.nn.Conv2d):
            m.padding_mode = mode


class SeamlessTile(io.ComfyNode):
    @classmethod
    def define_schema(cls):
        return io.Schema(
            node_id="SeamlessTile",
            display_name="Seamless Tile (circular padding)",
            category="model/patch",
            inputs=[
                io.Model.Input("model"),
                io.Vae.Input("vae"),
                io.Boolean.Input("enable", default=True),
            ],
            outputs=[io.Model.Output(), io.Vae.Output()],
        )

    @classmethod
    def execute(cls, model, vae, enable) -> io.NodeOutput:
        _set_padding(model.model.diffusion_model, enable)
        _set_padding(vae.first_stage_model, enable)
        return io.NodeOutput(model, vae)


class SeamlessTileExtension(ComfyExtension):
    @override
    async def get_node_list(self) -> list[type[io.ComfyNode]]:
        return [SeamlessTile]


async def comfy_entrypoint() -> SeamlessTileExtension:
    return SeamlessTileExtension()
