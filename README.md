# Bedroalism Shader

Realistic shader for Minecraft Bedrock (RenderDragon), built for Android, iOS and Windows at once.

## Build

Needs Python 3.11+ and `pip install -r requirements.txt`. The build tool lives in `tool/`.

| Command | What it does |
| --- | --- |
| `build setup` | downloads shaderc and the source materials into `tool/data` (once) |
| `build check` | audits the sources and test compiles every material, makes no files |
| `build build [-v PATCH] [--no-zip]` | compiles `material.bin` for all platforms and packs `build/Bedroalism-Shader-v<version>.mcpack` |

Use `./build.sh` on Linux, macOS and Termux, and `build.bat` on Windows.

The audit checks that every `br` function is called, every `BR_` define in `config.h` is used and defined,
every include exists, and every material folder is listed in `src/materials/project.json`.

Windows (HLSL) shaders can only be compiled by the Windows shaderc, so `build` needs Windows. On other systems `check`
tests Android and iOS only. The GitHub workflows run on Windows:

- **build-check** runs `check` on every push and pull request to `main`.
- **build** (run manually, optional patch version) runs `check` and `build` and uploads the `.mcpack`.

## Layout

- `src/bedroalism` shader config and functions, `src/materials` the materials
- `mcpack` pack files (manifest, textures, fogs, biomes), the compiled materials are added to `renderer/materials`
- `script` generators for the textures
