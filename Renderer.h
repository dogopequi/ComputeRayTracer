#pragma once
#include "Shader.h"
#include "Window.h"
#include <vector>
#include "glm/common.hpp"
#include <random>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <filesystem>
#include <cmath>

class Timer
{
private:
	typedef std::chrono::high_resolution_clock HighResolutionClock;
	typedef std::chrono::duration<float, std::milli> milliseconds_type;
	std::chrono::time_point<HighResolutionClock> m_Start;
public:
	Timer()
	{
		reset();
	}

	void reset()
	{
		m_Start = HighResolutionClock::now();
	}

	float elapsed()
	{
		return std::chrono::duration_cast<milliseconds_type>(HighResolutionClock::now() - m_Start).count() / 1000.0f;
	}

};

struct Sphere
{
	glm::vec3 center;
	float radius;
	glm::vec3 color;
	float index;
	float fuzziness;
	float refraction;
	glm::vec3 emissionColor;
	float emissionPower;
	glm::vec3 sPosition;
	bool isMoving;
};

struct Scene
{
	std::vector<Sphere> spheres;
	float fov;
	glm::vec3 lookFrom;
	glm::vec3 lookAt;
	glm::vec3 vUp;
	float defocusAngle;
	float focusDist;
	bool blur;
	glm::vec3 BackgroundColor;
};

struct Image
{
	unsigned int bitDepth = 0;
	unsigned int channels = 0;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned char* data = nullptr;
};

inline float randomFloat()
{
	static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
	static std::mt19937 generator;
	return distribution(generator);
}

class Renderer
{
public:
	Renderer(bool fullscreen, int windowWidth, int windowHeight, int imageWidth, int imageHeight, std::string name);
	~Renderer();

	void Init(std::string fragment, std::string vertex, std::string computeBase, std::string computeAccumulate);

private:
	void Render();
	bool LoadScenes();
	bool SaveScenes();
	void ImGuiRender();
	void OnResize(int width, int height);
	void controls();
	float load1f(std::string& line, std::string variable);
	glm::vec3 load3f(std::string& line, std::string variable);
	bool load1b(std::string& line, std::string variable);
	bool findFiles(std::string& name, std::string& type);
	void ImGuiStartFrame();
	void ImGuiSettings();
	void ImGuiScene();
	void ImGuiEndFrame();
	void preRender(int width, int height);

	Window* window = nullptr;
	GLuint VAO, VBO, EBO{};
	GLuint ssbo1, ssbo2, ssbo3, ssbo4;
	GLuint normalImage = 0;
	GLuint accumulatedImage = 0;
	Shader* compute = nullptr;
	Shader* pipeline = nullptr;
	Shader* accumulation = nullptr;

	std::string filename = "image.ppm";
	std::string m_Name{};


	bool show_demo_window = true;
	bool show_another_window = true;

	bool Fullscreen = false;
	bool Interactive = false;
	bool startRendering = false;
	bool canRenderAnimate = false;

	unsigned int SCR_WIDTH{};
	unsigned int SCR_HEIGHT{};
	unsigned int IMAGE_WIDTH{};
	unsigned int IMAGE_HEIGHT{};

	double deltaTime{};
	double  averageFrameTimeMilliseconds = 33.333;
	double  frameRate = 30;

	std::unordered_map<float, std::string> indexLabelMap{};

	bool writeImage = false;
	char str0[128] = "Hello, world!";
	size_t buf{};
	bool filexists{ false };

	char str1[128] = "Hello, world!";
	size_t buf1{};
	bool writeVideo = false;

	int ViewportWidth{};
	int ViewportHeight{};

	glm::vec2 worksizeGroup{ 16, 16 };

	int selectedList = 0;
	int selectedScene = 0;


	std::string filePath = "Resources/Scenes/scenes.dogo";

	std::vector<Scene> m_Scenes{};
	std::vector<Scene> m_AnimationScenes{};


	float aspectRatio{};
	std::vector<float> SphereBuffer1;
	std::vector<float> SphereBuffer2;
	std::vector<float> SphereBuffer3;
	std::vector<float> SphereBuffer4;
	int frame = 0;
	int frames = 0;

	bool accumulate{false};
	int bounces = 2;
	int samples = 2;
};

