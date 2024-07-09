
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

	bool bNoclip = true;
	m4 m_viewMat;
	m4 m_RotMat;

	v3 right()		const { return v3(m_viewMat[0].x, 0, m_viewMat[2].x); }
	v3 up()			const { return v3(0, m_viewMat[1].y, 0); }
	v3 forward()	const { return v3(m_viewMat[0].z, 0, m_viewMat[2].z); }

	v3 right_dir()   const { return v3(m_viewMat[0].x, m_viewMat[1].x, m_viewMat[2].x); }
	v3 up_dir()      const { return v3(m_viewMat[0].y, m_viewMat[1].y, m_viewMat[2].y); }
	v3 forward_dir() const { return v3(m_viewMat[0].z, m_viewMat[1].z, m_viewMat[2].z); }
	m4 getViewMat();
	m4 getRotMat();
	OBB _hitBox = OBB(v3(0), v3(1));

	void update();

	void processSDLEvents(SDL_Event& e, bool camlock);

};
