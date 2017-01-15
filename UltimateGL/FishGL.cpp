#include "FishGL.h"

FishGL::FishGL()
	:m_freemode(true),
	m_animation(nullptr),
	anim_count(0),
	m_drawAnimation(true),
	m_animationPoints(nullptr),
	m_AnimResolution(50),
	m_lightCam(false),
	m_shadowSwitch(true),
	m_normalFactor(.5f),
	m_AA(false),
	m_AASamples(1),
	m_lineId(-1),
	m_hitId(-1),
	m_debug(true)
{
	glfwInit();
	m_shaders.reserve(16);
	m_vao.reserve(16);
	m_vbo.reserve(16);
	m_camera.rotation = glm::quat(glm::vec3(0, glm::radians(0.0f), 0));
	m_light.position = glm::vec3(0.f, 5.0f, 0.f);
	m_light.color = glm::vec3(1.f, 1.0f, 1.f);

	if (FT_Init_FreeType(&m_ftlib)) {
		std::cout << "Could not init freetype library\n";
		exit(666);
	}
}

FishGL::~FishGL()
{
	delete[] m_animationPoints;
}

GLFWwindow * FishGL::createWindow(int width, int height)
{
	//AntiAliasing
	if (m_AA)
	{
		glEnable(GL_MULTISAMPLE);
	}else
		glDisable(GL_MULTISAMPLE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	m_window = glfwCreateWindow(width, height, "FishGL", nullptr, nullptr);
	if (m_window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(m_window);
	glGetError();

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		std::cin.ignore();
		exit(1);
	}

	glfwGetFramebufferSize(m_window, &m_size.x, &m_size.y);

	glViewport(0, 0, m_size.x, m_size.y);

	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	i_generateNewFrameBuffer();

	return m_window;
}

GLuint FishGL::i_generateMultiSampleTexture(GLuint samples)
{
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glDeleteTextures(1, &m_textureColorBufferMultiSampled);

	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, m_size.x, m_size.y, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	return texture;
}

void FishGL::i_generateNewFrameBuffer()
{
	std::cout << "Samples " << m_AASamples << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glDeleteFramebuffers(1, &m_framebuffer);
	glDeleteRenderbuffers(1, &m_rbo);

	//FBO
	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	// Create a multisampled color attachment texture
	m_textureColorBufferMultiSampled = i_generateMultiSampleTexture(m_AASamples);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_textureColorBufferMultiSampled, 0);
	// Create a renderbuffer object for depth and stencil attachments
	glGenRenderbuffers(1, &m_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_AASamples, GL_DEPTH24_STENCIL8, m_size.x, m_size.y);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FishGL::Run()
{
	float lastframe = static_cast<float>(glfwGetTime());
	dt = 0.014f;

	//init shadows
	i_initShadow();

	m_lineShader = m_shaders[1];

	//std::cin.ignore();//to test kdTree memory allocation
	calcKdTree();

	while (!glfwWindowShouldClose(m_window))
	{
		glGetError();
		glEnable(GL_DEPTH_TEST);
		glm::mat4 m_view;

		dt = static_cast<float>(glfwGetTime()) - lastframe;
		lastframe = static_cast<float>(glfwGetTime());
		m_fps = 1.f / dt;

		if (m_freemode && m_animation != nullptr)
		{
			float pi = 3.1415f;
			float moveSpeed = 0.1f;
			glm::vec3 forwardVec(0, 0, moveSpeed),
				sideVec(moveSpeed, 0, 0);

			if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			{
				m_camera.position -= forwardVec * m_camera.rotation;
			}
			else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			{
				m_camera.position += forwardVec * m_camera.rotation;
			}

			if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
				if (glfwGetKey(m_window, GLFW_KEY_N) == GLFW_PRESS && m_normalFactor > 0.f)
					m_normalFactor -= 0.5f * dt;
				else
					m_camera.position -= sideVec * m_camera.rotation;
			else if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
				if (glfwGetKey(m_window, GLFW_KEY_N) == GLFW_PRESS && m_normalFactor < 1.f)
					m_normalFactor += 0.5f * dt;
				else
					m_camera.position += sideVec * m_camera.rotation;

			

			glm::mat4 translate = glm::mat4(1.0f);
			m_view = glm::translate(m_view, -m_camera.position);
			m_view = glm::mat4_cast(m_camera.rotation) * m_view;
		}
		else {
			glm::vec3 pos;
			glm::quat rot;

			runAnimation(pos, rot);
			m_camera.position = pos;
			m_camera.rotation = -rot;
			m_view = glm::toMat4(m_camera.rotation) * glm::translate(m_view, -m_camera.position);
		}
		//keyboard events
		glfwPollEvents();

		if (m_lightCam)
			m_light.position = m_camera.position;

		//shadow stuff
		glViewport(0, 0, m_shadow.size, m_shadow.size);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadow.depthFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		i_renderScene(m_scene, m_view, true);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_size.x, m_size.y); //reset viewport


		//color render
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
		glClearColor(0.69f, 0.69f, 0.69f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		i_renderScene(m_scene, m_view);

		//render debug scene
		if(m_debug)
			i_renderScene(m_debugScene, m_view);

		//if(m_drawAnimation && false)
		//	drawAnimation(m_view);

		// 2. Now blit multisampled buffer(s) to default framebuffers
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, m_size.x, m_size.y, 0, 0, m_size.x, m_size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glHandleError("post swap buffers");

		//swap buffers
		glfwSwapBuffers(m_window);
	}
}

void FishGL::calcKdTree()
{
	std::vector<int> treeScene;
	treeScene.resize(m_triangles.size());
	for (int i = 0; i < m_triangles.size(); ++i)
	{
		treeScene[i] = i;
	}
	
	m_kdRoot = i_kdTree(0, treeScene);
}

kdNode * FishGL::i_kdTree(int axis, std::vector<int> objects)
{
	kdNode* newNode = new kdNode();

	newNode->calcBoundingBox(objects, m_triangles);

	if (objects.size() <= 12)//break if one cube fits into the leaf
	{
		newNode->left = newNode->right = nullptr;
		newNode->leaf = objects;
		return newNode;
	}
	else if (objects.size() > 12)
	{
		newNode->axis = axis;
		newNode->value = i_findMedian(axis, objects);
		std::vector<int> leftTree, rightTree;
		for (int t : objects)
		{
			if (m_triangles[t].median[axis] < newNode->value)
			{
				leftTree.push_back(t);
			}
			else if (m_triangles[t].median[axis] > newNode->value)
			{
				rightTree.push_back(t);
			}
			else
				std::cout << "MEH!\n";
		}

		newNode->left = i_kdTree((axis < 2) ? axis + 1 : 0, leftTree);
		newNode->right = i_kdTree((axis < 2) ? axis + 1 : 0, rightTree);

		return newNode;
	}else
		return nullptr;
}

float FishGL::i_findMedian(int axis, std::vector<int> objects)
{
	float mean(0.f);

	for (int t : objects)
	{
		mean += m_triangles[t].median[axis];
	}

	mean /= objects.size();

	return mean;
}

void FishGL::key_callback(int key, int action)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
			case GLFW_KEY_ESCAPE: 
				glfwSetWindowShouldClose(m_window, GL_TRUE);
				break;
			case GLFW_KEY_T:
				m_freemode = !m_freemode;
				m_animation_start = glfwGetTime();
				DEBUG(m_freemode);
				break;
			case GLFW_KEY_R:
				std::cout << "frame[" << anim_count << "].position = glm::vec3(" << m_camera.position.x << "f, " << m_camera.position.y << "f, " << m_camera.position.z << "f);" << std::endl;
				std::cout << "frame[" << anim_count << "].rotation = glm::quat(" << m_camera.rotation.w << "f, " << m_camera.rotation.x << "f, " << m_camera.rotation.y << "f, " << m_camera.rotation.z << "f);" << std::endl;
				m_animation->frames[anim_count].position = m_camera.position;
				m_animation->frames[anim_count].rotation = m_camera.rotation;
				anim_count++;
				if (anim_count == 10)
					anim_count = 0;

				m_animationPoints = getAnimation(m_AnimResolution);

				break;
			case GLFW_KEY_Q:
			{
				std::cout << "Camera Position: " << m_camera.position.x << "|" << m_camera.position.y << "|" << m_camera.position.z << std::endl;
				std::cout << m_camera.rotation.y << "|" << m_camera.rotation.y << "|" << m_camera.rotation.z << std::endl;
				glm::vec3 rayRay = glm::vec3(0, 0, -1.f) * m_camera.rotation;
				addLine(m_camera.position, rayRay);

				Triangle* closestT = nullptr;
				float closest = HUGE_VALF;
				for (Triangle& t : m_triangles)
				{
					float hit = t.isHit(m_camera.position, rayRay, 100.f);
					if (hit > 0.f)
					{
						if (hit < closest)
						{
							closest = hit;
							closestT = &t;
						}
					}
				}
/*
				Triangle* closestT = nullptr;
				float closest = HUGE_VALF;
				bool testShot = false;
				kdNode* node = m_kdRoot;
				do{
					testShot = node->isHit(m_camera.position, rayRay, 100.f);
					if (testShot && node->leaf.empty())
					{
						if (node->left->isHit(m_camera.position, rayRay, 100.f))
							node = node->left;
						else if (node->right->isHit(m_camera.position, rayRay, 100.f))
							node = node->right;
						else
						{
							std::cout << "WAT!?\n";
							break;
						}
					}
					else if (testShot && !node->leaf.empty())
					{
						for (int id : node->leaf)
						{
							float hit = m_triangles[id].isHit(m_camera.position, rayRay, 100.f);
							if (hit > 0.f)
							{
								if (hit < closest)
								{
									closest = hit;
									closestT = &m_triangles[id];
								}
							}
						}
						break;
					}
					else
					{
						std::cout << "MISS...\n";
						break;
					}
				} while (true);*/

				if (closestT != nullptr)
				{
					addHit(m_camera.position + (rayRay * closest), .1f);
					addLine(m_camera.position, rayRay, closest);
					std::cout << "HIT@" << closest << " -> " << closestT->parentObj << std::endl;
				}
				else
					std::cout << "MISS\n";
			}
				break;
			case GLFW_KEY_Z:
				m_drawAnimation = !m_drawAnimation;
				break;
			case GLFW_KEY_L:
				m_lightCam = !m_lightCam;
				DEBUG(m_lightCam);
				break;
			case GLFW_KEY_P:
				m_shadowSwitch = !m_shadowSwitch;
				DEBUG(m_shadowSwitch);
				break;
			case GLFW_KEY_N:
				DEBUG(m_normalFactor);
				break;
			case GLFW_KEY_I:
				i_generateNewFrameBuffer();
				DEBUG(m_fps);
				break;

			case GLFW_KEY_1: m_AASamples = 2;
				break;
			case GLFW_KEY_2: m_AASamples = 4;
				break;
			case GLFW_KEY_3: m_AASamples = 8;
				break;
			case GLFW_KEY_4: m_AASamples = 16;
				break;
			case GLFW_KEY_5: m_AASamples = 32;
				break;
		}
	}
}

