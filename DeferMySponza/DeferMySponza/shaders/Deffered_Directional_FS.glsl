#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;
layout (location = 3) uniform sampler2DArray textureArray;

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


layout (location = 0) out vec3 reflected_light;
int index = 0;
vec3 vertexPos;
vec3 vertexNormal;
uniform bool useTextures;

vec3 DirLightCalc(vec3 colour);
vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);

//vec3 Frostbite_DisneyDiffuse(vec3 NdotV, float NdotL, float LdotH, float linearRoughness);
//vec3 F_Schlick(vec3 f0, vec3 f90, float u);
////https://gist.github.com/galek/53557375251e1a942dfa
void main(void)
{
	vec2 uv = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rg;
	vec3 final_colour = DirLightCalc(vec3(0, 0, 0));

	if(useTextures && index < 27)
		final_colour *= texture(textureArray, vec3(uv, materials[index].diffuseTextureID)).xyz;

	reflected_light = final_colour;
}


vec3 DirLightCalc(vec3 colour)
{
	index = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).b);
	vec3 texel_M = materials[index].diffuseColour;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vertexNormal = normalize(texel_N);
	

	for (int i = 0; i < maxDirectionalLights; i++)
	{
		DirectionalLight dir = directionalLight[i];

		float scaler = max(0.0, dot(normalize(vertexNormal), dir.direction));

		vec3 diffuseIntensity = (dir.intensity * scaler);

		if (materials[index].vertexShineyness > 0)
			colour += diffuseIntensity + SpecularLight(dir.direction, dir.intensity) * texel_M;
		else
			colour += diffuseIntensity * texel_M;
	}

	

	return colour;
}

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

//vec3 Frostbite_DisneyDiffuse(vec3 NdotV, float NdotL, float LdotH, float linearRoughness)
//{
//	float energyBias = lerp(0, 0.5f, linearRoughness);
//	float energyFactor = lerp(1.0f, 1.0f / 1.51f, linearRoughness);
//	vec3 fd90 = energyBias + 2.0 * LdotH*LdotH * linearRoughness;
//	vec3 f0 = vec3(1.0f, 1.0f, 1.0f);
//	float lightScatter = F_Schlick(0.0f, fd90, NdotL).r;
//	float viewScatter = F_Schlick(0.0f, fd90, NdotV).r;
//
//	return lightScatter * viewScatter * energyFactor;
//}
//
//vec3 F_Schlick(vec3 f0, vec3 f90, float u)
//{
//	return f0 + (f90 - f0) * pow(1.0f - u, 5.0f);
//}