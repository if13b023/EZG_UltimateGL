#include "FishGL.h"

FishGL::FishGL()
	:m_freemode(true),
	m_animation(nullptr),
	anim_count(0),
	m_drawAnimation(true),
	m_animationPoints(nullptr),
	m_AnimResolution(50)
{
	glfwInit();
	m_shaders.reserve(16);
	m_vao.reserve(16);
	m_vbo.reserve(16);
	camera.rotation = glm::quat(glm::vec3(0, glm::radians(0.0f), 0));
}


FishGL::~FishGL()
{
	delete[] m_animationPoints;
}

GLFWwindow * FishGL::createWindow(int width, int height)
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	m_window = glfwCreateWindow(width, height, "LearnOpenGL", nullptr, nullptr);
	if (m_window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(m_window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		std::cin.ignore();
		exit(1);
	}

	int tmp_width, tmp_height;
	glfwGetFramebufferSize(m_window, &tmp_width, &tmp_height);

	glViewport(0, 0, tmp_width, tmp_height);

	glEnable(GL_DEPTH_TEST);

	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return m_window;
}

void FishGL::Run()
{
	double lastframe = glfwGetTime();
	dt = 0.014;

	while (!glfwWindowShouldClose(m_window))
	{
		GLint viewId;
		glm::mat4 m_view;
		//glUniformMatrix4fv(viewId, 1, GL_FALSE, glm::value_ptr(m_view));

		dt = glfwGetTime() - lastframe;
		lastframe = glfwGetTime();

		if (m_freemode && m_animation != nullptr)
		{
			float pi = 3.1415f;
			float moveSpeed = 0.1f;
			glm::vec3 forwardVec(0, 0, moveSpeed),
				sideVec(moveSpeed, 0, 0);

			if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			{
				camera.position += forwardVec * camera.rotation;
			}
			else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			{
				camera.position -= forwardVec * camera.rotation;
			}

			if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
				camera.position += sideVec * camera.rotation;
			else if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
				camera.position -= sideVec * camera.rotation;

			glm::mat4 translate = glm::mat4(1.0f);
			//translate = glm::translate(m_view, camera.position);
			//translate = glm::translate(m_view, -glm::vec3(0,0,-1.0f));
			m_view = glm::translate(m_view, camera.position);
			m_view = glm::mat4_cast(camera.rotation) * m_view;
		}
		else {
			glm::vec3 pos;
			glm::quat rot;

			runAnimation(pos, rot);
			camera.position = pos;
			camera.rotation = rot;
			//m_view = glm::translate(m_view, camera.position);
			//m_view = glm::mat4_cast(camera.rotation) * m_view;
			m_view = glm::toMat4(camera.rotation) * glm::translate(m_view, camera.position);
		}
		//keyboard events
		glfwPollEvents();

		//render stuff
		glEnable(GL_DEPTH_TEST);
		//glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//moving
		//main_shader->Use();
		//glBindVertexArray(VAO);

		for (GLuint i = 1; i < m_scene.size(); i++)
		{
			float iFl = i;
			m_scene[i].shader->Use();
			viewId = glGetUniformLocation(m_scene[i].shader->Program, "view");
			//glBindVertexArray(m_scene[i].VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_scene[i].EBO);

			glm::mat4 model;
			model = glm::translate(model, m_scene[i].position);
			model = glm::scale(model, glm::vec3(m_scene[i].scale, m_scene[i].scale, m_scene[i].scale));
			//GLfloat angle = 20.0f * i + static_cast<float>(glfwGetTime());
			//model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));

			glUniformMatrix4fv(m_scene[i].shader->transId, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(viewId, 1, GL_FALSE, glm::value_ptr(m_view));
			//glUniform4f(m_scene[i].shader->fragmentColorId, (m_scene[i].position.r + 5) / 10, (m_scene[i].position.g + 5) / 10, (m_scene[i].position.b + 5) / 10, 1.0f);
			glUniform4f(m_scene[i].shader->fragmentColorId, iFl / m_scene.size(), iFl / m_scene.size(), iFl / m_scene.size(), 1.0f);
			
			//glDrawArrays(GL_TRIANGLES, 0, 128);
			glDrawElements(GL_TRIANGLES, m_scene[i].iCount, GL_UNSIGNED_INT, 0);

			glBindVertexArray(0);
		}

		if(m_drawAnimation && false)
			drawAnimation(m_view);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		//swap buffers
		glfwSwapBuffers(m_window);
	}
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
			case GLFW_KEY_S:
				//m_view = glm::translate(m_view, glm::vec3(0,0,-1.0f));
				break;
			case GLFW_KEY_W:
				//m_view = glm::translate(m_view, glm::vec3(0, 0, 1.0f));
				break;
			case GLFW_KEY_T:
				m_freemode = !m_freemode;
				m_animation_start = glfwGetTime();
				std::cout << m_freemode << std::endl;
				break;
			case GLFW_KEY_R:
				std::cout << "frame[" << anim_count << "].position = glm::vec3(" << camera.position.x << "f, " << camera.position.y << "f, " << camera.position.z << "f);" << std::endl;
				std::cout << "frame[" << anim_count << "].rotation = glm::quat(" << camera.rotation.w << "f, " << camera.rotation.x << "f, " << camera.rotation.y << "f, " << camera.rotation.z << "f);" << std::endl;
				m_animation->frames[anim_count].position = camera.position;
				m_animation->frames[anim_count].rotation = glm::normalize(camera.rotation);
				anim_count++;
				if (anim_count == 10)
					anim_count = 0;

				m_animationPoints = getAnimation(m_AnimResolution);

				break;
			case GLFW_KEY_I:
				std::cout << camera.position.x << "|" << camera.position.y << "|" << camera.position.z << "|" << std::endl;
				std::cout << camera.rotation.x << "|" << camera.rotation.y << "|" << camera.rotation.z << "|" << camera.rotation.w << "|" << std::endl;
				break;
			case GLFW_KEY_N:
				m_drawAnimation = !m_drawAnimation;
				break;
		}
	}
}

