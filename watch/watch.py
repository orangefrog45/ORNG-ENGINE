# Quick thing just for ease of development, automatically updates shaders in build dirs without having to rebuild the whole thing
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
import shutil
import time

debug_dir_core = "..\\out\\debug\\ORNG-Core\\res\\shaders\\"
release_dir_core = "..\\out\\release\\ORNG-Core\\res\\shaders\\"
debug_dir_editor = "..\\out\\debug\\ORNG-Editor\\res\\shaders\\"
release_dir_editor = "..\\out\\release\\ORNG-Editor\\res\\shaders\\"
debug_dir_game = "..\\out\\debug\\Game\\res\\shaders\\"
release_dir_game = "..\\out\\release\\Game\\res\\shaders\\"
debug_dir_monitor = "..\\out\\debug\\HW-Monitor\\res\\shaders\\"
release_dir_monitor = "..\\out\\release\\HW-Monitor\\res\\shaders\\"
debug_dir_ifs = "..\\out\\debug\\IFS-fractals\\res\\shaders\\"
release_dir_ifs = "..\\out\\release\\IFS-fractals\\res\\shaders\\"
debug_dir_phys_game = "..\\out\\debug\\PhysicsGame\\res\\shaders\\"
release_dir_phys_game = "..\\out\\release\\PhysicsGame\\res\\shaders\\"

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
    valid_shader_extensions = [".glsl", ".vert", ".frag", ".compute", ".tess", ".comp"]
    for extension in valid_shader_extensions:
        if filepath.find(extension) != -1:
            return True
        
    return False

class Handler(FileSystemEventHandler):
    def on_modified(self, event):
        # Function to execute when a file is modified
        filepath : str = event.src_path;

        if filepath.find("res\\shaders") == -1 or filepath.find("cmake-build") != -1 or filepath.find("out") != -1 or not IsShaderFile(filepath) or filepath.find("core-res") != -1:
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
        TryCopy(event.src_path, ConstructPath(debug_dir_ifs + filename, is_core));
        TryCopy(event.src_path, ConstructPath(release_dir_ifs + filename, is_core));
        TryCopy(event.src_path, ConstructPath(debug_dir_phys_game + filename, is_core));
        TryCopy(event.src_path, ConstructPath(release_dir_phys_game + filename, is_core));

if __name__ == "__main__":
    path = "..\\" 
    event_handler = Handler()
    observer = Observer()
    observer.schedule(event_handler, path, recursive=True)
    observer.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()