#pragma once

#include "ColliderShapes.h"
#include"vk_types.h"
#include<string>
struct LoadedGLTF;

class GameObject
{
public:
	GameObject()
	{
		pos = v3(0);
		scale = v3(0);
	}
	~GameObject()
	{
		_vOBB.clear();
		_pGLTF = nullptr;
	}
	void Update();

	void updateModelMat();


	const bool getCollidable()
	{
		return canCollide;
	}

	m4 _modelMat = m4(1.f);
	Transform _transform;
	v3 pos, scale = v3(0);
	v3 rot_velocity = v3(0);
	v3 velocity = v3(0);
	bool followPlight = false;
	std::string m_id = "Cube"; // box by default
	//ColliderShapes::ColliderTypes _colliderType;
	std::vector<OBB> _vOBB;
	int _numColliders = 1;
	bool canCollide = true;
	bool _simulate = false;
	bool _static = false;
	glm::quat rot = { 0, 0, 0, 0 };
	std::string Name = "Unnammed";
	std::string tag;
	std::shared_ptr<LoadedGLTF> _pGLTF;
};