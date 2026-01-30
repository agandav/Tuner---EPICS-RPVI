Import("env")
import os

def skip_sd_files(node):
    """Skip SD card related source files from Audio library"""
    filepath = node.get_path()
    
    # Skip SD playback/recording files
    if any(pattern in filepath for pattern in [
        "play_sd_",
        "record_queue",
        "play_serialflash",
        "record_serialflash"
    ]):
        return None
    
    return node

# Filter library source files
env.AddBuildMiddleware(skip_sd_files, "*.cpp")
env.AddBuildMiddleware(skip_sd_files, "*.c")

print("Audio library SD file filter installed")
