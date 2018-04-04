set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8.3)

project(viewer360)

message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules")

# FIND OPENGL
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

# FIND GLEW
#find_package(GLEW REQUIRED)
#if (GLEW_FOUND)
#    message(GLEW_INCLUDE_DIRS = ${GLEW_INCLUDE_DIRS})
#    include_directories($(GLEW_INCLUDE_DIRS))
#endif()

find_package(FFmpeg REQUIRED)
if (FFMPEG_FOUND)
  message("Found FFMPEG/LibAV libraries")
  include_directories(${FFMPEG_INCLUDE_DIR})
else (FFMPEG_FOUND)
  message("Can't find libavcodec, libavformat or libavutil. Add them!")
endif (FFMPEG_FOUND)


# Set output paths
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#file (COPY ${CMAKE_SOURCE_DIR}/../shaders DESTINATION ${CMAKE_BINARY_DIR} )

include_directories(
	external/glfw-3.0.3/include/GLFW/
	external/glm-0.9.4.0/
	external/glew-1.9.0/include/
	../
	.
	/home/toand/git/sage-dev/include
)

link_directories(
	/home/toand/git/sage-dev/lib
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	GLFW_303
	GLEW_190
    #${GLEW_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    -lsail
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

# Compile external dependencies
add_subdirectory (external)

# Main app
SET( srcs
		Utils.cpp
		GLUtils.cpp
		Program.cpp
		Material.cpp
		Texture.cpp
		Mesh.cpp
		Camera.cpp
        Renderer.cpp
        VideoDecode.cpp
		main.cpp
		)

# Headers
SET( headers
		Utils.h
		GLUtils.h
        GLInclude.h
		Program.h
		Material.h
		Texture.h
		Mesh.h
		Camera.h
        Renderer.h
        stb_image.h
        VideoDecode.h
        )

add_definitions(-DCMAKE_BUILD_TYPE=Debug -DPRINT_DEBUG)
add_definitions(-DSHADER_DIR=\"${CMAKE_SOURCE_DIR}/shaders/\")

add_executable(viewer360 ${srcs} ${headers})

# create the program
target_link_libraries(viewer360 ${ALL_LIBS} )
