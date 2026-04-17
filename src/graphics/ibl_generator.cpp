#include "ibl_generator.hpp"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

std::shared_ptr<Texture> IBLGenerator::generateIrradianceMap(
    const Texture &skybox, const Skybox &cubeMesh, Shader &irradianceShader) {
  GLuint captureFBO, captureRBO;
  glCreateFramebuffers(1, &captureFBO);
  glCreateRenderbuffers(1, &captureRBO);

  glNamedRenderbufferStorage(captureRBO, GL_DEPTH_COMPONENT24, 32, 32);
  glNamedFramebufferRenderbuffer(captureFBO,
                                 GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER,
                                 captureRBO);

  GLuint irradianceMap;
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &irradianceMap);
  glTextureStorage2D(irradianceMap, 1, GL_RGB16F, 32, 32);

  glTextureParameteri(irradianceMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(irradianceMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(irradianceMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTextureParameteri(irradianceMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(irradianceMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glm::mat4 captureProjection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  glm::mat4 captureViews[] = {glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(1.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, -1.0f, 0.0f)),
                              glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(-1.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, -1.0f, 0.0f)),
                              glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f),
                                          glm::vec3(0.0f, 0.0f, 1.0f)),
                              glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, -1.0f, 0.0f),
                                          glm::vec3(0.0f, 0.0f, -1.0f)),
                              glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 0.0f, 1.0f),
                                          glm::vec3(0.0f, -1.0f, 0.0f)),
                              glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 0.0f, -1.0f),
                                          glm::vec3(0.0f, -1.0f, 0.0f))};

  irradianceShader.use();
  irradianceShader.setInt("u_Skybox", 0);
  irradianceShader.setMat4("u_Projection", captureProjection);
  glBindTextureUnit(0, skybox.getTexID());

  // We still need to bind the FBO for rendering, as glDrawArrays is not DSA for
  // the framebuffer target itself and glViewport affects the global state.
  glViewport(0, 0, 32, 32);
  glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
  for (unsigned int i = 0; i < 6; ++i) {
    irradianceShader.setMat4("u_View", captureViews[i]);
    glNamedFramebufferTextureLayer(captureFBO,
                                   GL_COLOR_ATTACHMENT0,
                                   irradianceMap,
                                   0,
                                   i);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(cubeMesh.getVAO());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glDeleteFramebuffers(1, &captureFBO);
  glDeleteRenderbuffers(1, &captureRBO);

  return std::make_shared<Texture>(irradianceMap,
                                   TextureType::IRRADIANCE_MAP,
                                   true);
}
