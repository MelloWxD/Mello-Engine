#include "GameObject.h"
#include "vk_engine.h"
void GameObject::Update()
{
	// Update Model Mat
	updateModelMat();

	// Update Hitboxes
	for (auto& hb : _vOBB)
	{
		hb.update(_transform, _modelMat);
	}


}

void GameObject::updateModelMat()
{
	if (_simulate)
	{
		_transform.position.y -= 9.8f * 0.001f;
	}
	_transform.position += velocity;
	_transform.rotation.x += rot_velocity.x;
	_transform.rotation.y += rot_velocity.y;
	_transform.rotation.z += rot_velocity.z;
	_modelMat = glm::translate(_transform.position) * glm::rotate(_transform.rotation.x, v3(1, 0, 0)) * glm::rotate(_transform.rotation.y, v3(0, 1, 0)) * glm::rotate(_transform.rotation.z, v3(0, 0, 1)) * glm::scale(_transform.scale);
	_pGLTF->topNodes;
}