#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;
layout (location = 3) uniform sampler2DArray textureArray;

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

struct PBRMaterial
{
	vec3 diffuseColour;
	float metallic;
	vec3 specularColour;
	float roughness;
	float vertexShineyness;
	float ambientOcclusion;
	int diffuseTextureID;
};

layout(std140) uniform PBRMaterialDataBlock
{
	PBRMaterial pbrMaterials[30];
};


out vec3 reflected_light;
int materialIndex = 0;
vec3 vertexPosition;
vec3 vertexNormal;
vec2 vertexUV;
uniform bool useTextures;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

void main(void)
{
	vertexPosition = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	vertexNormal = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vertexUV = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rg;
	materialIndex = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).b);

	PBRMaterial pbrMat = pbrMaterials[materialIndex];

	vec3 final_colour = vec3(0.0);

	vec3 N = vertexNormal;
	vec3 V = normalize(cameraPosition - vertexPosition);
	vec3 R = reflect(-V, N); 
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use their albedo color as F0 (metallic workflow)    
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, pbrMat.diffuseColour, pbrMat.metallic);
	vec3 F   = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, pbrMat.roughness); // use modified Fresnel-Schlick approximation to take roughness into account

	// kS is equal to Fresnel
	vec3 kS = F;
	// for energy conservation, the diffuse and specular light can't
	// be above 1.0 (unless the surface emits light); to preserve this
	// relationship the diffuse component (kD) should equal 1.0 - kS.
	vec3 kD = vec3(1.0) - kS;
	// multiply kD by the inverse metalness such that only non-metals 
	// have diffuse lighting, or a linear blend if partly metal (pure metals
	// have no diffuse light).
	kD *= 1.0 - pbrMat.metallic;

	// reflectance equation
	vec3 Lo = vec3(0.0);

	for(int i = 0; i < 2; ++i)
	{
		DirectionalLight currentDirectionalLight = directionalLight[i];

		// calculate per-light radiance
		vec3 L = normalize(currentDirectionalLight.direction);
		vec3 H = normalize(V + L);


		vec3 radiance = currentDirectionalLight.intensity;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(N, H, pbrMat.roughness);   
		float G   = GeometrySmith(N, V, L, pbrMat.roughness);      
           
		vec3 nominator    = NDF * G * F; 
		float denominator = 4 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0); // 0.001 to prevent divide by zero.
		if(denominator == 0.0)
			denominator = 0.001;
		vec3 brdf = nominator / denominator;

		// scale light by NdotL
		float NdotL = max(dot(N, L), 0.0);        

		// add to outgoing radiance Lo
		Lo += (kD * pbrMat.diffuseColour / PI + brdf) * radiance * NdotL ;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}
	// ambient lighting (note that the next IBL tutorial will replace 
	// this ambient lighting with environment lighting).
	vec3 ambient = pbrMat.diffuseColour * pbrMat.ambientOcclusion;

	if(useTextures && materialIndex < 27)
		ambient = texture(textureArray, vec3(vertexUV, pbrMat.diffuseTextureID)).xyz * pbrMat.ambientOcclusion;


	vec3 colour =/* ambient +*/ Lo;

	// HDR tonemapping
	colour = colour / (colour + vec3(1.0));
	// gamma correct
	colour = pow(colour, vec3(1.0/2.2)); 

	final_colour = colour;
	
	
	reflected_light = final_colour;
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------