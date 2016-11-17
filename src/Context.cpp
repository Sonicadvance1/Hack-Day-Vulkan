#include "Context.h"
#include <stdio.h>

namespace Context
{
	// GLFW error callback
	void ErrorCallback(int error, const char* desc)
	{
		fprintf(stderr, "GLFW error: %s\n", desc);
	}

	bool Init()
	{
		glfwSetErrorCallback(ErrorCallback);

		if (!glfwInit())
		{
			fprintf(stderr, "Couldn't initialize GLFW.\n");
			return false;
		}

		if (!glfwVulkanSupported())
		{
			fprintf(stderr, "GLFW failed to find a vulkan loader\n");
			return false;
		}

		return true;
	}

	void Shutdown()
	{
		glfwTerminate();
	}

	GLFWwindow* CreateWindow(int width, int height, const char* title)
	{
		GLFWwindow* win = glfwCreateWindow(width, height, title, nullptr, nullptr);

		if (!win)
		{
			fprintf(stderr, "Couldn't create GLFW window\n");
			return nullptr;
		}

		// XXX: Allow user provided callbacks
		return win;
	}

	void DestroyWindow(GLFWwindow* win)
	{
		glfwDestroyWindow(win);
	}
}
