Import("env")

from pathlib import Path

framework_dir = env.PioPlatform().get_package_dir("framework-espidf")
if not framework_dir:
    print("[vindriktning] framework-espidf package directory not found; skipping OpenThread timestamp patch")
else:
    cmake = Path(framework_dir) / "components" / "openthread" / "CMakeLists.txt"

    if not cmake.exists():
        print(f"[vindriktning] OpenThread CMakeLists not found: {cmake}")
    else:
        text = cmake.read_text(encoding="utf-8")

        active = '"OPENTHREAD_BUILD_DATETIME=\\"${OT_BUILD_TIMESTAMP}\\""'
        patched = '# "OPENTHREAD_BUILD_DATETIME=\\"${OT_BUILD_TIMESTAMP}\\"" # PlatformIO/Windows quoting workaround'

        if patched in text:
            print("[vindriktning] OpenThread build datetime workaround already applied")
        elif active in text:
            backup = cmake.with_suffix(".txt.vindriktning-backup")
            if not backup.exists():
                backup.write_text(text, encoding="utf-8")

            text = text.replace(active, patched, 1)
            cmake.write_text(text, encoding="utf-8")
            print("[vindriktning] Applied OpenThread OPENTHREAD_BUILD_DATETIME workaround")
            print(f"[vindriktning] Backup: {backup}")
        else:
            print("[vindriktning] NOTE: expected OPENTHREAD_BUILD_DATETIME line not found; no patch applied")
