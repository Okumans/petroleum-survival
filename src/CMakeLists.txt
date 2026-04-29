#-----------------------------------------------------------------------------#
# define the asset path in c++
set(ASSETS_DIR ${PROJECT_SOURCE_DIR}/assets)
set(ASSETS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/assets)
file(COPY ${ASSETS_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})
# Additionally, install assets for QtCreator and CLion IDE.
file(COPY ${ASSETS_DIR} DESTINATION ${CMAKE_BINARY_DIR})
 
#-----------------------------------------------------------------------------#
# Embed shaders using xxd
#-----------------------------------------------------------------------------#
set(GENERATED_SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
file(MAKE_DIRECTORY ${GENERATED_SHADERS_DIR})

file(GLOB SHADER_SOURCES "${ASSETS_DIR}/shaders/*.glsl")
set(GENERATED_SHADER_HEADERS "")

foreach(SHADER_PATH ${SHADER_SOURCES})
  get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
  set(OUTPUT_FILE "${GENERATED_SHADERS_DIR}/${SHADER_NAME}.h")

  add_custom_command(
        OUTPUT "${OUTPUT_FILE}"
        COMMAND xxd -i "${SHADER_NAME}" > "${OUTPUT_FILE}"
        WORKING_DIRECTORY "${ASSETS_DIR}/shaders"
        DEPENDS "${SHADER_PATH}"
        COMMENT "Embedding shader ${SHADER_NAME}"
    )
  list(APPEND GENERATED_SHADER_HEADERS "${OUTPUT_FILE}")
endforeach()

#-----------------------------------------------------------------------------#
# list of all source files
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INC_DIR ${PROJECT_SOURCE_DIR}/include)
file(GLOB_RECURSE SRC_HEADER_FILES CONFIGURE_DEPENDS "${SRC_DIR}/*.h" "${SRC_DIR}/*.hpp")
file(GLOB_RECURSE INC_HEADER_FILES CONFIGURE_DEPENDS "${INC_DIR}/*.h" "${INC_DIR}/*.hpp")
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${ASSETS_DIR}/shaders/*.glsl")
set(ALL_HEADERS ${SRC_HEADER_FILES} ${INC_HEADER_FILES})

#-----------------------------------------------------------------------------#
# list all files that will either be used for compilation or that should show
# up in the ide of your choice
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${ALL_HEADERS} ${SHADER_FILES} ${GENERATED_SHADER_HEADERS})
GroupSourcesByFolder(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}
    ${INC_DIR}
    ${GENERATED_SHADERS_DIR}
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

file(RELATIVE_PATH
  ASSETS_LOCATION
  ${CMAKE_INSTALL_PREFIX}/bin
  ${ASSETS_INSTALL_DIR}
)
target_compile_definitions(${PROJECT_NAME} PRIVATE ASSETS_PATH="${ASSETS_LOCATION}")

if(MSVC)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_COMMAND ${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:${CMAKE_PROJECT_NAME}>)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
endif()

# specify libraries to link with after compilation
target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
  glfw
  ${GLAD_LIBRARY}
  $<$<NOT:$<PLATFORM_ID:Windows>>:m>  # libm is Unix-only; on Windows math is in the CRT
  glm
  assimp
)

install(TARGETS ${CMAKE_PROJECT_NAME}
EXPORT ${CMAKE_PROJECT_NAME}-targets
RUNTIME DESTINATION bin
ARCHIVE DESTINATION lib
LIBRARY DESTINATION lib)
