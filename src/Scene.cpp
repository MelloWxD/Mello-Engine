#include "Scene.h"


Scene::Scene()
{
}

Scene::~Scene()
{
}

ComputePushConstants& Scene::getPushConstants()
{
	return _sceneComputeData;
}

void Scene::setPushConstants(ComputePushConstants data)
{
	_sceneComputeData = data;
}

GPUSceneData& Scene::getShaderData()
{
	return _sceneShaderData;
}

void Scene::setShaderData(GPUSceneData data)
{
	_sceneShaderData = data;
}

std::vector<GameObject>& Scene::getSceneObjects()
{
	return _vSceneObjs;
}

void Scene::setSceneObjects(std::vector<GameObject> data)
{
	_vSceneObjs = data;
}
