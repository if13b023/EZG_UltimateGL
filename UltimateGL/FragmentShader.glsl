#version 330 core

out vec4 fragColor;

in vertex_data{
	mat3 TBN;
	vec3 FragPos;
	vec2 uvCoords;
	vec4 fragPosLightSpace;
} fs_in;

uniform vec3 objColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform bool shadowSwitch;
uniform bool normalSwitch;
uniform sampler2D mainTexture;
uniform sampler2D normalMap;
uniform sampler2D depthMap;

float shadowCalc(vec4 fragPosLightSpace, vec3 norm)
{
	// perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
	if(projCoords.z > 1.0)
		return 0.0;
	
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(depthMap, projCoords.xy).r; 
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // Check whether current frag pos is in shadow
	float bias = max(0.005 * (1.0 - dot(norm, normalize(lightPos - fs_in.FragPos))), 0.005);  

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(depthMap, 0);
	
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(depthMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	
	shadow /= 9.0;	
	
    return shadow;
}

void main()
{
	vec3 color = texture(mainTexture,  fs_in.uvCoords).rgb;
	vec3 norm  = vec3(0.0,0.0,1.0);
	if(normalSwitch)
		norm = normalize(texture(normalMap, fs_in.uvCoords).rgb * 2.0 - 1.0);
	norm = normalize(fs_in.TBN * norm);
	//color = (norm + 1.0) * 2.0;
	
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = color * ambientStrength * lightColor;
  	
    // Diffuse 
    vec3 lightDir = /*fs_in.TBN * */normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = color * diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = /*fs_in.TBN * */normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
	
	//Shadow
	float shadow = 0.0;
	if(shadowSwitch)
		shadow = shadowCalc(fs_in.fragPosLightSpace, norm);
	//shadow = 0.0;
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));
    fragColor = vec4(lighting, 1.0);
}
