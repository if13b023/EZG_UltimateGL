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
	glBindTexture(GL_TEXTURE_2D, texture1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
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
	glBindTexture(GL_TEXTURE_2D, 0);

	// Load and create a texture 
	GLuint normal;
	// ====================
	// Normalmap
	// ====================
	glGenTextures(1, &normal);
	glBindTexture(GL_TEXTURE_2D, normal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Load, create texture and generate mipmaps
	image = SOIL_load_image("concrete2_n.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	//sample preps
	std::vector<Triangle> triangles;
	triangles.reserve(16 * shapes.size());
	std::vector<Mesh> objects;
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
			Triangle tmpT;
			tmpT.parentObj = shapes[i].name;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) 
			{
				// access to vertex
				tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
				unsigned int dataInd = dataIndOuter + (v * dataSize);

				//Positions
				objects[i].data[dataInd + 0] = attrib.vertices[3 * idx.vertex_index + 0];
				objects[i].data[dataInd + 1] = attrib.vertices[3 * idx.vertex_index + 1];
				objects[i].data[dataInd + 2] = attrib.vertices[3 * idx.vertex_index + 2];

				tmpT.vertices[v] = glm::vec3(objects[i].data[dataInd + 0], objects[i].data[dataInd + 1], objects[i].data[dataInd + 2]);

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

			tmpT.calcMedian();
			triangles.push_back(tmpT);
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
			//DEBUG(debug);
		}
	}

	Shader* main_shader = engine.addShader("VertexShader.glsl", "FragmentShader.glsl");

	short dist = 5;
	std::vector<sceneobj*> scene;
	scene.reserve(objects.size());
	//

	for (GLuint i = 0; i < objects.size(); i++)
	{
		scene.push_back(new sceneobj());
		engine.addObjectWithTangents(objects[i].data, scene[i]->VAO);
		scene[i]->iCount = objects[i].data.size() / 8;
		scene[i]->meshPtr = &objects[i];
		scene[i]->scale = 1.f;
		scene[i]->shader = main_shader;
		scene[i]->position = glm::vec3(0, 0.0f, 0);
		scene[i]->texture = texture1;
		scene[i]->normal = normal;
		scene[i]->color = glm::vec3(1.0f, 0.f, 0.f);
		scene[i]->simple = false;
		scene[i]->triangles = true;

		scene[i]->calcOrigin();
	}
	//*****

	engine.addObjectsToScene(scene.data(), objects.size());

	engine.addTriangles(triangles);
	engine.calcKdTree();

	//Animation
	animation anim;
	int size = 10;
	anim.duration = 10.0f;
	anim.count = size;
	keyframe* frame = new keyframe[size];

	frame[0].position = glm::vec3(-8.30853f, 3.05011f, 1.5966f);
	frame[0].rotation = glm::quat(0.301561f, -0.010715f, 0.952786f, -0.0338539f);
	frame[1].position = glm::vec3(-5.53763f, 3.06814f, 4.42721f);
	frame[1].rotation = glm::quat(0.493492f, 0.0182703f, 0.868963f, 0.0321713f);
	frame[2].position = glm::vec3(-1.52548f, 2.4424f, 5.74578f);
	frame[2].rotation = glm::quat(0.654132f, 0.076869f, 0.747322f, 0.08782f);
	frame[3].position = glm::vec3(2.2084f, 1.21507f, 5.3302f);
	frame[3].rotation = glm::quat(0.825496f, 0.145509f, 0.53705f, 0.0946649f);
	frame[4].position = glm::vec3(4.86051f, 0.170377f, 2.86173f);
	frame[4].rotation = glm::quat(0.938458f, 0.107891f, 0.325963f, 0.0374747f);
	frame[5].position = glm::vec3(6.59778f, -0.287681f, -1.6676f);
	frame[5].rotation = glm::quat(0.997986f, -0.0579962f, -0.0256593f, 0.00149121f);
	frame[6].position = glm::vec3(5.33945f, 0.63067f, -5.61334f);
	frame[6].rotation = glm::quat(0.94542f, -0.145819f, -0.288001f, 0.0444206f);
	frame[7].position = glm::vec3(1.41755f, 2.09333f, -8.68855f);
	frame[7].rotation = glm::quat(0.804603f, -0.008478f, -0.59372f, 0.00625599f);
	frame[8].position = glm::vec3(-2.90104f, 1.37972f, -8.81741f);
	frame[8].rotation = glm::quat(0.528989f, 0.095956f, -0.829647f, -0.150494f);
	frame[9].position = glm::vec3(-5.34199f, -0.193141f, -5.30862f);
	frame[9].rotation = glm::quat(0.0597976f, 0.00536179f, -0.994208f, -0.0891456f);

	anim.frames = frame;

	engine.addAnimation(&anim);
	//****

	engine.Run();

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
