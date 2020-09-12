Game roadmap:
- Add basic vehicle
- Add basic spherical world
- Import vehicle model and setup physics (ignore vehicle shape for now)

Engine roadmap:
- [partly done] optimize tiled lighting or implement clustered
- [partly done] FBX mesh export
- Add support for singleton components (store in EntityManager by pointer)
- FBX physics mesh export
- Material resource
- physics rendering interpolation
- Fix logging, log from multiple threads
- Fix and migrate lighting, decals, shadowmaps to ECS
- HLSL includes must be included in hash
- Image based lighting support
- Handles for all engine resources
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
+ Extract assets into a private repo