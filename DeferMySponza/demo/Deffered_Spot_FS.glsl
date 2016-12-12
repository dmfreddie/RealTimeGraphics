#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;
layout (location = 3) uniform sampler2DArray textureArray;
uniform sampler2D shadowMap;

uniform int currentSpotLight;

struct DirectionalLight
{
	vec3 direction;
	float padding1;
	vec3 intensity;
	float padding2;
};

struct PointLight
{
	vec3 position;
	float range;
	vec3 intensity;
	float padding;
};

struct AmbientLightBlock
{
	vec3 ambient_light;
	float padding;
};


struct SpotLight
{
	vec3 position;
	float range;
	vec3 direction;
	float coneAngle;
	vec3 intensity;
	bool castShadow;
};

layout(std140) uniform DataBlock
{
	PointLight pointLight[20];
	AmbientLightBlock ambientLight;
	DirectionalLight directionalLight[2];
	SpotLight spotLight[5];
	vec3 cameraPosition;
	float maxPointLights;	
	float maxDirectionalLights;
	float maxSpotlights;
};

struct Material
{
	vec3 diffuseColour;
	float vertexShineyness;
	vec3 specularColour;
	int diffuseTextureID;
};

layout(std140) uniform MaterialDataBlock
{
	Material materials[30];
};

out vec3 reflected_light;

vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);
vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation);
vec3 SpotLightCalc(vec3 colour);
float ShadowCalculation(vec4 fragPosLightSpace);

vec3 vertexPos = vec3(0,0,0);
vec3 vertexNormal = vec3(0,0,0);
int index = 0;
uniform bool useTextures;
uniform mat4 lightSpaceMatrix;
vec4 fraglightspacePos;

void main(void)
{
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	index = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).b);
	vertexNormal = normalize(texel_N);
	
	fraglightspacePos = lightSpaceMatrix * vec4(vertexPos, 1.0f);

	vec3 final_colour = SpotLightCalc(vec3(0,0,0));

	vec2 uv = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rg;

	if(useTextures && index < 27)
		final_colour *= texture(textureArray, vec3(uv, materials[index].diffuseTextureID)).xyz;

	reflected_light = final_colour;
}


float ShadowCalculation(vec4 fragPosLightSpace)
{
	// perform perspective divide
	vec3 projCoords = fraglightspacePos.xyz / fraglightspacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	// Get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// Check whether current frag pos is in shadow
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
	
	return shadow;
}


/*
Calculate the diffuse light for the point light and apply the diffuse texture.
Also call the specular for that light and add it to the diffuse value
@param currentLight - the light which the diffuse calculations need to be applied on
@param attenuation - the distance the light has an effect on
@return diffuseColour - the end result of the individual lights lighting calculation
*/
vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation)
{
	
	vec3 texel_M = materials[index].diffuseColour;
	vec3 L = normalize(lightPosition - vertexPos);
	float scaler = max(0, dot(L, normalize(vertexNormal))) * attenuation;

	if (scaler == 0)
		return vec3(0, 0, 0);

	vec3 diffuse_intensity = lightIntensity * scaler * texel_M;
	
	float shadow = ShadowCalculation(fraglightspacePos);

	if (materials[index].vertexShineyness > 0)
		return  diffuse_intensity + (1.0 - shadow) + SpecularLight(L, diffuse_intensity);
	else
		return  diffuse_intensity;

}

vec3 SpotLightCalc(vec3 colour)
{
	
	SpotLight spot = spotLight[currentSpotLight];
		

	// Compute smoothed dual-cone effect.
	float cosDir = dot(normalize(spot.position - vertexPos), -spot.direction);
	float spotEffect = smoothstep(cos(spot.coneAngle), cos(spot.coneAngle / 2), cosDir);

	float dist = distance(spot.position, vertexPos);
	float attenuation = 1 - smoothstep(0.0, spot.range, dist);
	// Compute height attenuation based on distance from earlier.
	//float attenuation = smoothstep(spot.range, 0.0f, length(spot.position - vertexPos));
	
	

	vec3 diffuse_intensity = DiffuseLight(spot.position, spot.intensity, attenuation);
				

	colour += (diffuse_intensity * spotEffect);

	

	return colour;
}

/*
Calculate the diffuse light for the point light and apply the diffuse texture.
Also call the specular for that light and add it to the diffuse value
@param lVector - the direction of the light for angular calculations
@param attenuation - the diffuse colour that needs to be used in the specular colour
@return specular_intensity - the end result of the specular calculations from the individual lights
*/
vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity)
{
	vec3 lightReflection = normalize(reflect(-LVector, normalize(vertexNormal)));
	vec3 vertexToEye = normalize(cameraPosition - vertexPos);
	float specularFactor = max(0.0, dot(vertexToEye, lightReflection));

	if (specularFactor > 0)
	{
		vec3 specularIntensity = diffuse_intensity * pow(specularFactor, materials[index].vertexShineyness);
		//if(useTextures)
		//	specularIntensity *= texture2DArray(specularTextureArray, vec3(text_coord, vert_diffuse_texture_ID)).rgb;
		return materials[index].specularColour * specularIntensity;
	}
	return vec3(0, 0, 0);
}