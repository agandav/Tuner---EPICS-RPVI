Import("env")
import os

def skip_sd_files(node):
    """Skip SD card related source files from Audio library"""
    filepath = str(node.get_path())
    
    # Skip SD playback/recording files
    skip_patterns = [
        "play_sd_",
        "record_queue",
        "play_serialflash",
        "record_serialflash"
    ]
    
    for pattern in skip_patterns:
        if pattern in filepath:
            print(f"[FILTER] Skipping: {os.path.basename(filepath)}")
            return None
    
    return node

# More aggressive filtering - apply to all build nodes
env.AddBuildMiddleware(skip_sd_files, "*.cpp")
env.AddBuildMiddleware(skip_sd_files, "*.c")
env.AddBuildMiddleware(skip_sd_files, "*.cc")

# Also filter the source files list before compilation
def filter_lib_sources(target, source, env):
    """Remove SD files from library source lists"""
    try:
        # Get all library dependencies
        lib_deps = env.GetProjectOption("lib_deps", [])
        for lib_builder in env.GetLibBuilders():
            lib_name = lib_builder.name
            if "Audio" in lib_name:
                # Filter source files
                original_sources = lib_builder.src_files[:]
                filtered_sources = []
                
                for src in original_sources:
                    src_path = str(src)
                    skip = any(pattern in src_path for pattern in [
                        "play_sd_", "record_queue", "play_serialflash", "record_serialflash"
                    ])
                    
                    if skip:
                        print(f"[LIB_FILTER] Excluding: {os.path.basename(src_path)}")
                    else:
                        filtered_sources.append(src)
                
                lib_builder.src_files = filtered_sources
                print(f"[LIB_FILTER] Audio library: {len(original_sources)} -> {len(filtered_sources)} files")
    except Exception as e:
        print(f"[LIB_FILTER] Warning: {e}")

# Run the library filter after library scanning
env.AddPreAction("checkprogsize", filter_lib_sources)

print("[FILTER] Audio library SD file filter installed")
