"""mats: compiles the materials to .material.bin. Stops with an error code on the first error or warning."""
import os
from importlib import import_module

from lazurite.compiler.macro_define import MacroDefine
from rich.console import Console

from util import (BUILD_DIR, MATERIALS_SRC, SHADERC_PATH, check_setup, material_names, material_paths,
                  print_styled_error)

console = Console()
project = import_module("lazurite.project.project")


def _log(message):
    """Lazurite reports every material it compiles. A warning is treated as a failure."""
    if message.startswith("Warning"):
        console.print(message, style="bold red", markup=False, highlight=False)
        exit(1)
    console.print("  " + message, style="cyan", markup=False, highlight=False)


project.print = _log


def compile_materials(profile: str, output: str, names=None, defines=""):
    """Compiles the given materials (all when None) for a profile into the output folder."""
    names = names or material_names()
    try:
        patterns = material_paths(names)
    except FileNotFoundError as error:
        console.print(f"Error: material '{error.args[0]}' does not exist in the project.", style="bold red")
        exit(1)

    os.makedirs(output, exist_ok=True)
    try:
        project.compile(
            project_path=MATERIALS_SRC,
            profiles=[profile],
            output_folder=output,
            material_patterns=patterns,
            shaderc_path=SHADERC_PATH,
            defines=[MacroDefine.from_string(defines)] if defines else [],
        )
    except Exception as error:
        print_styled_error(console, str(error.args[0]) if error.args else str(error))
        exit(1)


def run(args):
    if not check_setup(console):
        exit(1)
    output = os.path.join(BUILD_DIR, args.p)
    console.print(f"[bold]Compiling materials[/] for [cyan]{args.p}[/]")
    compile_materials(args.p, output, args.m, args.s)
    console.print("[bold green]All materials compiled.")