void FishGL::mouse_callback(double xpos, double ypos)
{
	const GLfloat sens = 0.3f;
	glm::vec2 diff = glm::vec2(xpos, ypos) - mouse;
	glm::quat yaw = glm::quat(glm::vec3(diff.y * dt * sens, 0.0f, 0.0f));
	glm::quat pitch = glm::quat(glm::vec3(0.0f, diff.x * dt * sens, 0.0f));
	m_camera.rotation = glm::normalize(yaw * m_camera.rotation * pitch);
	mouse.x = xpos;
	mouse.y = ypos;
}

void FishGL::addObject(GLfloat * vertices, int vSize, GLuint & vao)
{
	GLuint vbo;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vSize * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);

	//CLEANUP
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	m_vao.push_back(vao);
	m_vbo.push_back(vbo);
}

void FishGL::addObject(GLfloat* vertices, int vSize, GLuint* indices, int iSize, GLuint& vbo, GLuint& vao, GLuint& ebo)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vSize * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);

	//CLEANUP
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	m_vao.push_back(vao);
	m_vbo.push_back(vbo);
}

void FishGL::addObjectWithNormals(std::vector<GLfloat>& data, GLuint & vao)
{
	GLuint vbo;

	glGenVertexArrays(1, &vao);
	
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

	glBindVertexArray(vao);
	//Positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	//Normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	////UVs
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// reset bindings for VAO and VBO
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FishGL::addObjectWithTangents(std::vector<GLfloat>& data, GLuint & vao)
{
	GLuint vbo;

	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

	glBindVertexArray(vao);
	//Positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	//Normals
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	////UVs
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	//Tangents
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	// reset bindings for VAO and VBO
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Shader * FishGL::addShader(const char * vertex, const char * fragment)
{
	m_shaders.push_back(new Shader(vertex, fragment));
	return m_shaders.back();
}

void FishGL::setPerspective(float fovy, float aspect, float near, float far)
{
	m_projection = glm::perspective(fovy, aspect, near, far);
}

glm::mat4 FishGL::getPerspective() const
{
	return m_projection;
}

void FishGL::addObjectToScene(sceneobj* obj)
{
	m_scene.push_back(obj);
}

void FishGL::addObjectsToScene(sceneobj** obj, size_t size)
{
	for(int i = 0; i < size; ++i)
		m_scene.push_back(obj[i]);
}

void FishGL::addAnimation(animation* anim)
{
	m_animation = anim;
	m_animationPoints = getAnimation(m_AnimResolution);
}

void FishGL::runAnimation(glm::vec3 & pos, glm::quat & rot)
{
	float anim_time = static_cast<float>(glfwGetTime()) - m_animation_start;
	float percent = (anim_time / m_animation->duration)*m_animation->count;
	int index = int(floor(percent));

	if (index >= m_animation->count)
	{
		std::cout << "ERROR: Animation Percentage Over 1\n";
		m_animation_start = glfwGetTime();
		return;
	}

	int i[4];
	i[0] = index - 1;
	i[1] = index;
	i[2] = index + 1;
	i[3] = index + 2;

	glm::vec3 points[4];
	glm::quat rots[4];

	for (int j = 0; j < 4; ++j)
	{
		if (i[j] < 0)
			i[j] += (m_animation->count - 1);
		else if (i[j] >= m_animation->count)
		{
			i[j] -= (m_animation->count);
		}
	}

	points[0] = m_animation->frames[i[0]].position;
	points[1] = m_animation->frames[i[1]].position;
	points[2] = m_animation->frames[i[2]].position;
	points[3] = m_animation->frames[i[3]].position;
	
	rots[0] = m_animation->frames[i[0]].rotation;
	rots[1] = m_animation->frames[i[1]].rotation;
	rots[2] = m_animation->frames[i[2]].rotation;
	rots[3] = m_animation->frames[i[3]].rotation;

	//Position
	pos = glm::catmullRom(points[0], points[1], points[2], points[3], glm::fract(percent));
	//***

	//Rotation
	glm::quat rot_int1 = glm::intermediate(rots[0], rots[1], rots[2]);
	glm::quat rot_int2 = glm::intermediate(rots[1], rots[2], rots[3]);
	rot = glm::squad(rots[1], rots[2], rot_int1, rot_int2, glm::fract(percent));
	//rot = glm::mix(rots[1], rots[2], glm::fract(percent));
	//rot = glm::lerp(rots[1], rots[2], glm::fract(percent));
	//***
}

void FishGL::drawAnimation(glm::mat4& view)
{
	/*
	m_scene[0].shader->Use();
	GLuint viewId = glGetUniformLocation(m_scene[0].shader->Program, "view");
	glBindVertexArray(m_scene[0].VAO);

	for (int i = 0; i < m_AnimResolution; ++i)
	{
		glm::mat4 model;
		model = glm::translate(model, -m_animationPoints[i]);
		model = glm::scale(model, glm::vec3(m_scene[0].scale/2, m_scene[0].scale/2, m_scene[0].scale/2));

		glUniformMatrix4fv(m_scene[0].shader->transId, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewId, 1, GL_FALSE, glm::value_ptr(view));
		//glUniform4f(m_scene[i].shader->fragmentColorId, (m_scene[i].position.r + 5) / 10, (m_scene[i].position.g + 5) / 10, (m_scene[i].position.b + 5) / 10, 1.0f);
		glUniform4f(m_scene[0].shader->fragmentColorId, 0.0f, 0, 0, 1.0f);

		glDrawElements(GL_TRIANGLES, m_scene[0].iCount, GL_UNSIGNED_INT, 0);
	}

	for (int i = 0; i < m_animation->count; ++i)
	{
		glm::mat4 model;
		model = glm::translate(model, -m_animation->frames[i].position);
		model = glm::scale(model, glm::vec3(m_scene[0].scale, m_scene[0].scale, m_scene[0].scale));

		glUniformMatrix4fv(m_scene[0].shader->transId, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewId, 1, GL_FALSE, glm::value_ptr(view));
		//glUniform4f(m_scene[i].shader->fragmentColorId, (m_scene[i].position.r + 5) / 10, (m_scene[i].position.g + 5) / 10, (m_scene[i].position.b + 5) / 10, 1.0f);
		//glUniform4f(m_scene[0].shader->fragmentColorId, 1.0f, 0, 0, 1.0f);
		glUniform4f(m_scene[0].shader->fragmentColorId, m_animation->frames[i].rotation.x, m_animation->frames[i].rotation.y, m_animation->frames[i].rotation.z, 1.0f);

		glDrawArrays(GL_TRIANGLES, 0, m_scene[i].iCount);
		//glDrawElements(GL_TRIANGLES, m_scene[0].iCount, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);*/
}

glm::vec3* FishGL::getAnimation(int resolution)
{
	glm::vec3* points = new glm::vec3[resolution];

	for (int i = 0; i < resolution; ++i)
	{
		float percent = ((float)i / resolution)*m_animation->count;
		int index = int(floor(percent));

		int ind[4];
		ind[0] = index - 1;
		ind[1] = index;
		ind[2] = index + 1;
		ind[3] = index + 2;

		glm::vec3 pos[4];

		for (int j = 0; j < 4; ++j)
		{
			if (ind[j] < 0)
				ind[j] += (m_animation->count - 1);
			else if (ind[j] >= m_animation->count)
			{
				ind[j] -= (m_animation->count);
			}
		}

		pos[0] = m_animation->frames[ind[0]].position;
		pos[1] = m_animation->frames[ind[1]].position;
		pos[2] = m_animation->frames[ind[2]].position;
		pos[3] = m_animation->frames[ind[3]].position;

		//Position
		points[i] = glm::catmullRom(pos[0], pos[1], pos[2], pos[3], glm::fract(percent));
		//***
	}

	return points;
}

void FishGL::addLine(glm::vec3 start, glm::vec3 direction, float length)
{
	sceneobj* line;
	if (m_lineId == -1)
	{
		line = new sceneobj();
		line->shader = m_shaders[0];
		line->iCount = 6;
		line->color = glm::vec3(0.f, 0.f, 1.f);
		line->position = glm::vec3(0.f, 0.f, 0.f);
		line->scale = 1.0f;
		line->simple = true;
		line->triangles = false;
	}
	else
	{
		line = m_debugScene[m_lineId];
	}

	glm::vec3 end = start + (direction * length);
	GLfloat vertices[] = {
		start.x, start.y, start.z,
		end.x, end.y, end.z
	};

	addObject(vertices, 6, line->VAO);
	if (m_lineId == -1)
	{
		m_debugScene.push_back(line);
		m_lineId = m_debugScene.size() - 1;
	}
}

void FishGL::addHit(glm::vec3 center, float size)
{
	sceneobj* hit;
	if (m_hitId == -1)
	{
		hit = new sceneobj();
		hit->shader = m_shaders[0];
		hit->iCount = m_scene[0]->iCount;
		hit->origin = m_scene[0]->origin;
		hit->color = glm::vec3(0.f, 1.f, 1.f);
		hit->simple = true;
		hit->triangles = true;
		hit->VAO = m_scene[0]->VAO;
	}
	else
	{
		hit = m_debugScene[m_hitId];
	}

	hit->scale = size;
	hit->position = center - (hit->origin * hit->scale);

	if (m_hitId == -1)
	{
		m_debugScene.push_back(hit);
		m_hitId = m_debugScene.size() - 1;
	}
}

void FishGL::addTriangles(std::vector<Triangle>& triangles)
{
	m_triangles = triangles;
}

void FishGL::calcTangents(glm::vec3 * vert, glm::vec2 * uv, glm::vec3 & t)
{
	glm::vec3 edge1 = vert[1] - vert[0];
	glm::vec3 edge2 = vert[2] - vert[0];
	glm::vec2 deltaUV1 = uv[1] - uv[0];
	glm::vec2 deltaUV2 = uv[2] - uv[0];

	GLfloat n = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
	GLfloat f = 1.f;
	if(abs(n) > 0.001f)
		f = 1.0f / n;

	t.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	t.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	t.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
	t = glm::normalize(t);
}

void FishGL::i_renderScene(std::vector<sceneobj*>& scene, glm::mat4& m_view, bool isShadow)
{
	GLfloat near_plane = 1.0f, far_plane = 20.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
	glm::mat4 lightView = glm::lookAt(glm::vec3(m_light.position.x, 10.0f, m_light.position.z),
		glm::vec3(0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightMatrix = lightProjection * lightView;

	for (GLuint i = 0; i < scene.size(); i++)
	{
		Shader* shader;
		if (!isShadow)
		{
			shader = scene[i]->shader;
		}
		else
			shader = m_shadow.shader;

		shader->Use();
		if (!isShadow/* && false*/)
		{
			glUniform1i(glGetUniformLocation(shader->Program, "simple"), scene[i]->simple);
			if (scene[i]->simple == false)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, scene[i]->texture);
				glUniform1i(glGetUniformLocation(shader->Program, "mainTexture"), 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, m_shadow.depthTex);
				glUniform1i(glGetUniformLocation(shader->Program, "depthMap"), 1);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, scene[i]->normal);
				glUniform1i(glGetUniformLocation(shader->Program, "normalMap"), 2);

				glUniform1i(glGetUniformLocation(shader->Program, "shadowSwitch"), m_shadowSwitch);
				glUniform1f(glGetUniformLocation(shader->Program, "normalFactor"), m_normalFactor);
			}

			glUniform3f(glGetUniformLocation(shader->Program, "objColor"), scene[i]->color.r, scene[i]->color.g, scene[i]->color.b);
			glUniform3f(glGetUniformLocation(shader->Program, "lightColor"), m_light.color.r, m_light.color.g, m_light.color.b);
			glUniform3fv(glGetUniformLocation(shader->Program, "lightPos"), 1, &m_light.position[0]);
			glUniform3fv(glGetUniformLocation(scene[i]->shader->Program, "viewPos"), 1, &m_camera.position[0]);
			glUniformMatrix4fv(glGetUniformLocation(scene[i]->shader->Program, "view"), 1, GL_FALSE, glm::value_ptr(m_view));
		}

		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightMatrix"), 1, GL_FALSE, glm::value_ptr(lightMatrix));

		glBindVertexArray(scene[i]->VAO);

		glm::mat4 transform;
		transform = glm::translate(transform, scene[i]->position);
		transform = glm::scale(transform, glm::vec3(scene[i]->scale));

		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, glm::value_ptr(m_projection));
		if(scene[i]->triangles)
			glDrawArrays(GL_TRIANGLES, 0, scene[i]->iCount);
		else
			glDrawArrays(GL_LINES, 0, scene[i]->iCount);

		glBindVertexArray(0);
	}
}

