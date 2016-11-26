#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;

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

void main(void)
{
	vec3 final_colour = vec3(0, 0, 0);

	//int matID = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).a);

	final_colour = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;

	reflected_light = final_colour * ambientLight.ambient_light;
}
