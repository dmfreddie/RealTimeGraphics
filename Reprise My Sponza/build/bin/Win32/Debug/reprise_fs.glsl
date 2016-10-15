#version 440

struct SpotLight
{
	vec3 position;
	float range;
	vec3 direction;
	float coneAngle;
	vec3 intensity;
	bool castShadow;
};

layout (std140, binding = 0) uniform SpotLightBlock {
	SpotLight spotLights [15];
};

uniform float MAX_SPOT_LIGHTS;



struct PointLight
{
	vec3 position;
	float range;
	vec3 intensity;
	float padding;
};

layout (std140, binding=1) uniform PointLightBlock {
	PointLight pointLights [22];
};

uniform float MAX_LIGHTS;



struct DirectionalLight
{
	vec3 direction;
	float padding1;
	vec3 intensity;
	float padding2;
};

layout (std140, binding = 2) uniform DirectionalLightBlock {
	DirectionalLight directionalLights [5];
};
uniform float MAX_DIR_LIGHTS;


uniform vec3 vertex_ambient_colour;
uniform vec3 global_ambient_light;

uniform vec3 camera_position;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;


layout (binding=3) uniform sampler2DArray textureArray;



uniform float specular_smudge_factor;

in vec3 vertexPos;
in vec3 vertexNormal;
in vec2 text_coord;
in vec3 vert_diffuse_colour;
in vec3 vert_specular_colour;
in float vert_is_vertex_shiney;
flat in int vert_diffuse_texture_ID;

out vec4 fragment_colour;



vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);
vec3 DiffuseLight(int currentLight, float attenuation);
vec3 PointLightCalc(vec3 colour);
vec3 DirLightCalc(vec3 colour);
vec3 SpotLightCalc(vec3 colour);

void main(void)
{
	vec3 final_colour = global_ambient_light * vert_diffuse_colour;
	final_colour = DirLightCalc(final_colour);
	final_colour = SpotLightCalc(final_colour);
	final_colour = PointLightCalc(final_colour);

	//final_colour *= texture(textureArray, vec3(text_coord.x, text_coord.y, vert_diffuse_texture_ID)).xyz;
	
	/*if (has_diff_tex > 0)
		*/

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
		return (vert_specular_colour * texture2D(specular_texture, text_coord).rgb) * specularIntensity * specular_smudge_factor;
	}
	return vec3(0, 0, 0);
}

/*
Calculate the diffuse light for the point light and apply the diffuse texture.
Also call the specular for that light and add it to the diffuse value
@param currentLight - the light which the diffuse calculations need to be applied on
@param attenuation - the distance the light has an effect on
@return diffuseColour - the end result of the individual lights lighting calculation
//TODO : Refactor
*/
vec3 DiffuseLight(int currentLight, float attenuation)
{
	PointLight pointLight = pointLights[currentLight];
	vec3 L = normalize(pointLight.position - vertexPos);
	float scaler = max(0, dot(L, normalize(vertexNormal))) * attenuation;

	if (scaler == 0)
		return vec3(0, 0, 0);

	vec3 diffuse_intensity = pointLight.intensity * scaler;
	vec3 diffuseMat = diffuse_intensity;


	if (vert_is_vertex_shiney > 0)
	{
		return  diffuseMat + SpecularLight(L, diffuse_intensity);
	}

	return  pointLight.intensity * diffuseMat;
}
/*
Calculate the colour value for the light and add it to the total light for the pixel
@param currentLight - the light which the diffuse calculations need to be applied on
@return colour - the final colour for theat fragment after all point lighting calculations
TODO: Refactor
*/
vec3 PointLightCalc(vec3 colour)
{
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		PointLight pointLight = pointLights[i];
		float dist = distance(pointLight.position, vertexPos);
		float attenuation = 1 - smoothstep(0.0, pointLight.range, dist);

		if (attenuation > 0)
		{
			colour += DiffuseLight(i, attenuation);
		}
	}
	return colour;
}

vec3 DirLightCalc(vec3 colour)
{
	for (int i = 0; i < MAX_DIR_LIGHTS; i++)
	{
		DirectionalLight dir = directionalLights[i];

		float fDiffuseIntensity = max(0.0, dot(normalize(vertexNormal), dir.direction));
		colour += (dir.intensity * fDiffuseIntensity);
	}

	return colour;
}

vec3 SpotLightCalc(vec3 colour)
{
	for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
	{
		SpotLight spot = spotLights[i];
		

		vec3 L = spot.position - vertexPos;
		// Length of light vector (used for height attenuation).;
		float distToLight = length(L);
		// Normalize light vector.
		L = normalize(L);

		// Compute smoothed dual-cone effect.
		float cosDir = dot(L, -spot.direction);
		float spotEffect = smoothstep(cos(spot.coneAngle), cos(spot.coneAngle / 2), cosDir);

		// Compute height attenuation based on distance from earlier.
		float attenuation = smoothstep(spot.range, 0.0f, distToLight);

		float scaler = max(0, dot(L, normalize(vertexNormal))) * attenuation;
		vec3 diffuse_intensity = spot.intensity * scaler;

		
		if (vert_is_vertex_shiney > 0.0)
			diffuse_intensity += SpecularLight(L, diffuse_intensity);

		colour += (diffuse_intensity * spotEffect * attenuation);

	}

	return colour;
}