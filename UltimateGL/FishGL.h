#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Shader.h"
#include "SOIL.h"

#define DEBUG(x) std::cout << #x << " = " << x << std::endl;

struct Mesh
{
	std::string name;
	std::vector<GLfloat> data;
};

struct sceneobj {
	GLuint VAO;
	GLsizei iCount;
	Shader* shader;
	glm::vec3 color;
	glm::vec3 position;
	glm::vec3 origin;
	float scale;
	float dist;
	bool simple, triangles;
	GLuint texture;
	GLuint normal;
	Mesh* meshPtr;

	bool operator < (const sceneobj& t) const
	{
		return (dist < t.dist);
	}

	void calcOrigin();
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

struct shadow {
	GLuint size;
	GLuint depthFBO;
	GLuint depthTex;
	Shader* shader;
};

struct BoundingBox {
	glm::vec3 min, max;
};

struct Triangle {
	glm::vec3 vertices[3];
	glm::vec3 median;
	std::string parentObj;

	void calcMedian();
	float isHit(glm::vec3 rOrigin, glm::vec3 rDirection, float length);
};

struct kdNode {
	int axis;
	float value;
	BoundingBox bbox;
	kdNode* left;
	kdNode* right;
	std::vector<int> leaf;

	void calcBoundingBox(std::vector<int>& triangleIDs, std::vector<Triangle>& triangles);
	bool isHit(glm::vec3 rOrigin, glm::vec3 rDirection, float length);
};

struct ivec2
{
	int x, y;
};

class FishGL
{
public:
	FishGL();
	~FishGL();

	//callbacks
	void key_callback(int key, int action);
	void mouse_callback(double xpos, double ypos);

	GLFWwindow* createWindow(int width, int height);
	void Run();
	void calcKdTree();

	//add geometry
	void addObject(GLfloat* vertices, int vSize, GLuint& vao);
	void addObject(GLfloat* vertices, int vSize, GLuint* indices, int iSize, GLuint& vbo, GLuint& vao, GLuint& ebo);
	void addObjectWithNormals(std::vector<GLfloat>& data, GLuint& vao);
	void addObjectWithTangents(std::vector<GLfloat>& data, GLuint & vao);
	void addObjectToScene(sceneobj* obj);
	void addObjectsToScene(sceneobj** obj, size_t size);
	Shader* addShader(const char* vertex, const char* fragment);
	void addLine(glm::vec3 start, glm::vec3 direction, float length = 100.f);
	void addHit(glm::vec3 center, float size);
	void addTriangles(std::vector<Triangle>& triangles);

	//animation
	void addAnimation(animation* anim);
	void runAnimation(glm::vec3& pos, glm::quat& rot);
	void drawAnimation(glm::mat4& view);
	glm::vec3* getAnimation(int resolution);
	void setPerspective(float fovy, float aspect, float near, float far);	

	//statics
	static void calcTangents(glm::vec3* vert, glm::vec2* uv, glm::vec3& t);

	//const
	glm::mat4 getPerspective() const;

private:
	bool m_shadowSwitch, m_AA, m_debug;
	int m_AASamples;
	float m_normalFactor,
			m_fps;
	GLFWwindow* m_window;
	GLuint m_framebuffer, m_rbo, m_textureColorBufferMultiSampled;
	glm::ivec2 m_size;
	std::vector<GLuint> m_vbo, m_vao;
	std::vector<Shader*> m_shaders;
	std::vector<sceneobj*> m_scene;
	std::vector<sceneobj*> m_debugScene;
	glm::mat4 m_projection;
	glm::vec2 mouse;
	camera m_camera;
	light m_light;
	float dt;
	animation* m_animation;
	bool m_freemode,
		m_drawAnimation,
		m_lightCam;
	double m_animation_start;
	int anim_count;
	glm::vec3* m_animationPoints;
	int m_AnimResolution;
	shadow m_shadow;
	FT_Library m_ftlib;
	kdNode* m_kdRoot;
	std::vector<Triangle> m_triangles;
	ivec2 m_displacementSteps;

	//DEBUG
	Shader* m_lineShader;
	int m_lineId, m_hitId;
	GLuint m_lineVBO;

	//internals
	void i_renderScene(std::vector<sceneobj*>& scene, glm::mat4& view, bool isShadow = false);
	void i_initShadow();
	GLuint i_generateMultiSampleTexture(GLuint samples);
	void i_generateNewFrameBuffer();
	kdNode* i_kdTree(int axis, std::vector<int> objects);
	float i_findMedian(int axis, std::vector<int> objects);
};

void glHandleError(const char* info = "");
