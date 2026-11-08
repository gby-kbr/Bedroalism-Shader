"""Paths, download sources and small helpers shared by the build commands."""
import json
import os
import pickle
import platform

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MATERIALS_SRC = os.path.join("src", "materials")          # shader source, with project.json
PACK_SRC = "mcpack"                                        # resource pack content: manifest, textures, ...
DATA_DIR = os.path.join("tool", "data")                    # downloaded build dependencies
BASE_MATERIALS_DIR = os.path.join(DATA_DIR, "materials")   # vanilla material definitions (.material.json)
CONF_FILE = os.path.join(DATA_DIR, ".builder.pkl")
BUILD_DIR = "build"

SHADERC_PATH = os.path.join(DATA_DIR, "shaderc") + (".exe" if os.name == "nt" else "")
BASE_MATERIALS_SAMPLE = os.path.join(BASE_MATERIALS_DIR, "Sky.material.json")

# The build dependencies live on our own release first. The release is filled by the "import-tools" workflow
# from the dev release of Newb Shader, which stays as a fallback while ours is still empty.
SOURCES = (
    "https://github.com/gby-kbr/Bedroalism-Shader/releases/download/build-tools/",
    "https://github.com/devendrn/newb-shader/releases/download/dev/",
)
BASE_MATERIALS_ZIP = "src-materials-1.26.60.zip"
SHADERC_PREFIX = "shaderc-"

PROFILES = ("multiplatform", "android", "ios", "windows")
LOADERS = {
    "android": "MB Loader",
    "ios": "Minecraft with Hynis",
    "windows": "Wyvern Loader",
    "multiplatform": "MB Loader (Android), Wyvern Loader (Windows) or Minecraft with Hynis (iOS)",
}


def default_profile():
    return "windows" if os.name == "nt" else "android"


def shaderc_asset_name(os_name: str, arch: str):
    """Name of the shaderc binary of the release for this machine, None when there is none."""
    if os_name == "Windows":
        return SHADERC_PREFIX + "win-x64.exe"
    if os_name == "Darwin":
        return SHADERC_PREFIX + "osx-x64"
    if os_name in ("Linux", "Android"):
        if arch == "x86_64":
            return SHADERC_PREFIX + "linux-x64"
        if arch in ("aarch64", "arm64"):
            return SHADERC_PREFIX + "android-arm64"
        if arch in ("armv8l", "armv7l"):
            return SHADERC_PREFIX + "android-arm"
    return None


def material_names():
    """Materials of the project: the folders of src/materials that hold a shader."""
    names = []
    for name in sorted(os.listdir(MATERIALS_SRC)):
        folder = os.path.join(MATERIALS_SRC, name)
        if os.path.isdir(folder) and not name.startswith((".", "_")) and os.path.exists(os.path.join(folder, "material.json")):
            names.append(name)
    return names


def material_paths(names):
    """Folders of the given materials. Raises FileNotFoundError(name) for an unknown one."""
    known = os.listdir(MATERIALS_SRC)
    paths = []
    for name in names:
        if name not in known:
            raise FileNotFoundError(name)
        paths.append(os.path.join(MATERIALS_SRC, name))
    return paths


def print_styled_error(console, log: str):
    """Prints the log of the shader compiler, errors red and the rest dim."""
    for line in log.strip().split("\n"):
        words = line.split()
        if not words:
            continue
        style = "dim"
        if words[0] == "Warning:":
            style = "red"
        elif words[0] in ("Error:", ">>>", "cpp:"):
            style = "bold red"
        console.print(line, style=style, markup=False, highlight=False)


def load_conf():
    try:
        with open(CONF_FILE, "rb") as file:
            conf = pickle.load(file)
            return conf if isinstance(conf, dict) else {}
    except Exception:
        return {}


def save_conf(conf: dict):
    os.makedirs(DATA_DIR, exist_ok=True)
    with open(CONF_FILE, "wb") as file:
        pickle.dump(conf, file)


def check_setup(console):
    """True when 'setup' was done on this machine and the dependencies are there."""
    conf = load_conf()
    same_machine = conf.get("os_name") == platform.system() and conf.get("arch") == platform.machine()
    present = os.path.exists(SHADERC_PATH) and os.path.exists(BASE_MATERIALS_SAMPLE)
    if present and same_machine:
        return True
    console.print("Error: build dependencies are missing or belong to another machine.", style="bold red")
    console.print("       Please run the setup command first.", style="bold red")
    return False


def read_manifest():
    with open(os.path.join(PACK_SRC, "manifest.json"), "r", encoding="utf-8") as file:
        return json.load(file)
