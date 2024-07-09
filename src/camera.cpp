#include "camera.h"
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include <glm/gtx/quaternion.hpp>
m4 Camera::getViewMat()
{
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    //  so we will create the camera model matrix and invert
    m4 cameraTranslation = glm::translate(m4(1.f), Position);
    m4 cameraRotation = getRotMat();
    m_viewMat = glm::inverse(cameraTranslation * cameraRotation);
    return m_viewMat;
}

m4 Camera::getRotMat()
{
    // fairly typical FPS style camera. we join the pitch and yaw rotations into
    // the final rotation matrix
    pitch = std::clamp(pitch, -1.57f, 1.57f);

    glm::quat pitchRotation = glm::angleAxis(pitch, v3{ 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, v3{ 0.f, -1.f, 0.f });


    return m_RotMat = glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

void Camera::update()
{
	m4 cameraRotation = getRotMat();
    if (bNoclip)
    {
        Position += v3(cameraRotation * v4(Velocity * 0.5f, 0.f));

    }
    else
    {
        float y = Position.y;
        Position += v3(cameraRotation * v4(Velocity * 0.5f, 0.f));
        Position.y = y;
    }
    _hitBox.update(Position);
}

/// <summary>
/// Process Input
/// </summary>
/// <param name="e">- SDL Event</param>
void Camera::processSDLEvents(SDL_Event& e, bool camlock)
{
   
        if (e.type == SDL_KEYDOWN)
        {
            float mov = 0;
            if (e.key.keysym.sym == SDLK_LSHIFT) { isQuick = true; }
            (isQuick) ? mov = quickSpeed : mov = moveSpeed;

            if (e.key.keysym.sym == SDLK_w) { Velocity.z = -mov; }
            if (e.key.keysym.sym == SDLK_s) { Velocity.z = mov; }
            if (e.key.keysym.sym == SDLK_SPACE) { Velocity.y = 1; }

            if (e.key.keysym.sym == SDLK_LCTRL) { Velocity.y = -1; }

            if (e.key.keysym.sym == SDLK_a) { Velocity.x = -mov; }
            if (e.key.keysym.sym == SDLK_d) { Velocity.x = mov; }
        }

        if (e.type == SDL_KEYUP) {
            if (e.key.keysym.sym == SDLK_w) { Velocity.z = 0; }
            if (e.key.keysym.sym == SDLK_s) { Velocity.z = 0; }
            if (e.key.keysym.sym == SDLK_SPACE) { Velocity.y = 0; }
            if (e.key.keysym.sym == SDLK_LCTRL) { Velocity.y = 0; }
            if (e.key.keysym.sym == SDLK_LSHIFT) { isQuick = false; }

            if (e.key.keysym.sym == SDLK_a) { Velocity.x = 0; }
            if (e.key.keysym.sym == SDLK_d) { Velocity.x = 0; }
        }
  
    if (camlock)
    {
        return;
    }
    if (e.type == SDL_MOUSEMOTION)
    {
        yaw += (float)e.motion.xrel / 200.f;
        pitch -= (float)e.motion.yrel / 200.f;
    }
}
