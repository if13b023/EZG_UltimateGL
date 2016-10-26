#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "SOIL.h"

#include <iostream>
#include <random>
#include "FileIO.h"
#include "Shader.h"
#include "FishGL.h"

FishGL* global_engine_ptr = nullptr;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

int main()
{
	FishGL engine;
	global_engine_ptr = &engine;

	GLFWwindow* window = engine.createWindow(1280, 720);
	engine.setPerspective(70.0f, (GLfloat)1280 / (GLfloat)720, 0.1f, 500.0f);

	//bind keycallback method
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	//TINYOBJLOADER
	bool ret = false;
	std::string err;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	//ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "suzanne.obj");
	ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "cube.obj");

	if (!err.empty()) { // `err` may contain warning message.
		std::cout << err << std::endl;
		//std::cin.ignore();
	}
	if (!ret) {
		exit(1);
	}

	//sample preps
	/*GLfloat vertices[] = {
	0.5f,  0.5f, 0.0f,  // Top Right
	0.5f, -0.5f, 0.0f,  // Bottom Right
	-0.5f, -0.5f, 0.0f,  // Bottom Left
	-0.5f,  0.5f, 0.0f   // Top Left
	};
	GLuint indices[] = {  // Note that we start from 0!
	0, 1, 3,   // First Triangle
	1, 2, 3    // Second Triangle
	};*/

	//GLfloat* suzanne_vertices = new GLfloat[attrib.vertices.size()];
	/*GLuint* suzanne_indices = new GLuint[shapes[0].mesh.indices.size()];
	for (int i = 0; i < shapes[0].mesh.indices.size(); ++i)
		suzanne_indices[i] = shapes[0].mesh.indices[i].vertex_index;*/
	struct mesh
	{
		GLuint* indices;
		rsize_t size;
	};
	std::vector<mesh> scene;
	scene.resize(shapes.size());
	for (int i = 0; i < shapes.size(); ++i)
	{
		scene[i].indices = new GLuint[shapes[i].mesh.indices.size()];
		scene[i].size = shapes[i].mesh.indices.size();
		for (int j = 0; j < shapes[i].mesh.indices.size(); ++j)
			scene[i].indices[j] = shapes[i].mesh.indices[j].vertex_index;
	}

	// Load and create a texture 
	GLuint texture1;
	// ====================
	// Texture 1
	// ====================
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
											// Set our texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load, create texture and generate mipmaps
	int width, height;
	unsigned char* image = SOIL_load_image("concrete.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.

	Shader* main_shader = engine.addShader("VertexShader.glsl", "FragmentShader.glsl");

	GLuint VBO, VAO, EBO;

	GLint fragmentColorLocation = glGetUniformLocation(main_shader->Program, "uniformColor");

	short dist = 5;
	std::vector<sceneobj> objects;
	objects.resize(scene.size());
	for (GLuint i = 0; i < scene.size(); i++)
	{
		//objects[i].VAO = VAO;
		engine.addObject(attrib.vertices.data(), attrib.vertices.size(), scene[i].indices, scene[i].size, VBO, objects[i].VAO, objects[i].EBO);
		objects[i].iCount = scene[i].size;
		objects[i].scale = 1.0f;
		objects[i].shader = main_shader;
		//objects[i].position = glm::vec3((rand() % dist) - dist / 2, (rand() % dist) - dist / 2, (rand() % dist) - dist / 2);
		objects[i].position = glm::vec3(0,-2.0f,0);
	}
	//*****

	engine.addObjectsToScene(objects.data(), scene.size());

	//Animation
	animation anim;
	int size = 10;
	anim.duration = 10.0f;
	anim.count = size;
	keyframe* frame = new keyframe[size];
	/*for (int i = 0; i < size; ++i)
	{
		//frame[i].position = glm::vec3(glm::min(0, i - (size / 2)), 0, -glm::max(0, i - (size / 2)));
		frame[i].position = glm::vec3(3 * sinf(i*5.0f / size * 3.1415f), 0, i);
		if(i < size/2)
			frame[i].rotation = glm::quat(glm::vec3(0, glm::radians(90.0f), 0));
		else
			frame[i].rotation = glm::quat(glm::vec3(0, glm::radians(160.0f), 0));
		std::cout << "pos: " << frame[i].position.x << "|" << frame[i].position.y << "|" << frame[i].position.z << std::endl;
	}*/

	frame[0].position = glm::vec3(70.2145f, -35.0015f, -129.68f);
	frame[0].rotation = glm::quat(0.963065f, 0.143812f, 0.225151f, 0.033621f);
	frame[1].position = glm::vec3(23.5101f, -18.6689f, -92.4546f);
	frame[1].rotation = glm::quat(0.970905f, 0.139213f, 0.192871f, 0.0276547f);
	frame[2].position = glm::vec3(15.657f, -8.63422f, -47.8023f);
	frame[2].rotation = glm::quat(0.995971f, 0.0637391f, -0.0629597f, -0.00402941f);
	frame[3].position = glm::vec3(19.1488f, -6.00215f, -21.1694f);
	frame[3].rotation = glm::quat(0.998466f, 0.0337955f, -0.0438342f, -0.00148386f);
	frame[4].position = glm::vec3(19.1538f, -5.37055f, -0.728659f);
	frame[4].rotation = glm::quat(0.998556f, -0.00373702f, 0.0535915f, -0.000200745f);
	frame[5].position = glm::vec3(11.0216f, -4.84981f, 16.5689f);
	frame[5].rotation = glm::quat(0.935015f, 0.0182557f, 0.35407f, 0.00691284f);
	frame[6].position = glm::vec3(-4.20156f, -4.61827f, 27.8162f);
	frame[6].rotation = glm::quat(0.871484f, -0.0193797f, 0.489921f, -0.0108949f);
	frame[7].position = glm::vec3(-21.7111f, -6.11492f, 36.0732f);
	frame[7].rotation = glm::quat(0.811914f, -0.0219187f, 0.583153f, -0.0157432f);
	frame[8].position = glm::vec3(-40.3042f, -6.24003f, 38.6788f);
	frame[8].rotation = glm::quat(0.645806f, 0.0046985f, 0.763466f, 0.00555424f);
	frame[9].position = glm::vec3(-59.6454f, -7.27606f, 27.9539f);
	frame[9].rotation = glm::quat(0.380602f, -0.0175506f, 0.923591f, -0.0425896f);

	anim.frames = frame;

	engine.addAnimation(&anim);
	//****

	engine.Run();
	
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();
	
	//std::cin.ignore();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (global_engine_ptr == nullptr)
	{
		std::cout << "no engine pointer found\n";
	}
	else
		global_engine_ptr->key_callback(key, action);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	global_engine_ptr->mouse_callback(xpos, ypos);
}
