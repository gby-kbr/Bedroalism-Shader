"""setup: downloads shaderc (the shader compiler) and the vanilla material definitions."""
import os
import platform
import shutil
import urllib.error
import zipfile
from urllib.request import urlopen

from rich.console import Console

from util import (BASE_MATERIALS_DIR, BASE_MATERIALS_SAMPLE, BASE_MATERIALS_ZIP, DATA_DIR, SHADERC_PATH, SOURCES,
                  load_conf, save_conf, shaderc_asset_name)

console = Console()


def download(asset: str, path: str):
    """Downloads an asset from the first source that has it. Returns the source used."""
    last_error = None
    for source in SOURCES:
        url = source + asset
        try:
            with urlopen(url, timeout=120) as response, open(path, "wb") as out:
                shutil.copyfileobj(response, out)
            console.print(f"  [dim]{asset}[/] from [cyan]{source}[/]")
            return source
        except (urllib.error.URLError, OSError) as error:
            last_error = error
            console.print(f"  [yellow]{asset} not found at {source}[/] ({error})")
    raise RuntimeError(f"Could not download {asset}: {last_error}")


def run(args):
    os_name, arch = platform.system(), platform.machine()
    asset = shaderc_asset_name(os_name, arch)
    if asset is None:
        console.print(f"No shaderc build for {os_name} {arch}.", style="bold red")
        exit(1)

    if args.reset and os.path.exists(DATA_DIR):
        shutil.rmtree(DATA_DIR)
    os.makedirs(BASE_MATERIALS_DIR, exist_ok=True)

    if os_name == "Android":
        # Termux misses libc++_shared.so for shaderc
        lib_dir = os.path.join("tool", "lib")
        termux_lib = "/data/data/com.termux/files/usr/lib/libc++_shared.so"
        os.makedirs(lib_dir, exist_ok=True)
        if os.path.exists(termux_lib) and not os.path.exists(os.path.join(lib_dir, "libc++_shared.so")):
            shutil.copyfile(termux_lib, os.path.join(lib_dir, "libc++_shared.so"))

    if not os.path.exists(SHADERC_PATH):
        console.print("Downloading shaderc")
        download(asset, SHADERC_PATH)
        os.chmod(SHADERC_PATH, 0o755)

    if not os.path.exists(BASE_MATERIALS_SAMPLE):
        console.print("Downloading material definitions")
        archive = os.path.join(DATA_DIR, BASE_MATERIALS_ZIP)
        download(BASE_MATERIALS_ZIP, archive)
        with zipfile.ZipFile(archive, "r") as zipped:
            zipped.extractall(BASE_MATERIALS_DIR)
        os.remove(archive)

    conf = load_conf()
    conf.update(os_name=os_name, arch=arch)
    save_conf(conf)
    console.print("[bold green]Setup done.")
