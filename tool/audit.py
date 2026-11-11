import glob
import json
import os
import re
from rich.console import Console
from util import PROJECT_PATH, SRC_PATH

FUNC_DIR = os.path.join(SRC_PATH, "bedroalism", "functions")
CONFIG = os.path.join(SRC_PATH, "bedroalism", "config.h")
FUNC_DEF = re.compile(r"^(?:float|int|bool|void|vec[234]|mat[234]|br\w+)\s+(br\w+)\s*\(", re.M)
REQUIRED_FILES = ["material.json", "varying.def.sc", "vertex.sc", "fragment.sc"]


def _read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def _strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def run(console: Console) -> list:
    """Source level audit: every function, define and file must be defined, included and used.
    Returns the list of problems, empty when all is consistent."""
    errors = []
    files = glob.glob(os.path.join(SRC_PATH, "**", "*.h"), recursive=True) + \
        glob.glob(os.path.join(SRC_PATH, "**", "*.sc"), recursive=True)
    text = {f: _strip_comments(_read(f)) for f in files}
    headers = sorted(glob.glob(os.path.join(FUNC_DIR, "*.h")))

    # functions: defined once, called somewhere else than their own definition
    defined = {}
    for h in headers:
        for name in FUNC_DEF.findall(text[h]):
            if name in defined:
                errors.append(f"function {name} is defined twice ({defined[name]} and {h})")
            defined[name] = h
    for name, h in sorted(defined.items()):
        calls = sum(len(re.findall(r"\b" + name + r"\s*\(", t)) for t in text.values()) - 1
        if calls <= 0:
            errors.append(f"function {name} in {os.path.relpath(h, SRC_PATH)} is never called")

    # config: every define is used, every BR_ name that is used is defined somewhere
    config_names = set(re.findall(r"#define\s+(BR_\w+)", text[CONFIG]))
    all_defined = set(config_names)
    for t in text.values():
        all_defined |= set(re.findall(r"#define\s+(BR_\w+)", t))
    for name in sorted(config_names):
        if name.endswith("_H"):
            continue
        used = any(re.search(r"\b" + name + r"\b", t) for f, t in text.items() if f != CONFIG)
        if not used:
            errors.append(f"config {name} is never used")
    for f, t in text.items():
        for name in set(re.findall(r"\b(BR_[A-Z0-9_]+)\b", t)):
            if name not in all_defined:
                errors.append(f"{os.path.relpath(f, SRC_PATH)} uses {name} but it is not defined")

    # includes resolve, and every function header is included by something
    included = set()
    for f, t in text.items():
        for inc in re.findall(r'#include\s*[<"]([^>"]+)[>"]', t):
            if inc.startswith("bedroalism/"):
                path = os.path.join(SRC_PATH, inc)
            elif inc.endswith(".h") and os.path.exists(os.path.join(os.path.dirname(f), inc)):
                path = os.path.join(os.path.dirname(f), inc)
            else:
                continue  # bgfx and game headers come from include/ and the material sources
            if not os.path.exists(path):
                errors.append(f"{os.path.relpath(f, SRC_PATH)} includes {inc} which does not exist")
            included.add(os.path.normpath(path))
    for h in headers:
        if os.path.normpath(h) not in included:
            errors.append(f"header {os.path.relpath(h, SRC_PATH)} is not included anywhere")

    # HLSL has no vecN(scalar), use vecN_splat(scalar), GLSL and Metal accept it so only Windows would fail
    for f, t in text.items():
        for m in re.finditer(r"\bvec[234]\s*\(", t):
            depth, i, comma = 1, m.end(), False
            while depth and i < len(t):
                depth += (t[i] == "(") - (t[i] == ")")
                comma = comma or (t[i] == "," and depth == 1)
                i += 1
            if not comma:
                line = t.count("\n", 0, m.start()) + 1
                errors.append(f"{os.path.relpath(f, SRC_PATH)}:{line} {t[m.start():i]} has one argument, use the _splat version (HLSL)")

    # materials: every folder is listed in project.json and complete
    with open(os.path.join(PROJECT_PATH, "project.json")) as f:
        listed = json.load(f)["base_profile"]["include_patterns"]
    folders = sorted(d for d in os.listdir(PROJECT_PATH) if os.path.isdir(os.path.join(PROJECT_PATH, d)))
    for d in folders:
        if d not in listed:
            errors.append(f"material {d} is not listed in project.json, it would not be built")
        for req in REQUIRED_FILES:
            if not os.path.exists(os.path.join(PROJECT_PATH, d, req)):
                errors.append(f"material {d} has no {req}")
    for d in listed:
        if d not in folders:
            errors.append(f"project.json lists {d} but the folder does not exist")

    console.print(f"  [dim]functions :[/] {len(defined)} defined, {len(defined) - sum(1 for e in errors if e.startswith('function') and 'never called' in e)} called")
    console.print(f"  [dim]config    :[/] {len(config_names)} defines")
    console.print(f"  [dim]materials :[/] {len(folders)} folders, {len(listed)} listed")
    return errors


def materials() -> list:
    with open(os.path.join(PROJECT_PATH, "project.json")) as f:
        return sorted(json.load(f)["base_profile"]["include_patterns"])
