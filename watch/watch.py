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
debug_dir_net_game = "..\\out\\build\\x64-Debug\\NetGame\\res\\shaders\\"
release_dir_net_game = "..\\out\\build\\x64-Release-2\\NetGame\\res\\shaders\\"


class Handler(FileSystemEventHandler):
    def on_modified(self, event):
        # Function to execute when a file is modified
        filename : str = event.src_path;

        if filename.find("res\\shaders") == -1 or filename.find("out\\") != -1 or filename.find("build\\") != -1 or filename.find(".glsl") == -1 or filename.find("core-res") != -1:
            return;

        debug_path = ""
        release_path = ""

        if filename.find("ORNG-Core") != -1:
            debug_path = debug_dir_core
            release_path= release_dir_core

            
        if filename.find("ORNG-Editor") != -1:
            debug_path = debug_dir_editor
            release_path= release_dir_editor

        if filename.find("Game") != -1 and filename.find("Net") == -1:
            debug_path = debug_dir_game
            release_path= release_dir_game

        if filename.find("Net") != -1:
            debug_path = debug_dir_net_game
            release_path= release_dir_net_game

        print(f'File {event.src_path} has been modified.')
        filename = filename.split("\\").pop()
        shutil.copy(event.src_path, debug_dir_core + filename);
        shutil.copy(event.src_path, release_dir_core+ filename);
        shutil.copy(event.src_path, debug_dir_editor + filename);
        shutil.copy(event.src_path, release_dir_editor + filename);
        shutil.copy(event.src_path, debug_dir_game+ filename);
        shutil.copy(event.src_path, release_dir_game+ filename);
       # shutil.copy(event.src_path, debug_dir_net_game+ filename);
        shutil.copy(event.src_path, release_dir_net_game+ filename);

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