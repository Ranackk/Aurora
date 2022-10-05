#include "stdafx.h"

#include "Rendering/ShaderUtility.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


GLint ShaderUtility::LoadUpShader(const char* vertexShaderPath, const char* fragmentShaderPath, const char* geometryShaderPath /*= nullptr*/)
{
	std::cout << "Loading up shader from " << vertexShaderPath << ", " << fragmentShaderPath;
	if (geometryShaderPath)
	{
		std::cout << ", " << geometryShaderPath;
	}
	std::cout << std::endl;

	//////////////////////////////////////////////////////////////////////////
	// 0) Setup

	std::string vertexCodeStr;
	std::string fragmentCodeStr;
	std::string geometryCodeStr;

	//////////////////////////////////////////////////////////////////////////
	// 1) Load from file
	// 1a) Vertex Shader
	try
	{
		std::ifstream vShaderFile;
		std::stringstream vShaderStream;
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		vShaderFile.open(vertexShaderPath);
		vShaderStream << vShaderFile.rdbuf();
		vShaderFile.close();
		vertexCodeStr = vShaderStream.str();

	}
	catch (std::ifstream::failure& e)
	{
		std::cout << "LoadUpShader failed: \"" << e.what() << "\" for shader " << vertexShaderPath << std::endl;
	}

	//////////////////////////////////////////////////////////////////////////
	// 1b) Fragment Shader
	try
	{
		std::ifstream fShaderFile;
		fShaderFile.open(fragmentShaderPath);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::stringstream fShaderStream;
		fShaderStream << fShaderFile.rdbuf();
		fShaderFile.close();
		fragmentCodeStr = fShaderStream.str();
	}
	catch (std::ifstream::failure& e)
	{
		std::cout << "LoadUpShader failed: \"" << e.what() << "\" for shader " << vertexShaderPath << std::endl;
	}

	//////////////////////////////////////////////////////////////////////////
	// [OPT] 1c) Geometry Shader
	if (geometryShaderPath != nullptr)
	{
		try
		{
			std::ifstream gShaderFile;
			gShaderFile.open(geometryShaderPath);
			gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			std::stringstream gShaderStream;
			gShaderStream << gShaderFile.rdbuf();
			gShaderFile.close();
			geometryCodeStr = gShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "LoadUpShader failed: \"" << e.what() << "\" for shader " << vertexShaderPath << std::endl;
		}
	}

	const char* vertexShaderCharPtr			= vertexCodeStr.c_str();
	const char* fragemntShaderCharPtr		= fragmentCodeStr.c_str();
	const char* geometryShaderCodeCharPtr	= geometryCodeStr.length() != 0 ? geometryCodeStr.c_str() : nullptr;

	return LoadUpShaderCode(vertexShaderCharPtr, fragemntShaderCharPtr, geometryShaderCodeCharPtr);
}

//////////////////////////////////////////////////////////////////////////

