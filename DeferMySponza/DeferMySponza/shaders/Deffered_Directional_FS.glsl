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

layout(std140) uniform DataBlock{
	DirectionalLight directionalLights[3];
	vec3 camera_position;
	float maxDirectionalLights;
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
	vec3 texel_M = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 vertexNormal = normalize(texel_N);

	for (int i = 0; i < maxDirectionalLights; i++)
	{
		DirectionalLight dir = directionalLights[i];

		float scaler = max(0.0, dot(normalize(vertexNormal), dir.direction));

		vec3 diffuseIntensity = (dir.intensity * scaler) * texel_M;
		
		colour += diffuseIntensity;
	}

	return colour;
}
