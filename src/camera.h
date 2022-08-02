#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// defaults
const glm::vec3 POSITION_DEFAULT = glm::vec3(0.0f, 0.0f, 0.0f);
const float PITCH_DEFAULT = 0.0f;
const float YAW_DEFAULT = -90.0f;
const float VELOCITY_DEFAULT = 20.0f;
const float SENSITIVITY_DEFAULT = 0.1f;

class FlyCamera {
public:
    FlyCamera(glm::mat4 projection);

    glm::mat4 get_view_matrix();

    void process_keyboard(double delta);

    void process_mouse(float dx, float dy);

    glm::mat4 _projection;
private:
    glm::vec3 _position;
    glm::vec3 _up;
    glm::vec3 _front;
    glm::vec3 _right;
    glm::vec3 _worldUp;
    float _pitch;
    float _yaw;
    float _velocity;
    float _sensitivity;

    void update_camera_vectors();
};