GLint ShaderUtility::LoadUpShaderCode(const char* vertexShaderCharPtr, const char* fragemntShaderCharPtr, const char* geometryShaderCodeCharPtr /*= nullptr*/)
{
//////////////////////////////////////////////////////////////////////////
	// 2) Compile Shaders
	// 2a) Vertex Shader
	GLint vertexShaderHandle;
	vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderHandle, 1, &vertexShaderCharPtr, NULL);
	glCompileShader(vertexShaderHandle);
	VerifyLoadedShader(vertexShaderHandle, CompileStage::Vertex);
	//std::cout << "Vertex shader loaded as " << vertexShaderHandle << std::endl;


	// 2b) Fragment Shader
	GLint fragmentShaderHandle;
	fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderHandle, 1, &fragemntShaderCharPtr, NULL);
	glCompileShader(fragmentShaderHandle);
	VerifyLoadedShader(fragmentShaderHandle, CompileStage::Fragment);
	//std::cout << "Fragment shader loaded as " << fragmentShaderHandle << std::endl;

	// 2c) [OPT] Geometry Shader
	GLint geometryShaderHandle;
	if (geometryShaderCodeCharPtr != nullptr)
	{
		geometryShaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometryShaderHandle, 1, &geometryShaderCodeCharPtr, NULL);
		glCompileShader(geometryShaderHandle);
		VerifyLoadedShader(geometryShaderHandle, CompileStage::Geometry);
		//std::cout << "Geometry shader loaded as " << geometryShaderHandle << std::endl;
	}

	//////////////////////////////////////////////////////////////////////////
	// 3) Create Program

	GLint programHandle;
	// shader Program
	programHandle = glCreateProgram();
	glAttachShader(programHandle, vertexShaderHandle);
	glAttachShader(programHandle, fragmentShaderHandle);
	if (geometryShaderCodeCharPtr != nullptr)
	{
		glAttachShader(programHandle, geometryShaderHandle);
	}
	glLinkProgram(programHandle);
	VerifyLoadedShader(programHandle, CompileStage::Program);

	//////////////////////////////////////////////////////////////////////////
	// 4) Clean Up

	glDeleteShader(vertexShaderHandle);
	glDeleteShader(fragmentShaderHandle);
	if (geometryShaderCodeCharPtr != nullptr)
	{
		glDeleteShader(geometryShaderHandle);
	}

	std::cout << "Loaded up shader program with ID " << programHandle << std::endl;

	return programHandle;
}

//////////////////////////////////////////////////////////////////////////

void ShaderUtility::VerifyLoadedShader(const GLint shaderHandle, const CompileStage shaderCompileStage)
{
	GLint	success;
	GLchar	infoLog[1024];

	if (shaderCompileStage != CompileStage::Program)
	{
		glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shaderHandle, 1024, NULL, infoLog);
			std::cout << "VerifyLoadedShader failed: \"" << infoLog << "\" for shader type " << static_cast<int>(shaderCompileStage) << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shaderHandle, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shaderHandle, 1024, NULL, infoLog);
			std::cout << "VerifyLoadedShader failed: \"" << infoLog << "\" for program. " << std::endl;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void ShaderUtility::BindShader(const GLuint shaderHandle)
{
	glUseProgram(shaderHandle);
}

void ShaderUtility::UnBindShader()
{
	glUseProgram(0);
}

//////////////////////////////////////////////////////////////////////////

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const float x, const float y, const float z)
{
  glUniform3f(glGetUniformLocation(shaderHandle, name.c_str()), x, y, z);
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::vec2& v)
{
  glUniform2fv(glGetUniformLocation(shaderHandle, name.c_str()), 1, glm::value_ptr(v));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::vec3& v)
{
  glUniform3fv(glGetUniformLocation(shaderHandle, name.c_str()), 1, glm::value_ptr(v));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::dvec3& v)
{
  glUniform3dv(glGetUniformLocation(shaderHandle, name.c_str()), 1, glm::value_ptr(v));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::vec4& v)
{
  glUniform4fv(glGetUniformLocation(shaderHandle, name.c_str()), 1, glm::value_ptr(v));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::dvec4& v)
{
  glUniform4dv(glGetUniformLocation(shaderHandle, name.c_str()), 1, glm::value_ptr(v));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::dmat4& m)
{
  glUniformMatrix4dv(glGetUniformLocation(shaderHandle, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::mat4& m)
{
  glUniformMatrix4fv(glGetUniformLocation(shaderHandle, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const glm::mat3& m)
{
  glUniformMatrix3fv(glGetUniformLocation(shaderHandle, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const float val)
{
  glUniform1f(glGetUniformLocation(shaderHandle, name.c_str()), val);
}

void ShaderUtility::SetUniform(const GLuint shaderHandle, const std::string& name, const int val)
{
  glUniform1i(glGetUniformLocation(shaderHandle, name.c_str()), val);
}

