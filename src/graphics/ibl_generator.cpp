#include "ibl_generator.hpp"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

std::shared_ptr<Texture> IBLGenerator::generateIrradianceMap(
    const Texture &skybox, const Skybox &cube_mesh, Shader &irradiance_shader) {
  GLuint capture_fbo, capture_rbo;
  glCreateFramebuffers(1, &capture_fbo);
  glCreateRenderbuffers(1, &capture_rbo);

  glNamedRenderbufferStorage(capture_rbo, GL_DEPTH_COMPONENT24, 32, 32);
  glNamedFramebufferRenderbuffer(capture_fbo, GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER, capture_rbo);

  GLuint irradiance_map;
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &irradiance_map);
  glTextureStorage2D(irradiance_map, 1, GL_RGB16F, 32, 32);

  glTextureParameteri(irradiance_map, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(irradiance_map, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(irradiance_map, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTextureParameteri(irradiance_map, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(irradiance_map, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glm::mat4 capture_projection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  glm::mat4 capture_views[] = {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f))};

  irradiance_shader.use();
  irradiance_shader.setInt("u_Skybox", 0);
  irradiance_shader.setMat4("u_Projection", capture_projection);
  glBindTextureUnit(0, skybox.getTexID());

  // We still need to bind the FBO for rendering, as glDrawArrays is not DSA for
  // the framebuffer target itself and glViewport affects the global state.
  glViewport(0, 0, 32, 32);
  glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
  for (unsigned int i = 0; i < 6; ++i) {
    irradiance_shader.setMat4("u_View", capture_views[i]);
    glNamedFramebufferTextureLayer(capture_fbo, GL_COLOR_ATTACHMENT0,
                                   irradiance_map, 0, i);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(cube_mesh.getVAO());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDeleteFramebuffers(1, &capture_fbo);
  glDeleteRenderbuffers(1, &capture_rbo);

  return std::make_shared<Texture>(irradiance_map, TextureType::IRRADIANCE_MAP,
                                   true);
}
