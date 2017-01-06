#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h> // Include glew to get all the required OpenGL headers

class Shader
{
public:
	// The program ID
	GLuint Program;
	//GLint projId, transId, fragmentColorId;
	// Constructor reads and builds the shader
	Shader();
	Shader(const GLchar* vertexPath, const GLchar* fragmentPath);
	// Use the program
	void Use();
};
