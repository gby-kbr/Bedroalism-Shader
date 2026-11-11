import json
import os
import shutil
from rich.console import Console
from util import BUILD_PATH, CAN_BUILD_ALL, MCPACK_PATH, PROFILE
import audit
import build

console = Console()


def run(args):
    """Build: compile the materials into material.bin and pack them with mcpack/ into a .mcpack."""
    if not CAN_BUILD_ALL:
        console.print("Error: the multiplatform build needs Windows, the HLSL shaders cannot be compiled on this system.", style="bold red")
        console.print("       Use the GitHub workflow or a Windows machine. `check` still works here for Android and iOS.", style="red")
        exit(1)
    if not build.ready():
        exit(1)

    console.print("~ Audit", style="bold")
    problems = audit.run(console)
    if problems:
        for p in problems:
            console.print("  " + p, style="red")
        exit(1)

    out = os.path.join(BUILD_PATH, PROFILE)
    console.print("\n~ Compile (Android, iOS, Windows)", style="bold")
    if not build.compile_materials(out, PROFILE):
        exit(1)
    problems = build.verify(out)
    if problems:
        for p in problems:
            console.print("  " + p, style="red")
        exit(1)

    with open(os.path.join(MCPACK_PATH, "manifest.json"), encoding="utf-8") as f:
        manifest = json.load(f)
    if args.v:
        if not args.v.isdigit():
            console.print(f"Invalid version number '{args.v}', use the patch number, eg: 3", style="bold red")
            exit(1)
        manifest["header"]["version"][2] = int(args.v)
    version = ".".join(str(n) for n in manifest["header"]["version"])
    name = manifest["header"]["name"].replace(" ", "-")

    pack_dir = os.path.join(BUILD_PATH, "pack")
    shutil.rmtree(pack_dir, ignore_errors=True)
    shutil.copytree(MCPACK_PATH, pack_dir)
    mats_dir = os.path.join(pack_dir, "renderer", "materials")
    os.makedirs(mats_dir)
    for name_bin in sorted(os.listdir(out)):
        if name_bin.endswith(".material.bin"):
            shutil.copy(os.path.join(out, name_bin), mats_dir)
    with open(os.path.join(pack_dir, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    count = len(os.listdir(mats_dir))
    console.print(f"\n~ Pack\n  [dim]name      :[/] {manifest['header']['name']} v{version}\n  [dim]materials :[/] {count} .material.bin")

    if not args.no_zip:
        archive = os.path.join(BUILD_PATH, f"{name}-v{version}.mcpack")
        shutil.make_archive(pack_dir, "zip", pack_dir)
        if os.path.exists(archive):
            os.remove(archive)
        os.rename(pack_dir + ".zip", archive)
        console.print(f"  [dim]file      :[/] {archive}")
    console.print("\n[bold green]Done.")
