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

int main(int argc, char* argv[])
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

	if(argc < 2)
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "suzanne.obj");
		//ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "scene4.obj");
	else
		ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, argv[1]);

	if (!err.empty()) { // `err` may contain warning message.
		std::cout << err << std::endl;
		//std::cin.ignore();
	}
	if (!ret) {
		exit(1);
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
	unsigned char* image = SOIL_load_image("concrete2.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.

	// Load and create a texture 
	GLuint normal;
	// ====================
	// Normalmap
	// ====================
	glGenTextures(1, &normal);
	glBindTexture(GL_TEXTURE_2D, normal); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
											// Set our texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load, create texture and generate mipmaps
	image = SOIL_load_image("concrete2_n.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.

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
		std::string name;
		std::vector<GLfloat> data;
	};

	std::vector<mesh> objects;
	objects.resize(shapes.size());
	int dataSize = 8+3;
	for (int i = 0; i < shapes.size(); ++i)
	{
		objects[i].name = shapes[i].name;
		objects[i].data.resize(shapes[i].mesh.num_face_vertices.size()*3*(dataSize + 0));
		
		size_t index_offset = 0;
		for (size_t j = 0; j < shapes[i].mesh.num_face_vertices.size(); ++j)
		{
			int fv = shapes[i].mesh.num_face_vertices[j];
			unsigned int dataIndOuter = j * fv * (dataSize + 0);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
				unsigned int dataInd = dataIndOuter + (v * dataSize);
				//int dataIndInt = (v*dataSize) + dataInd;

				//Positions
				objects[i].data[dataInd + 0] = attrib.vertices[3 * idx.vertex_index + 0];
				objects[i].data[dataInd + 1] = attrib.vertices[3 * idx.vertex_index + 1];
				objects[i].data[dataInd + 2] = attrib.vertices[3 * idx.vertex_index + 2];
				//Normals
				objects[i].data[dataInd + 3] = attrib.normals[3 * idx.normal_index + 0];
				objects[i].data[dataInd + 4] = attrib.normals[3 * idx.normal_index + 1];
				objects[i].data[dataInd + 5] = attrib.normals[3 * idx.normal_index + 2];
				//Texture
				if (idx.texcoord_index != -1)
				{
					objects[i].data[dataInd + 6] = attrib.texcoords[2 * idx.texcoord_index + 0];
					objects[i].data[dataInd + 7] = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else
				{
					objects[i].data[dataInd + 6] = 0.0f;
					objects[i].data[dataInd + 7] = 0.0f;
				}
				//Tangent DEBUG
				objects[i].data[dataInd + 8] = 66.6f;
				objects[i].data[dataInd + 9] = 66.6f;
				objects[i].data[dataInd + 10] = 66.6f;
			}
			index_offset += fv;
		}

		//Insert Tangents
		for (int i = 0; i < objects.size(); ++i)
		{
			int debug = 0;
			for (int j = 0; j < objects[i].data.size(); j+=33)
			{
				glm::vec3 tangent;

				debug++;
				//Tangent
				glm::vec3 vert[3];
				glm::vec2 uv[3];

				for (int o = 0; o < 3; ++o)
			{
					int indx = 11 * o;
					vert[o].x = objects[i].data[j + indx + 0];
					vert[o].y = objects[i].data[j + indx + 1];
					vert[o].z = objects[i].data[j + indx + 2];

					uv[o].x = objects[i].data[j + indx + 6];
					uv[o].y = objects[i].data[j + indx + 7];
				}

				FishGL::calcTangents(vert, uv, tangent);

				for (int o = 0; o < 3; ++o)
				{
					int indx = 11 * o;
					objects[i].data[j + indx + 8] = tangent[0];
					objects[i].data[j + indx + 9] = tangent[1];
					objects[i].data[j + indx + 10] = tangent[2];
				}
			}
			DEBUG(debug);
		}
	}

	Shader* main_shader = engine.addShader("VertexShader.glsl", "FragmentShader.glsl");

	short dist = 5;
	std::vector<sceneobj> scene;
	scene.resize(objects.size());
	for (GLuint i = 0; i < objects.size(); i++)
	{
		//scene[i].VAO = VAO;
		//engine.addObject(attrib.vertices.data(), attrib.vertices.size(), objects[i].indices, objects[i].size, VBO, scene[i].VAO, scene[i].EBO);
		//engine.addObjectWithNormals(objects[i].data, scene[i].VAO);
		engine.addObjectWithTangents(objects[i].data, scene[i].VAO);
		scene[i].iCount = objects[i].data.size() / 8;
		scene[i].scale = 0.5f;
		scene[i].shader = main_shader;
		//scene[i].position = glm::vec3((rand() % dist) - dist / 2, (rand() % dist) - dist / 2, (rand() % dist) - dist / 2);
		scene[i].position = glm::vec3(0, -2.0f, 0);
		scene[i].texture = texture1;
		scene[i].normal = normal;
	}
	//*****

	engine.addObjectsToScene(scene.data(), objects.size());

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
	/*glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);*/

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
