## Demo video:
Engine top down shooter demo video:
https://www.youtube.com/watch?v=FHfV1GQ4_JA

Compute shader demo video:
https://www.youtube.com/watch?v=B6pjqZyI28w

## How to build
The easies way to build is to use Visual Studio (open cmake)
Requires Vulkan SDK

## How to run demo projects:
Follow these steps:  
- Make sure all the executables are built (ProjectManager.exe in particular)
- Add the following line to the vulkan-engine\bin\create_symlink.bat:  
```mklink /D %~dp0\assets %~dp0\..\assets\assets.cache```  
This step optional but must be done to avoid running ProjectManager.exe as administrator.
- Execute the batch file ```vulkan-engine\bin\create_symlink.bat```. It should create two symlinks in the vulkan-engine\bin folder: shaders and assets. Asset may not work yet, it's fine.
- Run ProjectManager project. If you skipped the editing of .bat file the ProjectManager must be run as an administrator in order to create the symlink.
- In the ProjectManager select Project->Open and select vulkan-engine\assets\assets
- Select Assets -> Export pending assets to trigger the asset build.

It should produce ```vulkan-engine\assets\assets.cache``` folder with engine-readable assets in it.

Check the assets symlink - it should be pointing to vulkan-engine\assets\assets.cache folder.  
And the shaders symlink must be pointing to vulkan-engine\engine\shaders

Now demo projects should run without errors

## Engine roadmap:
+- top down shooter demo
- Proper deletion of entities loaded from resource
- export default materials from fbx (with texture paths)
- physics rendering interpolation
- Fix logging, log from multiple threads
- Fix and migrate lighting, decals, shadowmaps to ECS
- Handles for all engine resources
- Use pipelines directly
- Shader reloading
- Multithreaded rendering
- GUI library integration

-- Completed -----------------------------------
+ bloom
+ Refactored resource and constants binding
+ bone attachments
+ fix deadlock: resource loading exception during main thread resolve()
+ properly handle missing textures
+ properly export animations in fbx exporter
+ asset database and UI
+ export textures
+ DDS loading
+ optimize tiled lighting or implement clustered
+ Image based lighting support
+ additional parameters for resource handles (e.g. texture srgb)
+ HLSL includes must be included in hash
+ FBX mesh export
+ fix normal mapping
+ New skinning animation system
+ Proper alignment of ECS components
+ Action puzzle demo
+ Add support for singleton components (store in EntityManager by pointer)
+ Material resource
+ Multithreading, job system
+ Async resource loading
+ Fix release build settings in cmake
+ lambda API for ECS
+ PhysX integration 
+ Rendering of MultiMesh
+ Extract assets into a private repo
+ FBX physics mesh export
+ GameObject file format
