"""
Pre-build script: patch esp32_https_server so it compiles with newer ESP32 Arduino core.

The library uses #include <hwcrypto/sha.h>, which was moved to <esp32/sha.h> in newer
ESP-IDF / Arduino ESP32 core. This script replaces the include in the library source.
See: https://github.com/fhessel/esp32_https_server/issues/143
"""
import os
import glob

Import("env")

PROJECT_DIR = env.get("PROJECT_DIR", "")
PIOENV = env.get("PIOENV", "esp32cam")
LIBDEPS_DIR = os.path.join(PROJECT_DIR, ".pio", "libdeps", PIOENV)

def patch_hwcrypto_in_file(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()
    except Exception as e:
        print("fix_https_server_sha: could not read %s: %s" % (path, e))
        return False
    if "hwcrypto/sha.h" not in content:
        return False
    new_content = content.replace("#include <hwcrypto/sha.h>", "#include <esp32/sha.h>")
    if new_content == content:
        return False
    try:
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(new_content)
    except Exception as e:
        print("fix_https_server_sha: could not write %s: %s" % (path, e))
        return False
    print("fix_https_server_sha: patched %s (hwcrypto/sha.h -> esp32/sha.h)" % path)
    return True

def run_patch(source, target, env):
    if not os.path.isdir(LIBDEPS_DIR):
        return
    # Find esp32_https_server (folder name may vary)
    for name in os.listdir(LIBDEPS_DIR):
        lib_path = os.path.join(LIBDEPS_DIR, name)
        if not os.path.isdir(lib_path):
            continue
        if "esp32" in name.lower() and "https" in name.lower() and "server" in name.lower():
            for ext in ("*.hpp", "*.cpp", "*.h"):
                for path in glob.glob(os.path.join(lib_path, "src", ext)):
                    patch_hwcrypto_in_file(path)
                for path in glob.glob(os.path.join(lib_path, ext)):
                    patch_hwcrypto_in_file(path)
            break

env.AddPreAction("buildprog", run_patch)
