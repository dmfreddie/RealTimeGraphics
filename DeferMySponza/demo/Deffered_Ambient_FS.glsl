#version 330

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;


out vec3 reflected_light;

void main(void)
{
	//vec3 texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 N = normalize(texel_N);
	reflected_light = 0.5 + 0.5 * N ;//(projection_view * vec4(texel_P, 1.0)).xyz; //0.5 + 0.5 * N;
}
