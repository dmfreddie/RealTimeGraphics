#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;

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


layout(std140) uniform MaterialDataBlock
{
	Material materials[30];
};

layout (location = 0) out vec3 reflected_light;

vec3 DirLightCalc(vec3 colour);
vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);

vec3 vertexNormal = vec3(0,0,0);
vec3 vertexPos = vec3(0,0,0);
int matID = 0;

void main(void)
{
	
	vec3 final_colour = DirLightCalc(vec3(0, 0, 0));
	reflected_light = final_colour;
}


vec3 DirLightCalc(vec3 colour)
{
	matID = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).a);
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vertexNormal = normalize(texel_N);

	for (int i = 0; i < maxDirectionalLights; i++)
	{
		DirectionalLight dir = directionalLight[i];

		float scaler = max(0.0, dot(normalize(vertexNormal), dir.direction));

		vec3 diffuseIntensity = (dir.intensity * scaler);
		
		if (materials[matID].vertexShineyness > 0)
			colour +=  diffuseIntensity + SpecularLight(dir.direction, dir.intensity);
		else
			colour += diffuseIntensity * materials[matID].diffuseColour;
	}

	return colour;
}

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
