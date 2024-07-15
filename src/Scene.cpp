#include"Scene.h"

#include"GameObject.h"
#include"Camera.h"
#include <CollisionHandler.h>


Scene::Scene()
{


}
Scene::~Scene()
{

}
void Scene::init(VulkanEngine* engine, const char* sceneJsonPath)
{
}
void Scene::Update(Camera& cam)
{
	auto start = std::chrono::system_clock::now();
	// _pEngine->mainDrawContext.OpaqueSurfaces.clear();

	auto& objs = _SceneObjects;
	
	//auto& stats = _pEngine->stats;
	// Check for collisions
	//_pEngine->stats.Collisions = 0;
	if (_SceneObjects.size() > 8)
	{
		// camera vs Gameobject collisions
		
		//if (!cam.bNoclip)
		//{
		//	for (int i = 8; i < objs.size(); ++i)
		//	{
		//		GameObject* objL = &_SceneObjects[i];
		//		if (objL->getCollidable() == false || cam.noclip == true)
		//		{
		//			continue; // go to next item
		//		}
		//		Simplex s;
		//		int T_idx = 0; // For knowing which tetrahedron inside the obb the collision was detected on
		//		for (auto& lOBB : objL->_vOBB)
		//		{
		//			if (GJK::GJK(lOBB, cam._hitBox, s, T_idx))
		//			{
		//				stats.Collisions += 1;
		//				CollisionInfo colinfo = EPA(s, &lOBB, &cam._hitBox, T_idx);
		//				if (colinfo.penDepth < 0.03)
		//				{
		//					colinfo.penDepth = 0.00f;
		//				}
		//				if (std::isnan(colinfo.penDepth))
		//				{
		//					colinfo.penDepth = 0;
		//				}
		//				int biggestIdx = 0;
		//				float max = -FLT_MAX;
		//				colinfo.Normal.y = 0;
		//				for (int y = 0; y < 3; ++y)
		//				{
		//					if (std::isnan(colinfo.Normal[y]))
		//					{
		//						colinfo.Normal[y] = 0;
		//						break;
		//					}
		//					colinfo.Normal[y] = fabs(colinfo.Normal[y]);
		//				}
		//				v3 DxDir = v3(0);
		//				DxDir[biggestIdx] = colinfo.Normal[biggestIdx]; // Single element response movement normal pointing from B - A
		//				v3 Dpos = glm::normalize(colinfo.Normal) * colinfo.penDepth;
		//				v3 diffVec = glm::normalize(lOBB.c - cam._hitBox.c);
		//				for (int y = 0; y < 3; ++y)
		//				{
		//					if (diffVec[y] < 0) // Obj is either on the right of player or behind
		//					{
		//						Dpos[y] *= -1;
		//					}
		//				}
		//				cam.Position -= (Dpos);
		//				//printf("Colliding with Object - %s  No %d\n", objR->GetModelID().c_str(), i);
		//			}
		//		}
		//	}
		//}
		// Game objects vs Game ojbects collisions
		for (int i = 8; i < _SceneObjects.size(); ++i)
		{
			for (int j = 8; j < _SceneObjects.size(); ++j)
			{
				if (i == j)
				{
					continue;
				}
				GameObject* objL = &_SceneObjects[i];
				GameObject* objR = &_SceneObjects[j];

				// Check for AABB Collisions.
				if (objL->getCollidable() == false || objR->getCollidable() == false)
				{
					continue; // go to next item
				}
				if (objL->_static == true && objR->_static == true)
				{
					continue; // go to next item
				}

				Simplex s;
				int T_idx = 0; // For knowing which tetrahedron inside the obb the collision was detected on
				for (auto lOBB : objL->_vOBB)
				{
					for (auto rObb : objR->_vOBB)
					{

						if (GJK::GJK(lOBB, rObb, s, T_idx))
						{

							//stats.Collisions += 1;

							CollisionInfo colinfo = EPA(s, &lOBB, &rObb, T_idx);

							if (colinfo.penDepth < 0.03)
							{
								colinfo.penDepth = 0.00f;
							}
							if (std::isnan(colinfo.penDepth))
							{
								colinfo.penDepth = 0;
							}
							int biggestIdx = 0;
							float max = -FLT_MAX;
							for (int y = 0; y < 3; ++y)
							{
								if (std::isnan(colinfo.Normal[y]))
								{
									colinfo.Normal[y] = 0;
									break;
								}
								colinfo.Normal[y] = fabs(colinfo.Normal[y]);

							}

							v3 DxDir = v3(0);
							DxDir[biggestIdx] = colinfo.Normal[biggestIdx]; // Single element response movement normal pointing from B - A

							v3 Dpos = glm::normalize(colinfo.Normal) * colinfo.penDepth;
							v3 diffVec = glm::normalize(lOBB.c - rObb.c);

							for (int y = 0; y < 3; ++y)
							{
								if (diffVec[y] < 0) // Obj is either on the right of player or behind
								{
									Dpos[y] *= -1;

								}
							}

							if (!objL->_static)
							{
								objL->_transform.position += (Dpos * v3(0.5f));
							}
							if (!objR->_static)
							{
								objR->_transform.position -= (Dpos * v3(0.5f));
							}

							objR->Update();
							objL->Update();

							//printf("Colliding with Object - %s  No %d\n", objR->GetModelID().c_str(), i);
						}
					}
				}
			}
		}
	}
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	
	//stats.col_resolve_time = elapsed.count() / 1000.f;

	start = std::chrono::system_clock::now();

	// Update gameobjects
	for (auto& entry : _SceneObjects)
	{

		entry.Update();
		//if (entry._pGLTF != nullptr) entry._pGLTF->Draw(entry._modelMat, _pEngine->mainDrawContext);

	}
	// Update rendering matrices
	glm::mat4 view = cam.getViewMat();

	// camera projection
	glm::mat4 projection = glm::perspective(glm::radians(cam.FOV), (float)CONST_SCREEN_RES.width / (float)CONST_SCREEN_RES.height, 10000.f, 0.1f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;

	// Update GPU push data
	_SceneGPUData.view = view;
	_SceneGPUData.proj = projection;
	_SceneGPUData.viewproj = projection * view;

	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	// stats.scene_update_time = elapsed.count() / 1000.f;

}

