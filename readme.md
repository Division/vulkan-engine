Roadmap:
-+ optimize tiled lighting or implement clustered
- Fix logging, log from multiple threads
- Fix and migrate lighting, decals, shadowmaps to ECS
- HLSL includes must be included in hash
- Image based lighting support
- Handles for all engine resources
- FBX mesh export
- New skinning animation system
- Use pipelines directly
- Shader reloading
- dds loading
- Multithreaded rendering
- GameObject file format and editor
- GUI library integration

Check:
- Shader compiles / loads every time?
- postprocess, changed HLSL texture binding but C++ side didn't see it changed

-- Completed -----------------------------------

+ Multithreading, job system
+ Async resource loading
+ Fix release build settings in cmake
+ lambda API for ECS
+ PhysX integration 
+ Rendering of MultiMesh