import os
import shutil
from rich.console import Console
from util import BUILD_PATH, CAN_BUILD_ALL, PROFILE
import audit
import build

console = Console()


def run(args):
    """Build check: audit the sources and compile every material for all platforms.
    Nothing is packed, the compiled files are removed afterwards."""
    if not build.ready():
        exit(1)

    console.print("~ Audit", style="bold")
    problems = audit.run(console)

    # the Windows shaderc is needed for HLSL, elsewhere Android and iOS are checked on their own
    profiles = [PROFILE] if CAN_BUILD_ALL else ["android", "ios"]
    console.print(f"\n~ Compile ({', '.join(profiles)})", style="bold")
    if not CAN_BUILD_ALL:
        console.print("  Windows (HLSL) cannot be compiled on this system, it is only checked on Windows.", style="yellow")
    for profile in profiles:
        out = os.path.join(BUILD_PATH, "check-" + profile)
        if build.compile_materials(out, profile):
            problems += build.verify(out)
            console.print(f"  [dim]{profile:<14}:[/] {len(audit.materials())} materials compiled")
        else:
            problems.append(f"compile failed for {profile}")
        shutil.rmtree(out, ignore_errors=True)

    if problems:
        console.print("\n~ Problems", style="bold red")
        for p in problems:
            console.print("  " + p, style="red")
        exit(1)
    console.print("\n[bold green]Ready to build, no problems found." + ("" if CAN_BUILD_ALL else " (Windows not checked)"))
