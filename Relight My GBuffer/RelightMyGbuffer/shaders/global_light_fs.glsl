#version 330

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;

uniform vec3 light_direction[2];
uniform float light_intensity = 0.15;

uniform vec3 ambient_light;

out vec3 reflected_light;

vec3 DirLightCalc(vec3 normal, vec3 direction);

void main(void)
{
	vec3 texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 N = normalize(texel_N);
	vec3 colour = ambient_light;
	for (int i = 0; i < 2; ++i)
		colour += DirLightCalc(N, light_direction[i]);

	reflected_light = colour * 4; // 0.5 + 0.5 * N;
}

vec3 DirLightCalc(vec3 normal, vec3 direction)
{
	float scaler = max(0.0, dot(normalize(normal), direction));

	vec3 diffuseIntensity = vec3(light_intensity, light_intensity, light_intensity) * scaler;

	return diffuseIntensity;
}