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

struct PBRMaterial
{
	vec3 diffuseColour;
	float metallic;
	vec3 specularColour;
	float roughness;
	float vertexShineyness;
	float ambientOcclusion;
	int diffuseTextureID;
};

layout(std140) uniform PBRMaterialDataBlock
{
	PBRMaterial pbrMaterials[30];
};

out vec3 reflected_light;
uniform bool useTextures;
//int index = 0;
void main(void)
{
	vec3 final_colour = vec3(0, 0, 0);

	////vec3 texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	////vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	//vec3 texel_M = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;
	int index = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).b);
	vec2 uv = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rg;
	vec3 texel_M = pbrMaterials[index].diffuseColour;
	//vec3 texel_M = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;
	//vec3 N = normalize(texel_N);

	//final_colour *= texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;

	if(useTextures && index < 27)
		final_colour = texture(textureArray, vec3(uv, pbrMaterials[index].diffuseTextureID)).xyz;
	else
		final_colour = texel_M;

	reflected_light = final_colour * ambientLight.ambient_light;
}
