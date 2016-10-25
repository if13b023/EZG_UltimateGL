#version 330 core
  
layout (location = 0) in vec3 position;

out vec4 vertexColor;

uniform mat4 transform;
uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * transform * vec4(position, 1.0f);
	//vertexColor = vec4(0.5f, 0.0f, 0.0f, 1.0f);
}
