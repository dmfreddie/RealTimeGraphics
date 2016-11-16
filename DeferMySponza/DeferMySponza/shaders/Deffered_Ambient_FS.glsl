#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;

layout (std140) uniform DataBlock {
	vec3 camera_position;
	vec3 global_ambient_light;
};


layout (location = 0) out vec3 reflected_light;

void main(void)
{
	vec3 final_colour = global_ambient_light;

	//vec3 texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	//vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	//vec3 texel_M = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;
	//vec3 N = normalize(texel_N);

	final_colour *= texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;

	reflected_light = vec3(final_colour);
}
