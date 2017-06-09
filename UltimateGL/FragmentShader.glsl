#version 330 core

#define BLOCKER_SEARCH_NUM_SAMPLES 16
#define PCF_NUM_SAMPLES 16
#define NEAR_PLANE 1.0
#define LIGHT_WORLD_SIZE .5
#define LIGHT_FRUSTUM_WIDTH 3.75

#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)

const vec2 poissonDisk[16] = vec2[](
	vec2( -0.94201624, -0.39906216 ),
	vec2( 0.94558609, -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2( 0.34495938, 0.29387760 ),
	vec2( -0.91588581, 0.45771432 ),
	vec2( -0.81544232, -0.87912464 ),
	vec2( -0.38277543, 0.27676845 ),
	vec2( 0.97484398, 0.75648379 ),
	vec2( 0.44323325, -0.97511554 ),
	vec2( 0.53742981, -0.47373420 ),
	vec2( -0.26496911, -0.41893023 ),
	vec2( 0.79197514, 0.19090188 ),
	vec2( -0.24188840, 0.99706507 ),
	vec2( -0.81409955, 0.91437590 ),
	vec2( 0.19984126, 0.78641367 ),
	vec2( 0.14383161, -0.14100790 )
);

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
uniform bool simple;
uniform float normalFactor;
uniform sampler2D mainTexture;
uniform sampler2D normalMap;
uniform sampler2D depthMap;

float PenumbraSize(float zReceiver, float zBlocker) //Parallel plane estimation
{
	return (zReceiver - zBlocker) / zBlocker;
}

void FindBlocker(out float avgBlockerDepth, out float numBlockers, vec2 uv, float zReceiver )
{
	//This uses similar triangles to compute what
	//area of the shadow map we should search
	float searchWidth = LIGHT_SIZE_UV * (zReceiver - NEAR_PLANE) / zReceiver;
	float blockerSum = 0;
	numBlockers = 0;
	for( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i )
	{
		//float shadowMapDepth = tDepthMap.SampleLevel(PointSampler, uv + poissonDisk[i] * searchWidth, 0); //DX
		float shadowMapDepth = texture(depthMap, uv + poissonDisk[i] * searchWidth).r;
		if ( shadowMapDepth < zReceiver )
		{
			blockerSum += shadowMapDepth;
			numBlockers++;
		}
	}
	avgBlockerDepth = blockerSum / numBlockers;
}

float PCF_Filter(vec2 uv, float zReceiver, float filterRadiusUV )
{
	// float sum = 0.0f;
	// for ( int i = 0; i < PCF_NUM_SAMPLES; ++i )
	// {
		// vec2 offset = poissonDisk[i] * filterRadiusUV;
		// sum += tDepthMap.SampleCmpLevelZero(PCF_Sampler, uv + offset, zReceiver);
	// }
	// return sum / PCF_NUM_SAMPLES;
	
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(depthMap, 0);
	float bias = 0.005;
	
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(depthMap, uv + vec2(x, y) * texelSize).r; 
			shadow += zReceiver - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	
	return shadow /= 9.0;
}

float PCSS (vec3 coords)
{
	
	vec2 uv = coords.xy;
	float zReceiver = coords.z; // Assumed to be eye-space z in this code
	
	/* STEP 1: blocker search */
	float avgBlockerDepth = 0;
	float numBlockers = 0;
	FindBlocker( avgBlockerDepth, numBlockers, uv, zReceiver );
	
	if( numBlockers < 1 )
	/* There are no occluders so early out (this saves filtering)*/
		return 0.0;
	
	/* STEP 2: penumbra size */
	float penumbraRatio = PenumbraSize(zReceiver, avgBlockerDepth);
	float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / coords.z;
	
	/* STEP 3: filtering */
	return PCF_Filter( uv, zReceiver, filterRadiusUV );
}

float shadowCalc(vec4 fragPosLightSpace, vec3 norm)
{
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	if(projCoords.z > 1.0)
		return 0.0;

	bool pcss = true;
	if(pcss)
		return PCSS(projCoords);
	else
	{	
		// Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
		float closestDepth = texture(depthMap, projCoords.xy).r; 
		// Get depth of current fragment from light's perspective
		float currentDepth = projCoords.z;
		// Check whether current frag pos is in shadow
		float bias = max(0.005 * (1.0 - dot(norm, normalize(lightPos - fs_in.FragPos))), 0.005);
		bias = 0.005;
		
		float shadow = 0.0;
		vec2 texelSize = 1.0 / textureSize(depthMap, 0);
		
		// for(int x = -1; x <= 1; ++x)
		// {
			// for(int y = -1; y <= 1; ++y)
			// {
				// float pcfDepth = texture(depthMap, projCoords.xy + vec2(x, y) * texelSize).r; 
				// shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
			// }    
		// }
		
		// shadow /= 9.0;	
		
		shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
		
		return shadow;
	}
}

void main()
{
	if(simple == false)
	{
		vec3 color = texture(mainTexture,  fs_in.uvCoords).rgb;
		vec3 normMap = normalize(texture(normalMap, fs_in.uvCoords).rgb * 2.0 - 1.0);
		vec3 norm = normalize(fs_in.TBN * normMap);
		norm = normalize(mix(fs_in.TBN[2], norm, normalFactor));
		//norm = normalize(fs_in.TBN * normFlat);
		//color = (norm + 1.0) / 2.0;
		
		// Ambient
		float ambientStrength = 0.1;
		vec3 ambient = color * ambientStrength * lightColor;
		
		// Diffuse 
		vec3 lightDir = /*fs_in.TBN * */normalize(lightPos - fs_in.FragPos);
		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = color * diff * lightColor;
		
		// Specular
		float specularStrength = 0.1;
		vec3 viewDir = /*fs_in.TBN * */normalize(viewPos - fs_in.FragPos);
		vec3 reflectDir = reflect(-lightDir, norm);  
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2);
		vec3 specular = specularStrength * spec * lightColor;  
		
		//Shadow
		float shadow = 0.0;
		if(shadowSwitch)
			shadow = shadowCalc(fs_in.fragPosLightSpace, norm);
		//shadow = 0.0;
		vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));
		fragColor = vec4(lighting, 1.0);
	}else
		fragColor = vec4(objColor, 1.0);
}
