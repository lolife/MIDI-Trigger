from pathlib import Path

Import("env")


def remove_appledouble_files(base: Path) -> int:
    removed = 0
    if not base.exists():
        return removed

    for path in base.rglob("._*"):
        if path.is_file():
            path.unlink()
            removed += 1

    return removed


project_dir = Path(env["PROJECT_DIR"])
pio_dir = project_dir / ".pio"
removed_count = remove_appledouble_files(pio_dir)

if removed_count:
    print(f"Removed {removed_count} macOS AppleDouble file(s) from {pio_dir}")
