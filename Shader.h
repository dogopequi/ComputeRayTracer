#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
class Shader
{
public:
	Shader(const char* vertPath, const char* fragPath);
	Shader(const char* computePath);
	Shader();
	~Shader();

	void setUniform1f(const GLchar* name, float value);
	void setUniform1i(const GLchar* name, int value);
	void setUniform1ui(const GLchar* name, unsigned int value);
	void setUniform2f(const GLchar* name, const glm::vec2& vector);
	void setUniform2ui(const GLchar* name, const glm::vec2& vector);
	void setUniform2i(const GLchar* name, const glm::vec2& vector);
	void setUniform3f(const GLchar* name, const glm::vec3& vector);
	void setUniform4f(const GLchar* name, const glm::vec4& vector);
	void setUniformMat4(const GLchar* name, const glm::mat4& matrix);

	void enable() const;
	void disable() const;

	void checkCompileErrors(unsigned int shader, std::string type);
	GLuint m_ShaderID;
	GLuint loadImage();
	void writeImage(std::string file, int width, int height, int channels, const void* data, int stride);
	GLuint load();
	GLuint loadCompute();

private:
	GLuint texID;
	GLint getUniformLocation(const GLchar* name);
	const char* m_VertPath;
	const char* m_FragPath;
	const char* m_ComputePath;
	std::string filename = "image.png";
};

