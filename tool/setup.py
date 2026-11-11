import os
import shutil
import zipfile
import platform
from functools import partial
from urllib.request import urlopen
from rich.progress import BarColumn, DownloadColumn, Progress, TextColumn, TransferSpeedColumn
from util import (load_conf, save_conf, DATA_PATH, MATERIALS_SRC_PATH, MAT_SRC_URL,
                  SHADERC_URL_PREFIX, SHADERC_PATH, SRC_MATERIALS_EG_PATH)

progress = Progress(
    TextColumn("[bold blue]{task.fields[filename]}", justify="right"),
    BarColumn(bar_width=None), "•", DownloadColumn(), "•", TransferSpeedColumn(),
)


def _download(url: str, path: str):
    task = progress.add_task("download", filename=url.split("/")[-1], start=False)
    response = urlopen(url)
    progress.update(task, total=int(response.info()["Content-length"]))
    with open(path, "wb") as dest:
        progress.start_task(task)
        for data in iter(partial(response.read, 32768), b""):
            dest.write(data)
            progress.update(task, advance=len(data))


def _shaderc_url(os_name: str, arch: str):
    if os_name == "Windows":
        return SHADERC_URL_PREFIX + "win-x64.exe"
    if os_name == "Darwin":
        return SHADERC_URL_PREFIX + "osx-x64"
    if os_name in ("Linux", "Android"):
        if arch == "x86_64":
            return SHADERC_URL_PREFIX + "linux-x64"
        if arch == "aarch64":
            return SHADERC_URL_PREFIX + "android-arm64"
        if arch == "armv8l":
            return SHADERC_URL_PREFIX + "android-arm"
    progress.console.print(f"No shaderc found for {os_name} {arch}", style="red")
    return None


def _termux_fix():
    # libc++_shared.so not found on termux
    lib_path = os.path.join("tool", "lib")
    os.makedirs(lib_path, exist_ok=True)
    termux_lib = "/data/data/com.termux/files/usr/lib/libc++_shared.so"
    target = os.path.join(lib_path, "libc++_shared.so")
    if os.path.exists(termux_lib) and not os.path.exists(target):
        shutil.copyfile(termux_lib, target)


def run(args):
    conf = load_conf()
    os_name, arch = platform.system(), platform.machine()
    shaderc_url = _shaderc_url(os_name, arch)
    if shaderc_url is None:
        exit(1)

    if args.reset and os.path.exists(DATA_PATH):
        shutil.rmtree(DATA_PATH)
        conf = {}
    os.makedirs(MATERIALS_SRC_PATH, exist_ok=True)
    if os_name == "Android":
        _termux_fix()

    # drop what an update makes outdated
    if conf.get("shaderc_url") != shaderc_url and os.path.exists(SHADERC_PATH):
        os.remove(SHADERC_PATH)
    if conf.get("mat_src_url") != MAT_SRC_URL:
        shutil.rmtree(MATERIALS_SRC_PATH, ignore_errors=True)
        os.makedirs(MATERIALS_SRC_PATH)

    with progress:
        if not os.path.exists(SHADERC_PATH):
            progress.console.print("Downloading shaderc")
            _download(shaderc_url, SHADERC_PATH)
            os.chmod(SHADERC_PATH, 0o755)
        if not os.path.exists(SRC_MATERIALS_EG_PATH):
            progress.console.print("Downloading source materials")
            archive = os.path.join(DATA_PATH, "materials.zip")
            _download(MAT_SRC_URL, archive)
            with zipfile.ZipFile(archive) as z:
                z.extractall(MATERIALS_SRC_PATH)
            os.remove(archive)

    conf.update(arch=arch, os_name=os_name, mat_src_url=MAT_SRC_URL,
                shaderc_url=shaderc_url, shaderc_url_prefix=SHADERC_URL_PREFIX)
    save_conf(conf)
    progress.console.print("[bold green]All done!")
