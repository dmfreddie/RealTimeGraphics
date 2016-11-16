#version 440

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;
uniform sampler2DRect sampler_world_material;

out vec3 reflected_light;

void main(void)
{
	vec3 texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_M = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rgb;
	vec3 N = normalize(texel_N);
	reflected_light = texel_M;// 0.5 + 0.5 * N;
}
