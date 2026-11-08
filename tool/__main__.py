"""Entry point: python tool <command>. Checks the dependencies of requirements.txt first."""
import importlib.metadata
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def check_requirements():
    """Warns when an installed package is older than the one the build was made with."""
    from packaging.version import parse
    with open("requirements.txt", "r", encoding="utf-8") as file:
        for line in file:
            line = line.strip().replace(" ", "")
            for sep in ("==", ">="):
                if sep not in line:
                    continue
                name, wanted = line.split(sep)
                found = importlib.metadata.version(name)
                too_old = parse(found) < parse(wanted)
                differs = sep == "==" and parse(found) != parse(wanted)
                if too_old or differs:
                    print(f'Warning: "{name}" {found} is installed, requirements.txt asks for {sep}{wanted}')
                break


if __name__ == "__main__":
    try:
        check_requirements()
        from cli import main
        main()
    except (importlib.metadata.PackageNotFoundError, ModuleNotFoundError) as error:
        print(f'Module "{getattr(error, "name", error)}" not found.')
        print("\nDid you run `python -m pip install -r requirements.txt`?\n")
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(1)
