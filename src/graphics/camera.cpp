#include "camera.hpp"
#include <cmath>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM),
      m_yaw(yaw), m_pitch(pitch), m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
      m_worldUp(up) {
  Position = position;
  _updateCameraVectors();
}

Camera::Camera(float posX,
               float posY,
               float posZ,
               float upX,
               float upY,
               float upZ,
               float yaw,
               float pitch)
    : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM),
      m_yaw(yaw), m_pitch(pitch), m_front(glm::vec3(0.0f, 0.0f, -1.0f)) {
  Position = glm::vec3(posX, posY, posZ);
  m_worldUp = glm::vec3(upX, upY, upZ);
  _updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(Position, Position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const {
  return glm::perspective(glm::radians(Zoom), getAspect(), 0.1f, 100.0f);
}

float Camera::getAspect() const {
  return (m_sceneHeight > 0) ? (m_sceneWidth / m_sceneHeight) : 1.0f;
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
  float velocity = MovementSpeed * deltaTime;
  if (direction == FORWARD)
    Position += m_front * velocity;
  if (direction == BACKWARD)
    Position -= m_front * velocity;
  if (direction == LEFT)
    Position -= m_right * velocity;
  if (direction == RIGHT)
    Position += m_right * velocity;
}

void Camera::processMouseMovement(float xoffset,
                                  float yoffset,
                                  GLboolean constrainPitch) {
  xoffset *= MouseSensitivity;
  yoffset *= MouseSensitivity;

  m_yaw += xoffset;
  m_pitch += yoffset;

  if (constrainPitch) {
    if (m_pitch > 89.0f)
      m_pitch = 89.0f;
    if (m_pitch < -89.0f)
      m_pitch = -89.0f;
  }

  _updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
  Zoom -= (float)yoffset;
  if (Zoom < 1.0f)
    Zoom = 1.0f;
  if (Zoom > 45.0f)
    Zoom = 45.0f;
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
