#version 330 core

out vec4 fragColor;

in vertex_data{
	vec3 fragPos;
	vec2 uvCoords;
	vec3 normal;
	vec3 tangentLightPos;
	vec3 tangentViewPos;
	vec3 tangentFragPos;
	vec3 tangentNormal;
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
uniform sampler2D displaceMap;
uniform ivec2 displacementSteps;

vec2 parallaxMapping(vec2 uvCoords, vec3 viewDir, float scale, float normalSign, int mode)
{
	int initSteps = displacementSteps.x;
	int refineSteps = displacementSteps.y;
	
	float layerDepth = 1.0 / initSteps;
	float depthCurrent = 0.0;
	vec2 shift;
	
	if(mode == 0 && normalSign < 0)
		shift = vec2(-viewDir.x, viewDir.y);
	else if((mode == 0 && normalSign > 0) || (mode == 1 && normalSign < 0) || (mode == 2 && normalSign > 0))
		shift = vec2(viewDir.x, -viewDir.y);
	else if((mode == 1 && normalSign > 0) || (mode == 2 && normalSign < 0))
		shift = vec2(viewDir.x, viewDir.y);
		
	shift = vec2(viewDir.x, -viewDir.y);
		
	shift *= normalFactor * normalSign;
	vec2 uvCoordsDiff = shift / initSteps;
	
	vec2 uvCoordsCurrent = uvCoords;
	float depthMapValue = texture(displaceMap, uvCoordsCurrent * scale).r;
	
	//init search
	for(int i = 0; i < initSteps; ++i)
	{
		if(depthMapValue <= depthCurrent)
			break;
	
		uvCoordsCurrent -= uvCoordsDiff;
		depthMapValue = texture(displaceMap, uvCoordsCurrent * scale).r;
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
		depthMapValue = texture(displaceMap, uvCoordsCurrent * scale).r;
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
	
	vec3 viewDir = normalize(fs_in.tangentViewPos - fs_in.tangentFragPos);
	vec3 norm = normalize(fs_in.tangentNormal);
	
	//tri-planar
	vec3 blend = abs(fs_in.normal);
	blend = normalize(max(blend, 0.00001));
	float b = (blend.x + blend.y + blend.z);
	blend /= vec3(b, b, b);
	
	vec3 xColorCode = vec3(1.0, 0, 0);
	vec3 yColorCode = vec3(0, 1.0, 0);
	vec3 zColorCode = vec3(0, 0, 1.0);
	
	float scale = 1.0;
	float colorFactor = 1.0;
	
	vec2 xCoords = parallaxMapping(fs_in.fragPos.zy, viewDir, scale, sign(fs_in.normal.x), 0);
	vec3 xColor = texture(mainTexture, xCoords * scale).rgb * max(xColorCode, colorFactor);
	vec3 xNormal = normalize(texture(normalMap, xCoords * scale).rgb * 2.0 - 1.0);
	
	vec2 yCoords = parallaxMapping(fs_in.fragPos.xz, viewDir, scale, sign(fs_in.normal.y), 1);
	vec3 yColor = texture(mainTexture, yCoords * scale).rgb * max(yColorCode, colorFactor);
	vec3 yNormal = normalize(texture(normalMap, yCoords * scale).rgb * 2.0 - 1.0);
	
	vec2 zCoords = parallaxMapping(fs_in.fragPos.xy, viewDir, scale, sign(fs_in.normal.z), 2);
	vec3 zColor = texture(mainTexture, zCoords * scale).rgb * max(zColorCode, colorFactor);
	vec3 zNormal = normalize(texture(normalMap, zCoords * scale).rgb * 2.0 - 1.0);
	
	vec3 color = xColor * blend.x * 1 + yColor * blend.y * 1 + zColor * blend.z * 1;
	vec3 normal = xNormal * blend.x * 1 + yNormal * blend.y * 1 + zNormal * blend.z * 1;
	
	// Ambient
	float ambientStrength = 0.2;
	vec3 ambient = color * ambientStrength * lightColor;
	
	// Diffuse 
	vec3 lightDir = normalize(fs_in.tangentLightPos - fs_in.tangentFragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = color * diff * lightColor;
	
	// Specular
	float specularStrength = 0.2;
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	vec3 specular = specularStrength * spec * lightColor;  
	
	//final
	vec3 lighting = (ambient + diffuse + specular);
	fragColor = vec4(lighting, 1.0);
}
