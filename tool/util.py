import os
import pickle
import platform
from rich.console import Console

# Windows (HLSL) can only be compiled by the Windows shaderc, so other systems can only test Android and iOS
CAN_BUILD_ALL = os.name == "nt"
PROFILE = "multiplatform"  # Android + iOS + Windows in one material.bin, the only target of this project

CONF_FILE = os.path.join("tool", "data", ".builder.pkl")
DATA_PATH = os.path.join("tool", "data")
MATERIALS_SRC_PATH = os.path.join(DATA_PATH, "materials")
RELEASE = "https://github.com/devendrn/newb-shader/releases/download/dev/"
MAT_SRC_URL = RELEASE + "src-materials-1.26.60.zip"
SHADERC_URL_PREFIX = RELEASE + "shaderc-"
SHADERC_PATH = os.path.join(DATA_PATH, "shaderc") + (".exe" if os.name == "nt" else "")
SRC_MATERIALS_EG_PATH = os.path.join(MATERIALS_SRC_PATH, "Sky.material.json")

SRC_PATH = "src"
PROJECT_PATH = os.path.join(SRC_PATH, "materials")
MCPACK_PATH = "mcpack"
BUILD_PATH = "build"


def print_styled_error(console: Console, log: str):
    for line in log.strip().split("\n"):
        words = line.split()
        if not words:
            continue
        style = "dim"
        if words[0] == "Warning:":
            style = "red"
        elif words[0] in ["Error:", ">>>", "cpp:"]:
            style = "bold red"
        console.print(line, style=style)


def load_conf() -> dict:
    try:
        with open(CONF_FILE, "rb") as file:
            conf = pickle.load(file)
            return conf if isinstance(conf, dict) else {}
    except Exception:
        return {}


def save_conf(conf: dict):
    with open(CONF_FILE, "wb") as file:
        pickle.dump(conf, file)


def check_conf(console: Console):
    # returns None when setup is missing or outdated
    conf = load_conf()
    ok = True
    if conf.get("os_name") != platform.system() or conf.get("arch") != platform.machine() or conf.get("shaderc_url_prefix") != SHADERC_URL_PREFIX:
        console.print("Error: Incompatible or outdated shaderc binary.", style="red")
        ok = False
    if conf.get("mat_src_url") != MAT_SRC_URL:
        console.print("Error: Outdated material source files.", style="red")
        ok = False
    if not (os.path.exists(SHADERC_PATH) and os.path.exists(SRC_MATERIALS_EG_PATH)):
        console.print("Error: 'setup' not done.", style="red")
        ok = False
    if not ok:
        console.print("       Please run `setup` again.", style="bold red")
        return None
    return conf
