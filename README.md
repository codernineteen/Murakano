# Murakano

Murakano is a very simple implementation of graphics application that renders OBJ model.

If you are interested in my posts about this project, you can check my [devlog](https://github.com/codernineteen/rendering-archive/tree/main/devlog).<br>
For anyone who are curious about implementation details, visualized architecture and workflows, Refer to [this repository for documentations](https://github.com/codernineteen/rendering-archive/tree/main/rendering/api/vulkan/implementation)

# Features

- Abstractions for Vulkan API
- OBJ format model rendering support
- HLSL, GLSL shader compilation support (HLSL is a default shader language)
- Easy to use Global command service interface
- Vulkan Memory Allocator support
- Descriptor Manager to allocate descriptor set

# Examples

### Basic .obj model rendering  ([viking room model](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38))
<div style="display: flex; flex:1 1 0px;">
	<p align="center">
		<label>[texture rendering without illumination]</label>
		<img 
		src="docs/images/render-obj.png" 
		alt="image of rendered viking room"
		style="width: auto; height: 250px; border: 2px solid #333"
		/>
	</p>
	<p align="center">
		<label>[simple diffuse lighting]</label>
		<img 
		src="docs/images/render-diffuse-obj.png" 
		alt="image of rendered viking room"
		style="wwidth: auto; height: 250px; border: 2px solid #333"
		/>
	</p>
</div>

### Local illumination with Phong shading
<p align="center" style="display:flex; flex-direction: column; align-items: center;">
	<label>[diffuse and specular reflection]</label>
	<img 
	src="docs/images/face_render.png" 
	alt="image of rendered viking room"
	style="width: 560px; height: 360px; border: 2px solid #333"
	/>
</p>

- camera movement

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
