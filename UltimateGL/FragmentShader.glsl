#version 330 core

out vec4 color;

uniform vec4 uniformColor;

void main()
{
    //color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
	color = uniformColor;
}
