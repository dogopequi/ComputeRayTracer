#include "Shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

Shader::Shader(const char* vertPath, const char* fragPath)
    : m_VertPath(vertPath), m_FragPath(fragPath)
{
    m_ShaderID = load();
}

Shader::Shader(const char* computePath)
    : m_ComputePath(computePath)
{
    m_ShaderID = loadCompute();
}

Shader::Shader()
{

}

Shader::~Shader()
{
    glDeleteProgram(m_ShaderID);
}

void Shader::setUniform1f(const GLchar* name, float value)
{
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniform1i(const GLchar* name, int value)
{
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniform2f(const GLchar* name, const glm::vec2& vector)
{
    glUniform2f(getUniformLocation(name), vector.x, vector.y);
}

void Shader::setUniform2ui(const GLchar* name, const glm::vec2& vector)
{
    glUniform2ui(getUniformLocation(name), vector.x, vector.y);
}

void Shader::setUniform2i(const GLchar* name, const glm::vec2& vector)
{
    glUniform2ui(getUniformLocation(name), vector.x, vector.y);
}

void Shader::setUniform3f(const GLchar* name, const glm::vec3& vector)
{
    glUniform3f(getUniformLocation(name), vector.x, vector.y, vector.z);
}

void Shader::setUniform1ui(const GLchar* name, unsigned int value)
{
    glUniform1ui(getUniformLocation(name), value);
}

void Shader::setUniform4f(const GLchar* name, const glm::vec4& vector)
{
    glUniform4f(getUniformLocation(name), vector.x, vector.y, vector.z, vector.w);
}

void Shader::setUniformMat4(const GLchar* name, const glm::mat4& matrix)
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::enable() const
{
    glUseProgram(m_ShaderID);
}

void Shader::disable() const
{
    glUseProgram(0);
}

GLuint Shader::loadCompute()
{
    std::string computeCode;
    std::ifstream ComputeFile;
    ComputeFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try 
    {
        ComputeFile.open(m_ComputePath);
        std::stringstream ShaderStream;
        ShaderStream << ComputeFile.rdbuf();
        ComputeFile.close();
        computeCode = ShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* cShaderCode = computeCode.c_str();

    unsigned int compute;
    int success;
    char infoLog[512];
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(compute, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" <<
            infoLog << std::endl;
    };
    GLuint ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    glUseProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
            infoLog << std::endl;
    }
    glDeleteShader(compute);
    std::cout << "Compute Shader Path: " << m_ComputePath << std::endl;

    return ID;
}

GLuint Shader::load()
{
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        vShaderFile.open(m_VertPath);
        fShaderFile.open(m_FragPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" <<
            infoLog << std::endl;
    };

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAMENT::COMPILATION_FAILED\n" <<
            infoLog << std::endl;
    };
    GLuint ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glUseProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" <<
            infoLog << std::endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    std::cout << "Vertex Shader Path: " << m_VertPath << std::endl;
    std::cout << "Fragment Shader Path: " << m_FragPath << std::endl;

    return ID;
}

void Shader::writeImage(std::string file, int width, int height, int channels, const void* data, int stride)
{
    stbi_write_png(file.c_str(), width, height, channels, data, stride);
}

GLint Shader::getUniformLocation(const GLchar* name)
{
    return glGetUniformLocation(m_ShaderID, name);
}

GLuint Shader::loadImage()
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    const char* c = stbi_failure_reason();
    std::cout << c << std::endl;
    if (image) {
        GLenum format;
        if (channels == 1) {
            format = GL_RED;
        }
        else if (channels == 3) {
            format = GL_RGB;
        }
        else if (channels == 4) {
            format = GL_RGBA;
        }
        else
        {
            format = GL_RED;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
    }
    else {
        std::cerr << "Failed to load texture: " << filename << std::endl;
    }
    if (textureID == 0) {
        glfwTerminate();
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(m_ShaderID, "textureSampler"), 0);

    return textureID;
}

void Shader::checkCompileErrors(unsigned int shader, std::string type)
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}