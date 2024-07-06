#pragma once

#include <vk_types.h>
#include <unordered_map>
#include <vk_descriptors.h>
#include <vk_images.h>
#include <filesystem>

class VulkanEngine;
struct LoadedGLTF : public IRenderable {

    // storage for all the data on a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    // nodes that dont have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;
    std::string Name;
    VulkanEngine* creator;

    ~LoadedGLTF() { clearAll(); };

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:

    void clearAll();
};


std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine, std::string_view filePath);

void read_assetDataJson(VulkanEngine* engine, const char* path);

void read_sceneJson(VulkanEngine* engine, const char* path);

void read_hitboxJson(VulkanEngine* engine, const char* path);

bool write_sceneJson(VulkanEngine* engine, std::string path);

bool write_hitboxJson(VulkanEngine* engine, std::string path);

