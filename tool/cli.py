"""Command line of the build tool."""
import argparse

import build
import pack
import setup
from util import PROFILES, default_profile


def main():
    parser = argparse.ArgumentParser(
        prog="build",
        description="Bedroalism Shader build tool: compiles the materials with Lazurite and packs them as .mcpack",
    )
    commands = parser.add_subparsers(dest="command")

    setup_cmd = commands.add_parser("setup", help="download shaderc and the vanilla material definitions")
    setup_cmd.add_argument("--reset", action="store_true", help="delete the downloaded data first")
    setup_cmd.set_defaults(func=setup.run)

    mats_cmd = commands.add_parser("mats", help="compile the materials only, to find errors (build/<profile>)")
    mats_cmd.add_argument("-m", nargs="+", default=[], help="materials to compile, all by default (eg: Sky)")
    mats_cmd.add_argument("-s", default="", help="macro to define while compiling")
    mats_cmd.set_defaults(func=build.run)

    pack_cmd = commands.add_parser("pack", help="compile the materials and build the .mcpack")
    pack_cmd.add_argument("--no-archive", action="store_true", help="leave the pack as a folder")
    pack_cmd.set_defaults(func=pack.run)

    for command in (mats_cmd, pack_cmd):
        command.add_argument("-p", choices=PROFILES, default=default_profile(),
                             help="target: multiplatform is Android, iOS and Windows in one pack")

    args = parser.parse_args()
    if args.command:
        args.func(args)
    else:
        parser.print_usage()
