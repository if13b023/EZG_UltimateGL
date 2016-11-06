#version 330 core

out vec4 fragColor;

in VS_OUT{
	vec3 Normal;
	vec3 FragPos;
	vec2 uvCoords;
	vec4 fragPosLightSpace;
} fs_in;

uniform vec3 objColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform sampler2D mainTexture;
uniform sampler2D depthMap;

float shadowCalc(vec4 fragPosLightSpace)
{
	// perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(depthMap, projCoords.xy).r; 
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // Check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main()
{
	vec3 color = texture(mainTexture,  fs_in.uvCoords).rgb;
	vec3 norm = normalize(fs_in.Normal);
	
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = color * ambientStrength * lightColor;
  	
    // Diffuse 
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = color * diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
	
	//Shadow
	float shadow = shadowCalc(fs_in.fragPosLightSpace);
	//shadow = 0.0;
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));
    fragColor = vec4(lighting, 1.0) ;
}
