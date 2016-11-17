set(EXECUTABLE VulkanTest)

set(SRCS main.cpp
         Context.cpp
	   PNGLoader.cpp
	   Texture2D.cpp
	   Utils.cpp
	   VertexInfo.cpp
	   Vulkan.cpp)

set(LIBS glfw vulkan png)

add_executable(${EXECUTABLE} ${SRCS})
target_link_libraries(${EXECUTABLE} ${LIBS})
