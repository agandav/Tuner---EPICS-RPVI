"""
Patch Audio library SD files to compile without SD library
This script runs after library installation to disable SD-dependent code
"""
Import("env")
import os
import shutil

def patch_audio_library(source, target, env):
    """Patch Audio library SD files to be NO_SD_CARD compatible"""
    
    # Find Audio library directory
    audio_lib_dir = None
    for lib_dir in env.get("CPPPATH", []):
        if "Audio" in lib_dir and "libdeps" in lib_dir:
            audio_lib_dir = os.path.dirname(lib_dir)
            break
    
    if not audio_lib_dir or not os.path.exists(audio_lib_dir):
        print("[PATCH] Audio library not found, skipping patch")
        return
    
    print(f"[PATCH] Found Audio library at: {audio_lib_dir}")
    
    # Files to patch (wrap entire content in #ifndef NO_SD_CARD)
    sd_files = [
        "play_sd_raw.cpp",
        "play_sd_wav.cpp",
        "record_queue.cpp"
    ]
    
    for filename in sd_files:
        filepath = os.path.join(audio_lib_dir, filename)
        
        if not os.path.exists(filepath):
            continue
        
        # Check if already patched
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        if "#ifndef NO_SD_CARD" in content:
            print(f"[PATCH] {filename} already patched, skipping")
            continue
        
        # Create backup
        backup_path = filepath + ".original"
        if not os.path.exists(backup_path):
            shutil.copy2(filepath, backup_path)
            print(f"[PATCH] Backed up {filename}")
        
        # Wrap content with NO_SD_CARD guard
        patched_content = f"""// Patched by patch_audio_lib.py to support NO_SD_CARD builds
#ifndef NO_SD_CARD

{content}

#endif // NO_SD_CARD
"""
        
        # Write patched file
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(patched_content)
        
        print(f"[PATCH] ✓ Patched {filename}")
    
    print("[PATCH] Audio library patching complete")

# Run patch after library installation but before compilation
env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", patch_audio_library)
