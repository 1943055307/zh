#pragma once

#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h> 
#include "xShader.h" 

class xProgram {
public:
	xProgram(char* vertexShaderStr, char* fragmentShaderStr) {
		program = glCreateProgram();

		xShader vertexShader(vertexShaderStr, xShaderType::Vertex);
		xShader fragmentShader(fragmentShaderStr, xShaderType::Fragment);

		glAttachShader(program, vertexShader.shader);
		glAttachShader(program, fragmentShader.shader);
		glLinkProgram(program);

		GLint logLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0) {
			char* log = (char*)malloc(logLen);
			glGetProgramInfoLog(program, logLen, NULL, log);
			printf("ProgramLog:\n%s\n", log);
			free(log);
		}
	}
	~xProgram() {
		if (program != NULL) {
			glDeleteProgram(program);
			program = 0;
		}
	}
	GLuint program = 0;
};