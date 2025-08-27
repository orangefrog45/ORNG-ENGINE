## ORNG Engine

This is a real-time windows-based 3D game engine, made with C++ and OpenGL, my first graphics/game engine project.

### Engine preview

![Engine screenshot](EngineHouseDay.jpg)
![Engine screenshot](EngineHouseNight.jpg)
![Engine screenshot](FractalEditor.jpg)

### Building:

Cloning this repo recursively and opening the folder with an IDE that has CMake and Vcpkg integration should allow you to build and run the core library, runtime and editor.
To create game builds with the editor, the runtime must already be built.

For scripts to be able to locate the correct binaries, your build paths must be "ORNG_ROOT/out/build/debug" and "ORNG_ROOT/out/build/release.", where ORNG_ROOT is this directory.

### Controls:

- Move camera: Hold right mouse, WASD + mouse controls
- Close window/delete component: double right click on header/title bar
- Drag and drop assets
- Create entity: right click on "Scene graph" window or drag/drop mesh asset in scene view panel
- Duplicate selected entities: Ctrl+D
- Make editor cam active: K
- Ctrl+z/Ctrl+shift+z undo/redo

### Currently, features include:

- Visual editor built with ImGui, with a state-preserving simulation mode and Lua console
- Deferred physically-based 3D renderer
- C++ scripting with hot reloading
- VR rendering and interaction support via OpenXR in the editor and runtime
- Editor project serialization and full game runtime generation
- Cascaded voxel cone traced global illumination
- GPU-driven particle system
- Volumetric fog
- Stable cascaded shadow maps with PCSS
- ECS integration using entt library
- HDRI environment map support
- Normal, parallax and emissive map support
- Mesh/texture file loading and binary serialization/deserialization
- YAML serialization of scenes
- Postprocessing such as HDR, gamma correction and bloom
- Point/spot/directional lights with shadows
- Auto-instancing of meshes
- Event-driven architecture
