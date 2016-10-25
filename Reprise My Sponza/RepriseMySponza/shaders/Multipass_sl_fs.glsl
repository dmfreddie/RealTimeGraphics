#version 440
#extension GL_EXT_texture_array : enable

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
	SpotLight spotLights[15];
	vec3 camera_position;
	float maxSpotLights;
};

layout (location=1) uniform sampler2DArray textureArray;
uniform bool useTextures;

in vec3 vertexPos;
in vec3 vertexNormal;
in vec2 text_coord;
in vec3 vert_diffuse_colour;
in vec3 vert_specular_colour;
in float vert_is_vertex_shiney;
in float vert_diffuse_texture_ID;

out vec4 fragment_colour;



vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);
vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation);
vec3 SpotLightCalc(vec3 colour);

void main(void)
{
	vec3 final_colour = SpotLightCalc(vec3(0,0,0));
	if(useTextures && vert_diffuse_texture_ID < 27)
	{
		#ifdef GL_EXT_texture_array
			final_colour *= texture2DArray(textureArray, vec3(text_coord, vert_diffuse_texture_ID)).rgb;
		#else
			final_colour *= texture(textureArray, vec3(text_coord, vert_diffuse_texture_ID)).xyz;
		#endif
	}
	else
	{
		final_colour *= vert_diffuse_colour;
	}
	fragment_colour = vec4(final_colour, 1.0);
}

/*
Calculate the diffuse light for the point light and apply the diffuse texture.
Also call the specular for that light and add it to the diffuse value
@param lVector - the direction of the light for angular calculations
@param attenuation - the diffuse colour that needs to be used in the specular colour
@return specular_intensity - the end result of the specular calculations from the individual lights
*/
vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity)
{
	vec3 lightReflection = normalize(reflect(-LVector, normalize(vertexNormal)));
	vec3 vertexToEye = normalize(camera_position - vertexPos);
	float specularFactor = max(0.0, dot(vertexToEye, lightReflection));

	if (specularFactor > 0)
	{
		vec3 specularIntensity = diffuse_intensity * pow(specularFactor, vert_is_vertex_shiney);
		//if(useTextures)
		//	specularIntensity *= texture2DArray(specularTextureArray, vec3(text_coord, vert_diffuse_texture_ID)).rgb;
		return vert_specular_colour * specularIntensity;
	}
	return vec3(0, 0, 0);
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
	vec3 L = normalize(lightPosition - vertexPos);
	float scaler = max(0, dot(L, normalize(vertexNormal))) * attenuation;

	if (scaler == 0)
		return vec3(0, 0, 0);

	vec3 diffuse_intensity = lightIntensity * scaler;


	if (vert_is_vertex_shiney > 0)
		return  diffuse_intensity + SpecularLight(L, diffuse_intensity);
	else
		return  diffuse_intensity;
}

vec3 SpotLightCalc(vec3 colour)
{
	for (int i = 0; i < maxSpotLights; i++)
	{
		SpotLight spot = spotLights[i];
		

		// Compute smoothed dual-cone effect.
		float cosDir = dot(normalize(spot.position - vertexPos), -spot.direction);
		float spotEffect = smoothstep(cos(spot.coneAngle), cos(spot.coneAngle / 2), cosDir);

		float dist = distance(spot.position, vertexPos);
		float attenuation = 1 - smoothstep(0.0, spot.range, dist);
		// Compute height attenuation based on distance from earlier.
		//float attenuation = smoothstep(spot.range, 0.0f, length(spot.position - vertexPos));

		vec3 diffuse_intensity = DiffuseLight(spot.position, spot.intensity, attenuation);
				

		colour += (diffuse_intensity * spotEffect);

	}

	return colour;
}