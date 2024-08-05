C:/VulkanSDK/1.3.280.0/Bin/glslc.exe passthrough.vert -o post-vert.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe post.frag -o post-frag.spv
move ./post-vert.spv C:/VulkanDev/Murakano/Shaders/Output/SPIR-V/
move ./post-frag.spv C:/VulkanDev/Murakano/Shaders/Output/SPIR-V/
pause