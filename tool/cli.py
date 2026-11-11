import argparse
import build  # noqa: F401  (patches Lazurite before anything compiles)
import check
import pack
import setup


def main():
    parser = argparse.ArgumentParser(
        prog="build",
        description="Bedroalism Shader builder, compiles for Android, iOS and Windows at once (multiplatform)",
    )
    sub = parser.add_subparsers(dest="command")

    setup_p = sub.add_parser("setup", help="download shaderc and the source materials")
    setup_p.add_argument("--reset", action="store_true", help="remove all downloaded data first")
    setup_p.set_defaults(func=setup.run)

    check_p = sub.add_parser("check", help="audit the sources and test compile, no files are made")
    check_p.set_defaults(func=check.run)

    build_p = sub.add_parser("build", help="compile material.bin and pack the .mcpack")
    build_p.add_argument("--no-zip", action="store_true", help="only make the pack folder")
    build_p.set_defaults(func=pack.run)

    args = parser.parse_args()
    if args.command is None:
        parser.print_usage()
        return
    args.func(args)
