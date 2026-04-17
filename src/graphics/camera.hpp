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
  glm::vec3 Position;
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;

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
  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
         float yaw, float pitch);

  // Matrix Getters
  glm::mat4 getViewMatrix() const;
  glm::mat4 getProjectionMatrix() const;
  float getAspect() const;

  // Input Processing
  void processKeyboard(Camera_Movement direction, float deltaTime);
  void processMouseMovement(float xoffset, float yoffset,
                            GLboolean constrainPitch = true);
  void processMouseScroll(float yoffset);

  // Setters & Getters
  void setYaw(float yaw, bool update_camera_vectors = true);
  void setPitch(float pitch, bool update_camera_vectors = true);

  float getYaw() const;
  float getPitch() const;

  glm::vec3 getRight() const;
  glm::vec3 getFront() const;

  void updateSceneSize(float width, float height);

  float getSceneWidth() const { return m_sceneWidth; }
  float getSceneHeight() const { return m_sceneHeight; }

private:
  // Internal update logic
  void _updateCameraVectors();
};
