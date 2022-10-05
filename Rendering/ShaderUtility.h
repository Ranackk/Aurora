#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class ShaderUtility
{
	enum class CompileStage
	{
		Vertex,
		Fragment,
		Geometry,

		Program
	};
public:

	static GLint LoadUpShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
	static GLint LoadUpShaderCode(const char* vertexShaderCharPtr, const char* fragemntShaderCharPtr, const char* geometryShaderCodeCharPtr = nullptr);
	static void BindShader(const GLuint shaderHandle);
	static void UnBindShader();
	
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const float x, const float y, const float z);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::vec2& v);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::vec3& v);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::dvec3& v);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::vec4& v);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::dvec4& v);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::dmat4& m);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::mat4& m);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const glm::mat3& m);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const float val);
	static void SetUniform(const GLuint shaderHandle, const std::string& name, const int val);

private:

	static void VerifyLoadedShader(GLint vertexShaderHandle, CompileStage param2);
};