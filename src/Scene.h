#include<vk_types.h>
#include<vector>
#include<vk_loader.h>

class VulkanEngine;
class GameObject;
class Camera;

class Scene
{
public:
	Scene();
	~Scene();

	void init(VulkanEngine* engine, const char* sceneJsonPath);
	// getters & setters
	GPUSceneData& getSceneData()
	{
		return _SceneGPUData;
	};
	void setSceneData(GPUSceneData data)
	{
		_SceneGPUData = data;
	};	
	ComputePushConstants& getComputeConstants()
	{
		return _ComputeData;
	};
	void setComputeConstants(ComputePushConstants data)
	{
		_ComputeData = data;
	};
	std::vector<GameObject>& getSceneGameObjects()
	{
		return _SceneObjects;
	};
	void setSceneGameObjects(std::vector<GameObject> data)
	{
		_SceneObjects = data;
	};

	void Update(Camera& cam);
private:
	GPUSceneData _SceneGPUData;
	ComputePushConstants _ComputeData;
	std::vector<GameObject> _SceneObjects;
	VulkanEngine* _pEngine;
};