#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;

uniform int currentPointLight;

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


layout(std140) uniform MaterialBlock
{
	Material materials[30];
};


out vec3 reflected_light;

vec3 vertexPos = vec3(0,0,0);
vec3 vertexNormal = vec3(0,0,0);
int matID = 0;

vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);
vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation);
vec3 PointLightCalc();

void main(void)
{
	vec3 vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	matID = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).r);
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vertexNormal = normalize(texel_N);

	vec3 colour = PointLightCalc();

	reflected_light = colour; // 0.5 + 0.5 * N;
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
	vec3 vertexToEye = normalize(cameraPosition - vertexPos);
	float specularFactor = max(0.0, dot(vertexToEye, lightReflection));

	if (specularFactor > 0)
	{
		vec3 specularIntensity = diffuse_intensity * pow(specularFactor, materials[matID].vertexShineyness);
		//if(useTextures)
		//	specularIntensity *= texture2DArray(specularTextureArray, vec3(text_coord, vert_diffuse_texture_ID)).rgb;
		return materials[matID].specularColour * specularIntensity;
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
	float scaler = max(0, dot(L, vertexNormal)) * attenuation;

	if (scaler == 0)
		return vec3(0, 0, 0);

	vec3 diffuse_intensity = lightIntensity * scaler * materials[matID].diffuseColour;
	
	if (materials[matID].vertexShineyness > 0)
		return  diffuse_intensity + SpecularLight(L, diffuse_intensity);
	else
		return  diffuse_intensity;

}

/*
Calculate the colour value for the light and add it to the total light for the pixel
@param currentLight - the light which the diffuse calculations need to be applied on
@return colour - the final colour for theat fragment after all point lighting calculations
*/
vec3 PointLightCalc()
{
	float dist = distance(pointLight[currentPointLight].position, vertexPos);
	float attenuation = 1-smoothstep(0.0, pointLight[currentPointLight].range, dist);

	vec3 colour = DiffuseLight(pointLight[currentPointLight].position, pointLight[currentPointLight].intensity, attenuation);
	
	
	return colour;
}
