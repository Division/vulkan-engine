Engine roadmap:
- bone attachments
- top down shooter demo
- Proper deletion of entities loaded from resource
- export default materials from fbx (with texture paths)
- export textures from fbx
- properly export animations in fbx exporter
- physics rendering interpolation
- Fix logging, log from multiple threads
- Fix and migrate lighting, decals, shadowmaps to ECS
- Handles for all engine resources
- Use pipelines directly
- Shader reloading
- Multithreaded rendering
- GUI library integration
- effects for action-puzzle demo

Check:
- postprocess, changed HLSL texture binding but C++ side didn't see it changed

-- Completed -----------------------------------
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
