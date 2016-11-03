#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Shader.h"

struct renderObj {
	renderObj()
		: VAO(0), textureID(0), iCount(0)
	{};
	GLuint VAO;
	int iCount;
	GLuint textureID;
};

struct sceneobj {
	GLuint VAO;
	int iCount;
	Shader* shader;
	glm::vec3 color;
	glm::vec3 position;
	float scale;
	float dist;
	GLuint texture;

	bool operator < (const sceneobj& t) const
	{
		return (dist < t.dist);
	}
};

struct keyframe {
	glm::vec3 position;
	glm::quat rotation;
};

struct animation {
	keyframe* frames;
	int count;
	float duration;
};

struct camera {
	glm::vec3 position;
	glm::quat rotation;
};

struct light {
	glm::vec3 position;
	glm::vec3 color;
};

class FishGL
{
public:
	FishGL();
	~FishGL();
	GLFWwindow* createWindow(int width, int height);
	void Run();
	void key_callback(int key, int action);
	void mouse_callback(double xpos, double ypos);
	void addObject(GLfloat* vertices, int vSize, GLuint& vao);
	void addObject(GLfloat* vertices, int vSize, GLuint* indices, int iSize, GLuint& vbo, GLuint& vao, GLuint& ebo);
	void addObjectWithNormals(std::vector<GLfloat>& data, GLuint& vao);
	Shader* addShader(const char* vertex, const char* fragment);
	void setPerspective(float fovy, float aspect, float near, float far);
	glm::mat4 getPerspective();
	void addObjectToScene(sceneobj& obj);
	void addObjectsToScene(sceneobj* obj, int size);
	void addAnimation(animation* anim);
	void runAnimation(glm::vec3& pos, glm::quat& rot);
	void drawAnimation(glm::mat4& view);
	glm::vec3* getAnimation(int resolution);
private:
	GLFWwindow* m_window;
	std::vector<GLuint> m_vbo, m_vao;
	std::vector<Shader> m_shaders;
	std::vector<sceneobj> m_scene;
	glm::mat4 m_projection;
	glm::vec2 mouse;
	camera m_camera;
	light m_light;
	double dt;
	animation* m_animation;
	bool m_freemode,
		m_drawAnimation,
		m_lightCam;
	double m_animation_start;
	int anim_count;
	glm::vec3* m_animationPoints;
	int m_AnimResolution;
};
