#version 330 core

out vec4 fragColor;

in vertex_data{
	vec3 FragPos;
	vec2 uvCoords;
	vec3 normal;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
	vec3 TangentNormal;
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
	const float initSteps = 10;
	const float refineSteps = 5;
	
	float layerDepth = 1.0 / initSteps;
	float depthCurrent = 0.0;
	vec2 shift = vec2(viewDir.x, viewDir.y) / viewDir.z * 0.1; //y flip -> other solution?
	vec2 uvCoordsDiff = shift / initSteps;
	
	vec2 uvCoordsCurrent = uvCoords;
	float depthMapValue = texture(normalMap, uvCoordsCurrent).r;
	
	//init search
	for(int i = 0; i < initSteps; ++i)
	{
		if(depthMapValue <= depthCurrent)
			break;
	
		uvCoordsCurrent -= uvCoordsDiff;
		depthMapValue = texture(normalMap, uvCoordsCurrent).r;
		depthCurrent += layerDepth;
	}
	
	//refinement
	depthCurrent -= layerDepth;
	uvCoordsCurrent += uvCoordsDiff;
	depthMapValue = 1.0; //no need to sample again, because it IS higher than depthCurrent
	layerDepth /= refineSteps;
	uvCoordsDiff /= refineSteps;
	for(int i = 0; i < refineSteps; ++i)
	{
		if(depthMapValue < depthCurrent)
			break;
	
		uvCoordsCurrent -= uvCoordsDiff;
		depthMapValue = texture(normalMap, uvCoordsCurrent).r;
		depthCurrent += layerDepth;
	}
	
	return uvCoordsCurrent;
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
	//vec2 uvCoords = parallaxMapping(fs_in.uvCoords, viewDir);
	vec2 uvCoords = fs_in.uvCoords;
	
	// discards a fragment when sampling outside default texture region (fixes border artifacts)
    //if(uvCoords.x > 1.0 || uvCoords.y > 1.0 || uvCoords.x < 0.0 || uvCoords.y < 0.0)
    //    discard;
	
	vec3 norm = normalize(fs_in.TangentNormal);
	
	vec3 color = texture(mainTexture,  uvCoords).rgb;
	//if(uvCoords.x > 1.0 || uvCoords.y > 1.0 || uvCoords.x < 0.0 || uvCoords.y < 0.0)
		//color = vec3(1.0, 0.0, 0.0);
		//discard;
	
	//tri-planar
	vec3 blend = abs(fs_in.normal);
	blend = normalize(max(blend, 0.00001));
	float b = (blend.x + blend.y + blend.z);
	blend /= vec3(b, b, b);
	/* vec3 xColor = vec3(1.0, 0.0, 0.0);
	vec3 yColor = vec3(0.0, 1.0, 0.0);
	vec3 zColor = vec3(0.0, 0.0, 1.0); */
	vec3 xColor = texture(mainTexture, fs_in.FragPos.zy).rgb;
	vec3 yColor = texture(mainTexture, fs_in.FragPos.xz).rgb;
	vec3 zColor = texture(mainTexture, fs_in.FragPos.xy).rgb;
	color = xColor * blend.x + yColor * blend.y + zColor * blend.z;
	
	// Ambient
	float ambientStrength = 0.1;
	vec3 ambient = color * ambientStrength * lightColor;
	
	// Diffuse 
	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	float diff = max(dot(lightDir, norm), 0.0);
	vec3 diffuse = color * diff * lightColor;
	
	// Specular
	float specularStrength = 0.4;
	vec3 reflectDir = reflect(-lightDir, norm);  
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;  
	
	//final
	vec3 lighting = (ambient + diffuse + specular);
	fragColor = vec4(lighting, 1.0);
}