void FishGL::mouse_callback(double xpos, double ypos)
{
	const GLfloat sens = 0.3f;
	//GLfloat diff_xpos = xpos - mouse.x;
	//GLfloat diff_ypos = mouse.y - ypos;
	glm::vec2 diff = glm::vec2(xpos, ypos) - mouse;
	//camera.rotation = glm::normalize(glm::quat(glm::vec3(glm::radians(diff.y * dt * sens), glm::radians(diff.x * dt * sens), 0.0f)) * camera.rotation);
	//camera.rotation = glm::normalize(glm::quat(glm::vec3(diff.y * dt * sens, diff.x * dt * sens, 0.0f)) * camera.rotation);
	glm::quat yaw = glm::quat(glm::vec3(diff.y * dt * sens, 0.0f, 0.0f));
	glm::quat pitch = glm::quat(glm::vec3(0.0f, diff.x * dt * sens, 0.0f));
	camera.rotation = glm::normalize(yaw * camera.rotation * pitch);
	//std::cout << camera.rotation.z << std::endl;
	mouse.x = xpos;
	mouse.y = ypos;
}

void FishGL::addObject(GLfloat* vertices, int vSize, GLuint* indices, int iSize, GLuint& vbo, GLuint& vao, GLuint& ebo)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vSize * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(GLfloat), (GLvoid*)0);
	// Color attribute
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	//glEnableVertexAttribArray(1);

	//EBO
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, iSize * sizeof(GLuint), indices, GL_STATIC_DRAW);

	//CLEANUP
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	m_vao.push_back(vao);
	m_vbo.push_back(vbo);
}

Shader * FishGL::addShader(const char * vertex, const char * fragment)
{
	Shader tmp(vertex, fragment);
	tmp.Use();

	glUniformMatrix4fv(tmp.projId, 1, GL_FALSE, glm::value_ptr(m_projection));

	m_shaders.push_back(tmp);
	return &m_shaders[m_shaders.size() - 1];
}

void FishGL::setPerspective(float fovy, float aspect, float near, float far)
{
	m_projection = glm::perspective(fovy, aspect, near, far);
}

glm::mat4 FishGL::getPerspective()
{
	return m_projection;
}

void FishGL::addObjectToScene(sceneobj & obj)
{
	m_scene.push_back(obj);
}

void FishGL::addObjectsToScene(sceneobj* obj, int size)
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
	float anim_time = glfwGetTime() - m_animation_start;
	float percent = (anim_time / m_animation->duration)*m_animation->count;
	int index = floor(percent);

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
	//rot = glm::normalize(glm::lerp(rots[1], rots[2], glm::fract(percent)));
	//***
}

void FishGL::drawAnimation(glm::mat4& view)
{
	m_scene[0].shader->Use();
	GLuint viewId = glGetUniformLocation(m_scene[0].shader->Program, "view");
	//glBindVertexArray(m_scene[0].VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_scene[0].EBO);

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

		glDrawElements(GL_TRIANGLES, m_scene[0].iCount, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
}

glm::vec3* FishGL::getAnimation(int resolution)
{
	glm::vec3* points = new glm::vec3[resolution];

	for (int i = 0; i < resolution; ++i)
	{
		float percent = ((float)i / resolution)*m_animation->count;
		int index = floor(percent);

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
