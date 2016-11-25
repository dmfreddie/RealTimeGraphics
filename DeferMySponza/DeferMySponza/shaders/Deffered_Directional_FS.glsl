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

layout (location = 0) out vec3 reflected_light;

vec3 DirLightCalc(vec3 colour);

void main(void)
{
	
	vec3 final_colour = DirLightCalc(vec3(0, 0, 0));
	reflected_light = final_colour;
}


vec3 DirLightCalc(vec3 colour)
{
	int matID = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).r);
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 vertexNormal = normalize(texel_N);

	for (int i = 0; i < maxDirectionalLights; i++)
	{
		DirectionalLight dir = directionalLight[i];

		float scaler = max(0.0, dot(normalize(vertexNormal), dir.direction));

		vec3 diffuseIntensity = (dir.intensity * scaler);
		
		colour += diffuseIntensity * materials[matID].diffuseColour;
	}

	return colour;
}
