#version 330 core
out vec4 fragColor;

in float depth;

void main()
{
	fragColor = vec4(vec3(depth),1.0);
}
