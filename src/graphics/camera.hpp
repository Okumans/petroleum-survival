#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
  // Camera Attributes
  glm::vec3 position;
  float movementSpeed;
  float mouseSensitivity;
  float zoom;

private:
  // Euler Angles
  float m_yaw;
  float m_pitch;

  // Camera Vectors
  glm::vec3 m_front;
  glm::vec3 m_up;
  glm::vec3 m_right;
  glm::vec3 m_worldUp;

  float m_sceneWidth = 800.0f;
  float m_sceneHeight = 600.0f;

public:
  // Constructor with vectors
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
         glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
         float pitch = PITCH);

  // Constructor with scalar values
  Camera(float pos_x, float pos_y, float pos_z, float up_x, float up_y,
         float up_z, float yaw, float pitch);

  // Matrix Getters
  [[nodiscard]] glm::mat4 getViewMatrix() const;
  [[nodiscard]] glm::mat4 getProjectionMatrix() const;
  [[nodiscard]] float getAspect() const;

  // Input Processing
  void processKeyboard(Camera_Movement direction, float deltaTime);
  void processMouseMovement(float x_offset, float y_offset,
                            GLboolean constrain_pitch);
  void processMouseScroll(float y_offset);

  // Setters & Getters
  void setYaw(float yaw, bool update_camera_vectors = true);
  void setPitch(float pitch, bool update_camera_vectors = true);

  [[nodiscard]] float getYaw() const;
  [[nodiscard]] float getPitch() const;

  [[nodiscard]] glm::vec3 getRight() const;
  [[nodiscard]] glm::vec3 getFront() const;

  [[nodiscard]] float getSceneWidth() const { return m_sceneWidth; }
  [[nodiscard]] float getSceneHeight() const { return m_sceneHeight; }

  void updateSceneSize(float width, float height);

private:
  // Internal update logic
  void _updateCameraVectors();
};
