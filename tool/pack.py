"""pack: compiles the materials and puts them into the resource pack of the mcpack folder, as a .mcpack file."""
import os
import shutil
from importlib import metadata

from rich.console import Console

from build import compile_materials
from util import BUILD_DIR, PACK_SRC, check_setup, read_manifest

console = Console()


def lazurite_version():
    try:
        return metadata.version("lazurite")
    except metadata.PackageNotFoundError:
        return "unknown"


def run(args):
    if not check_setup(console):
        exit(1)

    # the manifest is the one of mcpack, as it is: the version is set by hand in mcpack/manifest.json
    header = read_manifest()["header"]
    version = ".".join(str(part) for part in header["version"])
    name = header["name"]
    profile = args.p
    console.print(f"[bold green]{name}[/] [dim]{version}[/] for [cyan]{profile}[/]  [dim]Lazurite {lazurite_version()}")

    pack_dir = os.path.join(BUILD_DIR, "pack-" + profile)
    if os.path.exists(pack_dir):
        shutil.rmtree(pack_dir)
    shutil.copytree(PACK_SRC, pack_dir)

    compile_materials(profile, os.path.join(pack_dir, "renderer", "materials"))

    if args.no_archive:
        console.print(f"[bold green]Pack folder:[/] {pack_dir}")
        return

    # a .mcpack is the pack itself, no extra archive around it
    file_name = f"{name.replace(' ', '-')}-{version}-{profile}"
    archive = shutil.make_archive(os.path.join(BUILD_DIR, file_name), "zip", pack_dir)
    final = os.path.join(BUILD_DIR, file_name + ".mcpack")
    if os.path.exists(final):
        os.remove(final)
    os.rename(archive, final)
    console.print(f"[bold green]Pack:[/] {final}")
