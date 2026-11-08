# Bedroalism Shader

A realistic shader for Minecraft Bedrock Edition (RenderDragon), made for Android, iOS and Windows. It does not use
vibrant visuals. The shaders are `.material.bin` files, so they need a loader that patches the game:
MB Loader (Android), Minecraft with Hynis (iOS) or Wyvern Loader (Windows).

## Repository

```
├── src/bedroalism/     functions and settings of the shader (config.h holds every setting)
├── src/materials/      materials: vertex.sc, fragment.sc, varying.def.sc, project.json
├── include/            shared headers (bgfx_shader.sh, MinecraftRenderer.Materials)
├── mcpack/             content of the resource pack: manifest.json, textures, biomes, fogs
├── script/             scripts that generate the textures
├── tool/               build tool (Lazurite and shaderc)
├── build.sh / build.bat
└── .github/workflows/  Build, Build-Check, Import-Tools
```

## Building

Needs Python 3.10 or newer.

```
pip install -r requirements.txt
./build.sh setup                    # Windows: build.bat setup
./build.sh mats -p multiplatform    # compile only, to find errors
./build.sh pack -p multiplatform    # compile and build the .mcpack
```

| Command | What it does |
|---|---|
| `setup` | downloads shaderc and the vanilla material definitions into `tool/data` (`--reset` starts over) |
| `mats` | compiles the materials into `build/<profile>`. `-m Sky RenderChunk` compiles only those |
| `pack` | compiles into a copy of `mcpack` and writes `build/Bedroalism-Shader-<version>-<profile>.mcpack`. `--no-archive` keeps the folder |

`mcpack/manifest.json` is copied as it is: the build never creates or changes a manifest. The version of the pack,
and the number in the file name, is the one written there by hand.

Profiles (`-p`): `multiplatform` (Android, iOS and Windows in one pack), `android`, `ios`, `windows`.
Direct3D shaders (Windows) can only be compiled on Windows, so `multiplatform` and `windows` need a Windows machine.
Android and iOS compile on Linux, macOS and Termux.

The build dependencies are taken from the `build-tools` release of this repository. Until it exists they come from
the dev release of Newb Shader.

## Workflows

| Workflow | Does |
|---|---|
| **Build** | Manual. Compiles the multiplatform pack and uploads the `.mcpack` as it is, not wrapped in a `.zip`. The version is the one of `mcpack/manifest.json` |
| **Build-Check** | On pull requests and on pushes to `main` that change shader code, and manually. Runs the same compilation and stops at the first error or warning. Makes no file |
| **Import-Tools** | Manual, run once. Copies shaderc and the material definitions from Newb Shader to the `build-tools` release of this repository |

## Credits

- [Newb X](https://github.com/devendrn/newb-x-mcbe) - reference for the structure of a RenderDragon shader and its build
- [Lazurite](https://github.com/veka0/lazurite) - compiles and packs `.material.bin`
- [RenderDragon Shader Template](https://github.com/kren-0x/RenderDragon-Shader-Template)
- [mcbe-shader-codebase](https://github.com/veka0/mcbe-shader-codebase) - readable vanilla shader code
