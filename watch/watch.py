# Quick thing just for ease of development, automatically updates shaders in build dirs without having to rebuild the whole thing
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import shutil

debug_dir_core = "..\\out\\build\\x64-Debug\\ORNG-Core\\res\\shaders\\"
release_dir_core = "..\\out\\build\\x64-Release-2\\ORNG-Core\\res\\shaders\\"
debug_dir_editor = "..\\out\\build\\x64-Debug\\ORNG-Editor\\res\\shaders\\"
release_dir_editor = "..\\out\\build\\x64-Release-2\\ORNG-Editor\\res\\shaders\\"
debug_dir_game = "..\\out\\build\\x64-Debug\\Game\\res\\shaders\\"
release_dir_game = "..\\out\\build\\x64-Release-2\\Game\\res\\shaders\\"
debug_dir_monitor = "..\\out\\build\\x64-Debug-2\\HW-Monitor\\res\\shaders\\"
release_dir_monitor = "..\\out\\build\\x64-Release-2\\HW-Monitor\\res\\shaders\\"

def TryCopy(original, copy_location):
    try:
        shutil.copy(original, copy_location);
    except:
        return

def ConstructPath(path : str, is_core):
    if is_core:
        path = path.replace("res\\", "res\\core-res\\")
    return path

def IsShaderFile(filepath : str) -> bool:
    return filepath.find(".glsl") != -1 or filepath.find(".vert") != -1 or filepath.find(".frag") != -1

class Handler(FileSystemEventHandler):
    def on_modified(self, event):
        # Function to execute when a file is modified
        filepath : str = event.src_path;

        if filepath.find("res\\shaders") == -1 or filepath.find("out\\") != -1 or filepath.find("build\\") != -1 or not IsShaderFile(filepath) or filepath.find("core-res") != -1:
            return;

        is_core = filepath.find("ORNG-Core") != -1

        print(f'File {event.src_path} has been modified.')
        filename = filepath.split("\\").pop()
        TryCopy(event.src_path, ConstructPath(debug_dir_core + filename, is_core));
        TryCopy(event.src_path, ConstructPath(release_dir_core + filename, is_core));
        TryCopy(event.src_path, ConstructPath(debug_dir_editor + filename, is_core));
        TryCopy(event.src_path, ConstructPath(release_dir_editor + filename, is_core));
        TryCopy(event.src_path, ConstructPath(debug_dir_game + filename, is_core));
        TryCopy(event.src_path, ConstructPath(release_dir_game + filename, is_core));
        TryCopy(event.src_path, ConstructPath(debug_dir_monitor + filename, is_core));
        TryCopy(event.src_path, ConstructPath(release_dir_monitor + filename, is_core));

if __name__ == "__main__":
    path = "..\\" 
    event_handler = Handler()
    observer = Observer()
    observer.schedule(event_handler, path, recursive=True)
    observer.start()

    try:
        while True:
            pass
    except KeyboardInterrupt:
        observer.stop()
    observer.join()