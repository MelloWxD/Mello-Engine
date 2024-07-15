#include<vk_types.h>
#include<vk_loader.h>
#include<GameObject.h>
#include<vector>


class Scene
{
public:
	Scene();
	~Scene();

	ComputePushConstants& getPushConstants();
	void setPushConstants(ComputePushConstants data);
	
	
	GPUSceneData& getShaderData();
	
	void setShaderData(GPUSceneData data);
	

	std::vector<GameObject>& getSceneObjects();
	void setSceneObjects(std::vector<GameObject> data);
	



private:
	std::vector<GameObject> _vSceneObjs; // Game Objs in the scene
	GPUSceneData _sceneShaderData;
	ComputePushConstants _sceneComputeData;
};

