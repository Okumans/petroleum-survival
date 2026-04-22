#include "camera.hpp"
#include <cmath>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM),
      m_yaw(yaw), m_pitch(pitch), m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
      m_worldUp(up) {
  this->position = position;
  this->_updateCameraVectors();
}

Camera::Camera(float pos_x, float pos_y, float pos_z, float up_x, float up_y,
               float up_z, float yaw, float pitch)
    : movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), zoom(ZOOM),
      m_yaw(yaw), m_pitch(pitch), m_front(glm::vec3(0.0f, 0.0f, -1.0f)) {
  position = glm::vec3(pos_x, pos_y, pos_z);
  m_worldUp = glm::vec3(up_x, up_y, up_z);
  _updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(position, position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const {
  return glm::perspective(glm::radians(zoom), getAspect(), 0.1f, 100.0f);
}

float Camera::getAspect() const {
  return (m_sceneHeight > 0) ? (m_sceneWidth / m_sceneHeight) : 1.0f;
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = movementSpeed * deltaTime;
  if (direction == FORWARD)
    position += m_front * velocity;
  if (direction == BACKWARD)
    position -= m_front * velocity;
  if (direction == LEFT)
    position -= m_right * velocity;
  if (direction == RIGHT)
    position += m_right * velocity;
}

void Camera::processMouseMovement(float x_offset, float y_offset,
                                  GLboolean constrain_pitch) {
  x_offset *= mouseSensitivity;
  y_offset *= mouseSensitivity;

  m_yaw += x_offset;
  m_pitch += y_offset;

  if (constrain_pitch) {
    if (m_pitch > 89.0f)
      m_pitch = 89.0f;
    if (m_pitch < -89.0f)
      m_pitch = -89.0f;
  }

  _updateCameraVectors();
}

void Camera::processMouseScroll(float y_offset) {
  zoom -= y_offset;
  if (zoom < 1.0f)
    zoom = 1.0f;
  if (zoom > 45.0f)
    zoom = 45.0f;
}

void Camera::setYaw(float yaw, bool update_camera_vectors) {
  m_yaw = yaw;
  if (update_camera_vectors)
    _updateCameraVectors();
}

void Camera::setPitch(float pitch, bool update_camera_vectors) {
  m_pitch = pitch;
  if (m_pitch > 89.0f)
    m_pitch = 89.0f;
  if (m_pitch < -89.0f)
    m_pitch = -89.0f;

  if (update_camera_vectors)
    _updateCameraVectors();
}

float Camera::getYaw() const { return m_yaw; }
float Camera::getPitch() const { return m_pitch; }

glm::vec3 Camera::getRight() const { return m_right; }
glm::vec3 Camera::getFront() const { return m_front; }

void Camera::updateSceneSize(float width, float height) {
  m_sceneWidth = width;
  m_sceneHeight = height;
}

void Camera::_updateCameraVectors() {
  glm::vec3 front;
  front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
  front.y = sin(glm::radians(m_pitch));
  front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

  m_front = glm::normalize(front);
  m_right = glm::normalize(glm::cross(m_front, m_worldUp));
  m_up = glm::normalize(glm::cross(m_right, m_front));
}
