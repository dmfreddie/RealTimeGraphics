#version 330

uniform sampler2DRect sampler_world_position;
uniform sampler2DRect sampler_world_normal;

uniform vec3 light_direction;
uniform float light_intensity = 0.15;

out vec3 reflected_light;

vec3 DirLightCalc(vec3 normal);

void main(void)
{
	vec3 texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vec3 N = normalize(texel_N);

	vec3 colour = DirLightCalc(N);

	reflected_light = colour; // 0.5 + 0.5 * N;
}

vec3 DirLightCalc(vec3 normal)
{
	float scaler = max(0.0, dot(normalize(normal), light_direction));

	vec3 diffuseIntensity = vec3(light_intensity, light_intensity, light_intensity) * scaler;

	return diffuseIntensity;
}