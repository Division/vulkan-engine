Roadmap:
+- optimize tiled lighting or implement clustered
- Fix and migrate lighting, decals, shadowmaps to ECS
- HLSL includes must be included in hash
- PhysX integration 
- Image based lighting support
- Handles for all engine resources
- Fix release build settings in cmake
- FBX mesh export
- New skinning animation system
- Use pipelines directly
- Shader reloading
- dds loading
- Multithreaded rendering
- GameObject file format and editor
- lambda API for ECS
- GUI library integration

Check:
- Shader compiles / loads every time?
- postprocess, changed HLSL texture binding but C++ side didn't see it changed

-- Completed -----------------------------------

+ Multithreading, job system
+ Async resource loading