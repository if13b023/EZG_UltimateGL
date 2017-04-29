#version 330 core

out vec4 fragColor;

in vertex_data{
	mat3 TBN;
	vec3 FragPos;
	vec2 uvCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

uniform vec3 objColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform bool shadowSwitch;
uniform bool simple;
uniform float normalFactor;
uniform sampler2D mainTexture;
uniform sampler2D normalMap;

vec2 parallaxMapping(vec2 uvCoords, vec3 viewDir)
{
	const float minLayers = 10;
	const float maxLayers = 20;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0f), viewDir)));
	float layerDepth = 1.0 / numLayers;
	float depthCurrent = 0.0;
	vec2 shift = viewDir.xy / viewDir.z; //* heightScale <- customization
	vec2 uvCoordsDiff = shift / numLayers;
	
	vec2 uvCoordsCurrent = uvCoords;
	float depthMapCurrent = texture(normalMap, uvCoordsCurrent).r;
	
	while(depthCurrent < depthMapCurrent)
	{
		uvCoordsCurrent -= uvCoordsDiff;
		depthMapCurrent = texture(normalMap, uvCoordsCurrent).r;
		depthCurrent += layerDepth;
	}
	
	vec2 uvCoordsPrev = uvCoordsCurrent + uvCoordsDiff;
	
	float depthAfter = depthMapCurrent - depthCurrent;
	float depthBefore = texture(normalMap, uvCoordsPrev).r - depthCurrent + layerDepth;
	
	float weight = depthAfter / (depthAfter - depthBefore);
	
	vec2 newUvCoords = uvCoordsPrev * weight + uvCoordsCurrent * (1.0 - weight);
	
	return newUvCoords;
}

void main()
{
	if(simple)
	{
		fragColor = vec4(objColor, 1.0);
		return;
	}
	
	//vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec2 uvCoords = parallaxMapping(fs_in.uvCoords, viewDir);
	
	// discards a fragment when sampling outside default texture region (fixes border artifacts)
    //if(uvCoords.x > 1.0 || uvCoords.y > 1.0 || uvCoords.x < 0.0 || uvCoords.y < 0.0)
    //    discard;
	
	vec3 norm = normalize(fs_in.TBN[2]);
	
	vec3 color = texture(mainTexture,  uvCoords).rgb;
	if(uvCoords.x > 1.0 || uvCoords.y > 1.0 || uvCoords.x < 0.0 || uvCoords.y < 0.0)
		color = vec3(1.0, 0.0, 0.0);
	
	// Ambient
	float ambientStrength = 0.1;
	vec3 ambient = color * ambientStrength * lightColor;
	
	// Diffuse 
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = color * diff * lightColor;
	
	// Specular
	float specularStrength = 0.4;
	vec3 reflectDir = reflect(-lightDir, norm);  
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;  
	
	//final
	vec3 lighting = (ambient + diffuse + specular);
	fragColor = vec4(lighting, 1.0);
}
