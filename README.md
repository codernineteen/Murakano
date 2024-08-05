# Murakano

Murakano is a very simple implementation of graphics application that renders OBJ model.

If you are interested in my posts about this project, you can check my [devlog](https://github.com/codernineteen/rendering-archive/tree/main/devlog).<br>
For anyone who are curious about implementation details, visualized architecture and workflows, Refer to [this repository for documentations](https://github.com/codernineteen/rendering-archive/tree/main/rendering/api/vulkan/implementation)

# Features

- Render Hardware Interface for Vulkan API
- Direct lighting
- OBJ format model rendering support
- HLSL, GLSL shader compilation support (HLSL is a default shader language)
- Vulkan Memory Allocator support
- Descriptor Manager to allocate descriptor set
- Post-processing pipeline

# Examples

### Basic .obj model rendering  ([viking room model](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38))

texture rendering without illumination           |  simple diffuse lighting
:-------------------------:|:-------------------------:
![](docs/images/render-obj.png)  |  ![](docs/images/render-diffuse-obj.png)

### Local illumination with Phong shading

| diffuse and specular reflection |
|:-------------------------:|
|![](docs/images/face_render.png)  | 

### Input control and camera movement

| Action          | Key            |
|---|---|
| Move Forward    | W              |
| Move Left       | A              |
| Move Backward   | S              |
| Move Right      | D              |
| Rotate Up       | Arrow Up       |
| Rotate Down     | Arrow Down     |
| Rotate Left     | Arrow Left     |
| Rotate Right    | Arrow Right    |

<p align="center">
<img src="docs/images/camera-moving.gif" alt="move camera perspective" />
</p>

# Dependencies

- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
- [glfw](https://github.com/glfw/glfw/tree/3.3-stable)
- [DirectXMath](https://github.com/microsoft/DirectXMath)
- [DirectXShaderCompiler](https://github.com/microsoft/DirectXMath)
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
- [stb_image](https://github.com/nothings/stb)
- [fmt](https://github.com/fmtlib/fmt)

# Acknowledgments

- This [cmakeSetup](https://github.com/meemknight/cmakeSetup) repository helped me a lot to setup this project at initial phase