void FishGL::i_initShadow()
{
	glGenFramebuffers(1, &m_shadow.depthFBO);

	m_shadow.size = 2048;

	glGenTextures(1, &m_shadow.depthTex);
	glBindTexture(GL_TEXTURE_2D, m_shadow.depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_shadow.size, m_shadow.size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, m_shadow.depthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadow.depthTex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_shadow.shader = addShader("VertexShader_Shadow.glsl", "FragmentShader_Shadow.glsl");
}

void glHandleError(const char* info)
{
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
	{
		char* fill = "";
		if (strlen(info) > 0)
		{
			std::cout << info << ":\n";
			fill = "\t";
		}

		while (err != GL_NO_ERROR)
		{
			std::cout << fill;
			switch (err)
			{
			case GL_INVALID_ENUM: std::cout << fill << "GL_INVALID_ENUM\n";
				break;
			case GL_INVALID_VALUE: std::cout << "GL_INVALID_VALUE\n";
				break;
			case GL_INVALID_OPERATION: std::cout << "GL_INVALID_OPERATION\n";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION\n";
				break;
			case GL_OUT_OF_MEMORY: std::cout << "GL_OUT_OF_MEMORY\n";
				break;
			case GL_STACK_UNDERFLOW: std::cout << "GL_STACK_UNDERFLOW\n";
				break;
			case GL_STACK_OVERFLOW: std::cout << "GL_STACK_OVERFLOW\n";
				break;
			}
			err = glGetError();
		}
	}
}

void Triangle::calcMedian()
{
	glm::vec3 tmpMedian(0, 0, 0);
	for (int i = 0; i < 3; ++i)
		tmpMedian += vertices[i];
	median = tmpMedian / 3.0f;
}

float Triangle::isHit(glm::vec3 rOrigin, glm::vec3 rDirection, float length)
{
	glm::vec3 e1, e2, h, s, q;
	float a, f, u, v, t;
	e1 = vertices[1] - vertices[0];
	e2 = vertices[2] - vertices[0];

	h = glm::cross(rDirection, e2);
	a = glm::dot(e1, h);

	if (a > -0.00001 && a < 0.00001)
		return(false);

	f = 1 / a;
	s =rOrigin - vertices[0];
	u = f * (glm::dot(s, h));

	if (u < 0.0 || u > 1.0)
		return(false);

	q = glm::cross(s, e1);
	v = f * glm::dot(rDirection, q);

	if (v < 0.0 || u + v > 1.0)
		return(false);

	// at this stage we can compute t to find out where
	// the intersection point is on the line
	t = f * glm::dot(e2, q);

	if (t > 0.f && t < length)
	{
		return t;
	}
	return -1.f;
}

void kdNode::calcBoundingBox(std::vector<int>& triangleIDs, std::vector<Triangle>& triangles)
{
	for (int id : triangleIDs)
	{
		for (int i = 0; i < 3; ++i)
		{
			glm::vec3 vertex = triangles[id].vertices[i];
			for (int axis = 0; axis < 3; ++axis)
			{
				bbox.min[axis] = std::min(bbox.min[axis], vertex[axis]);
				bbox.max[axis] = std::max(bbox.max[axis], vertex[axis]);
			}
		}
	}
}

bool kdNode::isHit(glm::vec3 rOrigin, glm::vec3 rDirection, float length)
{
	float tMin = (bbox.min.x - rOrigin.x) / rDirection.x;
	float tMax = (bbox.max.x - rOrigin.x) / rDirection.x;

	if (tMin > tMax)
		std::swap(tMin, tMax);

	float tyMin = (bbox.min.y - rOrigin.y) / rDirection.y;
	float tyMax = (bbox.max.y - rOrigin.y) / rDirection.y;

	if (tyMin > tyMax)
		std::swap(tyMin, tyMax);

	if ((tMin > tyMax) || (tyMin > tMax))
		return false;

	if (tyMin > tMin)
		tMin = tyMin;

	if (tyMax < tMax)
		tMax = tyMax;

	float tzMin = (bbox.min.z - rOrigin.z) / rDirection.z;
	float tzMax = (bbox.max.z - rOrigin.z) / rDirection.z;

	if (tzMin > tzMax)
		std::swap(tzMin, tzMax);

	if ((tMin > tzMax) || (tzMin > tMax))
		return false;

	if (tzMin > tMin)
		tMin = tzMin;

	if (tzMax < tMax)
		tMax = tzMax;

	return ((tMin < length) && (tMax > 0));
}

void sceneobj::calcOrigin()
{
	glm::vec3 o;
	int vertexCnt = 0;
	for (int i = 0; i < meshPtr->data.size(); i += 11)
	{
		vertexCnt++;
		o[0] += meshPtr->data[i + 0];
		o[1] += meshPtr->data[i + 1];
		o[2] += meshPtr->data[i + 2];
	}

	o.x /= vertexCnt;
	o.y /= vertexCnt;
	o.z /= vertexCnt;
	origin = o;
}
