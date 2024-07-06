
#include <vk_types.h>
#include <SDL_events.h>
#include"ColliderShapes.h"
class Camera 
{
public:
	v3 Position;
	v3 Velocity;

	float moveSpeed = 1;
	float quickSpeed = 2.5f;

	bool isQuick = false;
	bool noclip = false;
	float pitch{ 0.f };
	float FOV{ 90.f }; // Stored in Degrees;
	float yaw{ 0.f };

	m4 getViewMat();
	m4 getRotMat();
	OBB _hitBox = OBB(v3(0), v3(1));

	void update();

	void processSDLEvents(SDL_Event& e, bool camlock);

};
