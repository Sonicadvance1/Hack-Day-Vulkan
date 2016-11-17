#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Context
{
	bool Init();
	void Shutdown();

	GLFWwindow* CreateWindow(int width, int height, const char* title);
	void DestroyWindow(GLFWwindow* win);
}
