"""
PlatformIO extra script to patch Audio library for NO_SD_CARD support
This script runs before building and adds conditional compilation guards
to exclude SD card functionality when NO_SD_CARD is defined.
"""

Import("env")  # type: ignore  # Provided by PlatformIO SCons
import os

def patch_audio_library(source, target, env):
    """Patch Audio library files to support NO_SD_CARD"""
    
    # Path to Audio library
    audio_lib_path = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), "teensy41", "Audio")
    
    if not os.path.exists(audio_lib_path):
        print("Audio library not found yet, will patch on next build")
        return
    
    # Files to patch
    files_to_patch = {
        "Audio.h": {
            "search": '#include "play_memory.h"\n#include "play_queue.h"\n#include "play_sd_raw.h"\n#include "play_sd_wav.h"\n#include "play_serialflash_raw.h"',
            "replace": '#include "play_memory.h"\n#include "play_queue.h"\n#ifndef NO_SD_CARD\n#include "play_sd_raw.h"\n#include "play_sd_wav.h"\n#endif\n#include "play_serialflash_raw.h"'
        },
        "play_sd_raw.cpp": {
            "search": "#include <Arduino.h>\n#include \"play_sd_raw.h\"\n#include \"spi_interrupt.h\"",
            "replace": "#ifndef NO_SD_CARD\n\n#include <Arduino.h>\n#include \"play_sd_raw.h\"\n#include \"spi_interrupt.h\"",
            "end_search": "return ((uint64_t)file_size * B2M) >> 32;\n}",
            "end_replace": "return ((uint64_t)file_size * B2M) >> 32;\n}\n\n#endif // NO_SD_CARD"
        },
        "play_sd_wav.cpp": {
            "search": "#include <Arduino.h>\n#include \"play_sd_wav.h\"\n#include \"spi_interrupt.h\"",
            "replace": "#ifndef NO_SD_CARD\n\n#include <Arduino.h>\n#include \"play_sd_wav.h\"\n#include \"spi_interrupt.h\"",
            "end_marker": True  # Will add #endif at end of file
        }
    }
    
    for filename, patches in files_to_patch.items():
        filepath = os.path.join(audio_lib_path, filename)
        if not os.path.exists(filepath):
            continue
            
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Check if already patched
        if "#ifndef NO_SD_CARD" in content and "NO_SD_CARD" in env.get("BUILD_FLAGS", []):
            continue  # Already patched
        
        # Apply patches
        modified = False
        if "search" in patches and patches["search"] not in content:
            # Not yet patched, apply patch
            if patches["search"] in content.replace("#ifndef NO_SD_CARD\n", "").replace("#endif\n", ""):
                content = content.replace(patches["search"], patches["replace"])
                modified = True
        
        if "end_search" in patches and patches["end_search"] in content:
            content = content.replace(patches["end_search"], patches["end_replace"])
            modified = True
        elif patches.get("end_marker"):
            if not content.rstrip().endswith("#endif // NO_SD_CARD"):
                content = content.rstrip() + "\n\n#endif // NO_SD_CARD\n"
                modified = True
        
        if modified:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Patched {filename} for NO_SD_CARD support")

# Register the patch function to run before build
env.AddPreAction("$BUILD_DIR/src/main.cpp.o", patch_audio_library)

print("Audio library patch script loaded")
