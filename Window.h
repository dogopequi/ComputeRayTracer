#pragma once
#define MAX_KEYS 1024
#define MAX_BUTTONS 32
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


class Window
{
public:
	Window(int width, int height, std::string name, bool fullscreen);
	Window() = delete;
	~Window();

	GLFWwindow* getWindow() const { return window; }

	bool isKeyPressed(unsigned int keycode) const;
	bool isMouseButtonPressed(unsigned int button) const;
	void getMousePosition(double& x, double& y) const;

private:
	friend struct GLFWwindow;
	GLFWwindow* window{};
	int m_Width;
	int m_Height;
	std::string m_Name;
	friend void window_resize(GLFWwindow* window, int width, int height);
	friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	friend void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	bool m_Keys[MAX_KEYS];
	bool m_MouseButtons[MAX_BUTTONS];
	double mx, my;
};

