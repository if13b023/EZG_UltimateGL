#version 330 core
  
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

out VS_OUT{
	vec3 Normal;
	vec3 FragPos;
	vec2 uvCoords;
	vec4 fragPosLightSpace;
} vs_out;

uniform mat4 transform;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 lightMatrix;

void main()
{
    gl_Position = projection * view * transform * vec4(position, 1.0);
	vs_out.FragPos = vec3(transform * vec4(position, 1.0));
	vs_out.Normal = mat3(transpose(inverse(transform))) * normal;
	vs_out.uvCoords = uv;
	vs_out.fragPosLightSpace = lightMatrix * vec4(vs_out.FragPos, 1.0);
}
