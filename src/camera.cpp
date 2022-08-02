#include <SDL.h>
#include "camera.h"

FlyCamera::FlyCamera(glm::mat4 projection) {
    _projection = projection;
    // projection must be flipped
    _projection[1][1] *= -1;
    _position = POSITION_DEFAULT;
    _pitch = PITCH_DEFAULT;
    _yaw = YAW_DEFAULT;
    _velocity = VELOCITY_DEFAULT;
    _sensitivity = SENSITIVITY_DEFAULT;
    _up = glm::vec3(0.0f, 1.0f, 0.0f);
    _front = glm::vec3(0.0f, 0.0f, -1.0f);
    _worldUp = _up;
    update_camera_vectors();
}

glm::mat4 FlyCamera::get_view_matrix() {
    return glm::lookAt(_position, _position + _front, _up);
}

void FlyCamera::process_keyboard(double delta) {
    float velocity = _velocity * (delta / 1000);
    auto *keystate = const_cast<uint8_t *>(SDL_GetKeyboardState(nullptr));
    if (keystate[SDL_SCANCODE_W]) _position += _front * velocity;
    if (keystate[SDL_SCANCODE_A]) _position -= _right * velocity;
    if (keystate[SDL_SCANCODE_S]) _position -= _front * velocity;
    if (keystate[SDL_SCANCODE_D]) _position += _right * velocity;
    if (keystate[SDL_SCANCODE_Q]) _position -= _up * velocity;
    if (keystate[SDL_SCANCODE_E]) _position += _up * velocity;
}

void FlyCamera::process_mouse(float dx, float dy) {
    dx *= _sensitivity;
    dy *= _sensitivity;
    _yaw += dx;
    _pitch += -dy;

    if(_pitch > 89.0f) _pitch = 89.0f;
    if(_pitch < -89.0f) _pitch = -89.0f;

    update_camera_vectors();
}

void FlyCamera::update_camera_vectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    front.y = sin(glm::radians(_pitch));
    front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    _front = glm::normalize(front);
    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
}
