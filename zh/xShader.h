#pragma once

#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum xShaderType {
	Vertex = 1, Fragment = 2
};

class xShader {
public:
	xShader(char* shaderStr, xShaderType type) {
		if (type == xShaderType::Vertex) {
			shader = glCreateShader(GL_VERTEX_SHADER);
		}
		else if (type == xShaderType::Fragment) {
			shader = glCreateShader(GL_FRAGMENT_SHADER);
		}
		glShaderSource(shader, 1, &shaderStr, NULL);
		glCompileShader(shader);
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0) {
			char* log = (char*)malloc(logLen);
			glGetShaderInfoLog(shader, logLen, NULL, log);
			printf("ShaderLog:\n%s\n", log);
			free(log);
		}
	}
	~xShader() {
		if (shader != NULL) {
			glDeleteShader(shader);
			shader = 0;
		}
	}
	GLuint shader = 0;
};