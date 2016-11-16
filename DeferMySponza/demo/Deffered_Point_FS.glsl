#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;

uniform vec3 point_light_position;
uniform float point_light_range;
uniform vec3 point_light_intensity = vec3(1.0, 0.0, 0.0);

out vec3 reflected_light;

vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation);
vec3 PointLightCalc();

vec3 texel_P = vec3(0,0,0);
vec3 N = vec3(0,0,0);

void main(void)
{
	texel_P = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	N = normalize(texel_N);

	vec3 colour = PointLightCalc();

	reflected_light = colour; // 0.5 + 0.5 * N;
}

/*
Calculate the diffuse light for the point light and apply the diffuse texture.
Also call the specular for that light and add it to the diffuse value
@param currentLight - the light which the diffuse calculations need to be applied on
@param attenuation - the distance the light has an effect on
@return diffuseColour - the end result of the individual lights lighting calculation
*/
vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation)
{
	vec3 L = normalize(lightPosition - texel_P);
	float scaler = max(0, dot(L, normalize(N))) * attenuation;

	if (scaler == 0)
		return vec3(0, 0, 0);

	vec3 diffuse_intensity = lightIntensity * scaler;


	return  diffuse_intensity;
}

/*
Calculate the colour value for the light and add it to the total light for the pixel
@param currentLight - the light which the diffuse calculations need to be applied on
@return colour - the final colour for theat fragment after all point lighting calculations
*/
vec3 PointLightCalc()
{
	
	float dist = distance(point_light_position, texel_P);
	float attenuation = 1 - smoothstep(0.0, point_light_range, dist);

	
	vec3 colour = DiffuseLight(point_light_position, point_light_intensity, attenuation);
	
	
	return colour;
}
