#version 330 core
  
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 tangent;

out vertex_data{
	mat3 TBN;
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
	vs_out.uvCoords = uv;
	vs_out.fragPosLightSpace = lightMatrix * vec4(vs_out.FragPos, 1.0);
	mat3 normalMatrix = transpose(inverse(mat3(transform)));
	vec3 T = normalize(vec3(normalMatrix * tangent));
	vec3 N = normalize(vec3(normalMatrix * normal));
	vec3 B = cross(T, N);
	vs_out.TBN = mat3(T, B, N);
}

























