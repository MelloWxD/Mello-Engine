// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>
#pragma once
#include<stdio.h>
#include<cmath>
constexpr const char* CONST_MELLO_ENGINE_VER = "Mello Engine 0.1a";

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec4.hpp>

// Macros
using v3 = glm::vec3;
using v4 = glm::vec4;
using m4 = glm::mat4;
using m3 = glm::mat3;

struct Transform
{
    v3 position = v3(0);
    glm::quat rotation = { 0, 0, 0, 0 };
    v3 scale = v3(1);


};

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			printf("Detected Vulkan error: %d\n", err);				\
			abort();                                                \
		}                                                           \
	} while (0)
// End Macros

// main reusable types here
struct AllocatedImage {
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex
{
    v3 position;
    float uv_x;
    v3 normal;
    float uv_y;
    v4 color;
};

struct DirectionalLight
{
    //Sun / Directional Light
    v4 sDir = v4(1, 1, -1, 1); // Sun direction xyz for direction, w for power
    v4 sClr = v4(1); // Sun Colour RGB;
};

struct PointLight
{
    v4 Position = v4(0);
    v4 Colour = v4(1,0,0,1); // xyz for rgb, w for Diffuse power
    v4 Ambient_Colour = v4(1,0,0,1); // xyz for rgb, w for Diffuse power
    

};
struct SpotLight
{
    v4 Position = v4(0,0,0,1); // w for enabled
    v4 Direction = v4(0.f,0.f,0.f, 12.5f); // w for cut off angle /radius
};
constexpr uint8_t MAX_POINT_LIGHTS = 8;
struct GPUSceneData 
{
    m4 view;
    m4 proj;
    m4 viewproj;

    DirectionalLight Sun;
  
    PointLight pLights[MAX_POINT_LIGHTS];

    SpotLight Torch;
    
};
enum class MaterialPass :uint8_t {
    MainColor,
    Transparent,
    Other
};
struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};


struct DrawContext;
// base class for a renderable dynamic object
class IRenderable {

    virtual void Draw(const m4& topMatrix, DrawContext& ctx) = 0;
};
// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them

struct Node : public IRenderable {

    // parent pointer must be a weak pointer to avoid circular dependencies
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    m4 Translation;
    m4 Rotation;
    m4 Scale;
    m4 mMat = Translation * Rotation * Scale;
    v3 vTrans = v3(0);
    glm::quat qRot = { 0, 0, 0, 0 };
    v3 vScale = v3(1);

    m4 localTransform;
    m4 worldTransform;

    //glm::translate();
    
    void refreshTransform(const m4& parentMatrix)
    {
        worldTransform = parentMatrix * localTransform;
        for (auto c : children) {
            c->refreshTransform(worldTransform);
        }
    }

    virtual void Draw(const m4& topMatrix, DrawContext& ctx)
    {
        // draw children
        for (auto& c : children) {
            c->Draw(topMatrix, ctx);
        }
    }
};
// holds the resources needed for a mesh
struct GPUMeshBuffers
{
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants 
{
    m4 worldMatrix; // modelMatrix not world
    VkDeviceAddress vertexBuffer;
};

struct GLTFMaterial {
    MaterialInstance data;
};

// for frustum cull
struct Bounds {
    v3 origin;
    float sphereRadius;
    v3 extents;
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

