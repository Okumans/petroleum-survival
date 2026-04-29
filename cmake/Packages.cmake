set(FETCHCONTENT_QUIET FALSE)

# --- GLFW ---
CPMAddPackage(
  NAME glfw
  VERSION 3.3.10
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.3.10
  OPTIONS "BUILD_SHARED_LIBS ON"
          "GLFW_BUILD_EXAMPLES OFF"
          "GLFW_BUILD_TESTS OFF"
          "GLFW_BUILD_DOCS OFF"
)
if(glfw_ADDED)
  find_targets(glfw_targets ${glfw_SOURCE_DIR})
  make_folder("glfw" ${glfw_targets})
endif()

# --- GLAD ---
CPMAddPackage(
  NAME glad
  GITHUB_REPOSITORY Dav1dde/glad
  VERSION 2.0.6
  DOWNLOAD_ONLY
)
if(glad_ADDED)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${glad_SOURCE_DIR}/cmake)
  include(GladConfig)
  add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
  set(GLAD_LIBRARY glad_gl_core_46)
  # https://github.com/Dav1dde/glad/wiki/C#generating-during-build-process
  glad_add_library(${GLAD_LIBRARY} SHARED API gl:core=4.6)
  make_folder("glad" ${GLAD_LIBRARY})
  install(TARGETS ${GLAD_LIBRARY}
    EXPORT ${GLAD_LIBRARY}-targets
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)
endif()

# --- GLM ---
CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 0.9.9.8
)
if(glm_ADDED)
  make_folder("glm" glm)
endif()



# --- Assimp ---
# NOTE: System pkg-config + minizip lookup removed — assimp is built with
# ASSIMP_BUILD_MINIZIP ON below, which provides minizip internally, and the
# project source does not include minizip headers directly. Re-enable these
# lines if you switch ASSIMP_BUILD_MINIZIP to OFF.
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(MINIZIP REQUIRED minizip)
# include_directories(${MINIZIP_INCLUDE_DIRS})

CPMAddPackage(
  NAME assimp
  GITHUB_REPOSITORY assimp/assimp
  VERSION 5.4.3
  OPTIONS "ASSIMP_BUILD_TESTS OFF"
          "ASSIMP_BUILD_SAMPLES OFF"
          "ASSIMP_BUILD_ASSIMP_TOOLS OFF"
          "ASSIMP_INSTALL_PDB OFF"
          "BUILD_SHARED_LIBS ON"
          "ASSIMP_BUILD_MINIZIP ON"
          "ASSIMP_BUILD_ZLIB ON"
          "ASSIMP_WARNINGS_AS_ERRORS OFF"
)
if(assimp_ADDED)
  make_folder("assimp" assimp)
endif()

# --- Utilities ---
# To fix build errors in Qt Creator for the latest versions of CMake.
set(CMP0169 OLD)

# Source file grouping of visual studio and xcode
CPMAddPackage(
  NAME GroupSourcesByFolder.cmake
  GITHUB_REPOSITORY TheLartians/GroupSourcesByFolder.cmake
  VERSION 1.0
)
