#version 440
#extension GL_EXT_texture_array : enable

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


in vec3 vertexPos;
in vec3 vertexNormal;
in vec2 text_coord;
in vec3 vert_diffuse_colour;
in vec3 vert_specular_colour;
in float vert_is_vertex_shiney;
in float vert_diffuse_texture_ID;

out vec4 fragment_colour;


vec3 DirLightCalc(vec3 colour);
vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);

void main(void)
{
	vec3 final_colour = DirLightCalc(vec3(0, 0, 0));
	fragment_colour = vec4(final_colour, 1.0);
}


vec3 DirLightCalc(vec3 colour)
{
	for (int i = 0; i < maxDirectionalLights; i++)
	{
		DirectionalLight dir = directionalLights[i];

		float scaler = max(0.0, dot(normalize(vertexNormal), dir.direction));

		vec3 diffuseIntensity = (dir.intensity * scaler);

		if (vert_is_vertex_shiney > 0)
			colour +=  diffuseIntensity + SpecularLight(dir.direction, dir.intensity);
		else
			colour += diffuseIntensity;
	}

	return colour;
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