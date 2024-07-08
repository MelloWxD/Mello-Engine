@echo off
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/gradient_color.comp -o F:/MelloEngine/shaders/gradient.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/sky.comp -o F:/MelloEngine/shaders/sky.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/colored_triangle.vert -o F:/MelloEngine/shaders/colored_triangleVS.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/colored_triangle.frag -o F:/MelloEngine/shaders/colored_trianglePS.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/colored_triangle_mesh.vert -o F:/MelloEngine/shaders/colored_triangle_meshVS.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/tex_image.frag -o F:/MelloEngine/shaders/tex_image.spv

C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/mesh.vert -o F:/MelloEngine/shaders/mesh.vert.spv
C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/mesh.frag -o F:/MelloEngine/shaders/mesh.frag.spv

C:/VulkanSDK/1.3.280.0/Bin/glslc.exe F:/MelloEngine/shaders/mesh_pbr.frag -o F:/MelloEngine/shaders/meshPBR.frag.spv


echo "Compiled Shaders"

