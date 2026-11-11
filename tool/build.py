import os
import shutil
from importlib import import_module
from rich.console import Console
from lazurite.compiler.macro_define import MacroDefine  # noqa: F401  (kept for parity with the Newb tool)
from util import print_styled_error, check_conf, SHADERC_PATH, PROJECT_PATH, PROFILE
import audit

console = Console()
status = console.status("[bold green]Building...")


def _lp_print(m):
    if m.startswith("Warning"):
        console.print(m, style="bold red")
        raise RuntimeError(m)
    status.update("[bold green]Building " + m)


lp = import_module("lazurite.project.project")
lp.print = _lp_print


def compile_materials(output_path: str, profile: str = PROFILE) -> bool:
    """Compile every material for the profile (multiplatform = Android, iOS and Windows) into output_path."""
    shutil.rmtree(output_path, ignore_errors=True)
    os.makedirs(output_path)
    with status:
        try:
            lp.compile(
                project_path=PROJECT_PATH,
                profiles=[profile],
                output_folder=output_path,
                material_patterns=[],
                shaderc_path=SHADERC_PATH,
                defines=[],
            )
        except Exception as e:
            print_styled_error(console, str(e.args[0]) if e.args else str(e))
            return False
    return True


def verify(output_path: str) -> list:
    """Every material must come out as a non empty .material.bin."""
    problems = []
    for name in audit.materials():
        path = os.path.join(output_path, name + ".material.bin")
        if not os.path.exists(path):
            problems.append(f"{name}.material.bin was not produced")
        elif os.path.getsize(path) == 0:
            problems.append(f"{name}.material.bin is empty")
    return problems


def ready() -> bool:
    return check_conf(console) is not None
