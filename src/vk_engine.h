


#pragma once

#include <SDL.h>
#include <SDL_vulkan.h>
#include "VkBootstrap.h"

#include <array>
#include <chrono>
#include <iostream>
#include <fstream>


#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_images.h>
#include <vk_pipelines.h>
#include <vk_descriptors.h>
#include <glm/gtx/transform.hpp>
#include <vk_loader.h>
#include <camera.h>

#include <CollisionHandler.h>


#include <Scene.h>


struct GLTFMetallic_Roughness {
	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;

	VkDescriptorSetLayout materialLayout;

	struct MaterialConstants {
		v4 colorFactors;
		v4 metal_rough_factors;
		//padding, we need it anyway for uniform buffers
		v4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage colorImage;
		VkSampler colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler metalRoughSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};

	DescriptorWriter writer;

	void build_pipelines(VulkanEngine* engine);
	void clear_resources(VkDevice device);

	MaterialInstance write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};



class VulkanEngine {
public:

	bool _isInitialized{ false };
	bool debugwindow{ true };
	int _frameNumber {0};
	VkExtent2D _windowExtent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	void rebuild_swapchain();

	//draw loop
	void draw();

	void resize_swapchain();

	void loadSavedHitbox(GameObject* go);

	//run main loop
	void run();

	EngineStats stats;

	std::unordered_map<std::string, std::vector<std::pair<v3, v3>>> savedHitboxData; // first = extents, second = offset.

	//GLTF MATS
	MaterialInstance defaultData;
	GLTFMetallic_Roughness metalRoughMaterial;

	DrawContext mainDrawContext;

	void update_scene();
	std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedGLTFs;
	std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

	//std::vector<GameObject> _vGameObjects;

	std::vector<Scene> _Scenes;
	Scene defaultScene;


	Camera mainCamera;
	float sDTime;
	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& img);

	VkInstance _instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT _debug_messenger;// Vulkan debug output handle
	VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
	VkDevice _device; // Vulkan device for commands
	VkSurfaceKHR _surface;// Vulkan window surface

	bool stopRendering = false;
	bool resize_requested = false;

	// Swap Chain stuff
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	VkExtent2D _swapchainExtent;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;

	// Framedata & graphicsqueue stuff
	FrameData _frames[FRAME_OVERLAP];

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	void destroy_buffer(const AllocatedBuffer& buffer);


	DeletionQueue _mainDeletionQueue;

	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{ 1 };

	VmaAllocator _allocator;

	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

	VkSampler _defaultSamplerLinear;
	VkSampler _defaultSamplerNearest;


	//draw resources
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;
	VkExtent2D _drawExtent;
	float renderScale = 1.f;

	DescriptorAllocatorGrowable globalDescriptorAllocator;
	// Descriptors

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	VkDescriptorSetLayout _singleImageDescriptorLayout;


	//GPUSceneData sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

	//Pipelines
	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout = {};

	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;
	void init_triangle_pipeline();
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
	void draw_geometry(VkCommandBuffer cmd);


	//Imgui stuff
	// immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _meshPipeline;

	GPUMeshBuffers rectangle;

	void init_mesh_pipeline();



	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);

private:
	void init_vulkan();
	void init_swapchain();
	void draw_main(VkCommandBuffer cmd);
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	void init_descriptors();

	void init_background_pipelines();

	void createDebugSpheres();

	void init_default_data();

	void init_renderables();

	void init_pipelines();
	void init_imgui();

};