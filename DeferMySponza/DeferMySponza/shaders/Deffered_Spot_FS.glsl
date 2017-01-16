#version 440

layout (location = 0) uniform sampler2DRect sampler_world_position;
layout (location = 1) uniform sampler2DRect sampler_world_normal;
layout (location = 2) uniform sampler2DRect sampler_world_material;
layout (location = 3) uniform sampler2DArray textureArray;
layout (location = 4) uniform sampler2D shadowMap;


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

flat in int InstanceID;

const float PI = 3.14159265359;

vec3 SpecularLight(vec3 LVector, vec3 diffuse_intensity);
vec3 DiffuseLight(vec3 lightPosition, vec3 lightIntensity, float attenuation);
vec3 SpotLightCalc(vec3 colour);
float ShadowCalculation();
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

vec3 vertexPos = vec3(0,0,0);
vec3 vertexNormal = vec3(0,0,0);
int index = 0;
uniform bool useTextures;
uniform mat4 lightSpaceMatrix;
//vec4 fraglightspacePos;
int materialIndex;

vec3 vertexPosition;
vec2 vertexUV;

void main(void)
{
	vec3 texel_N = texelFetch(sampler_world_normal, ivec2(gl_FragCoord.xy)).rgb;
	vertexPos = texelFetch(sampler_world_position, ivec2(gl_FragCoord.xy)).rgb;
	index = int(texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).b);
	vertexNormal = normalize(texel_N);
	vertexPosition = vertexPos;
	materialIndex = index;
	vertexUV = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rg;
	//fraglightspacePos = lightSpaceMatrix * vec4(vertexPos, 1.0f);

	vec3 final_colour = SpotLightCalc(vec3(0,0,0));

	vec2 uv = texelFetch(sampler_world_material, ivec2(gl_FragCoord.xy)).rg;

	if(useTextures && index < 27)
		final_colour *= texture(textureArray, vec3(uv, pbrMaterials[index].diffuseTextureID)).xyz;


	float shadow = ShadowCalculation();

	reflected_light = vec3(shadow);// vec3(1 - shadow, 1 - shadow, 1 - shadow); // (1.0 - shadow) * final_colour;
}
uniform mat4 lightSpaceMatrixUniform;

float ShadowCalculation()
{
	vec4 fragPosLightSpace = lightSpaceMatrixUniform * vec4(vertexPos, 1.0);
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, projCoords.xy).r;

	// Get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	float bias = 0;// .005;
	// Check whether current frag pos is in shadow


	return smoothstep(0.99, 1.0, currentDepth);
	return smoothstep(0.99, 1.0, closestDepth);


	return  (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
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
	
	vec3 texel_M = pbrMaterials[index].diffuseColour;
	vec3 L = normalize(lightPosition - vertexPos);
	float scaler = max(0, dot(L, normalize(vertexNormal))) * attenuation;
	
	if (scaler == 0)
		return vec3(0, 0, 0);

	vec3 diffuse_intensity = lightIntensity * scaler * texel_M;
	
	//float shadow = ShadowCalculation(fraglightspacePos);

	if (pbrMaterials[index].vertexShineyness > 0)
		return  diffuse_intensity +/* (1.0 - shadow) + */SpecularLight(L, diffuse_intensity);
	else
		return  diffuse_intensity;

}

vec3 SpotLightCalc(vec3 colour)
{
	
	SpotLight spot = spotLight[InstanceID];
	PBRMaterial pbrMat = pbrMaterials[materialIndex];

	float dist = length(spot.position - vertexPosition);
	float attenuation = 1 - smoothstep(0.0, spot.range, dist);
	
	if(attenuation <= 0.0)
		return vec3(0,0,0);
 
	
	// Compute smoothed dual-cone effect.
	//float cosDir = dot(normalize(spot.position - vertexPos), spot.direction);
	//float spotEffect = smoothstep(cos(spot.coneAngle), cos(spot.coneAngle / 2), cosDir);
	//spotEffect = intensity;

	// Compute smoothed dual-cone effect.
	float cosDir = dot(normalize(vertexPos - spot.position), spot.direction);
	float spotEffect = smoothstep(cos(spot.coneAngle), cos(spot.coneAngle / 2), cosDir);


	//vec3 L = normalize(lightPosition - vertexPos);
	//float SpotFactor = dot(L, spot.direction);

	if(spotEffect <= 0.0)
		return vec3(0,0,0);

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

	// calculate per-light radiance
	vec3 L = normalize(spot.position - vertexPosition);
	vec3 H = normalize(V + L);
		

	vec3 radiance = spot.intensity;

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

	// ambient lighting (note that the next IBL tutorial will replace 
	// this ambient lighting with environment lighting).
	vec3 ambient  = pbrMat.diffuseColour * pbrMat.ambientOcclusion;

	if(useTextures && materialIndex < 27)
		ambient = texture(textureArray, vec3(vertexUV, pbrMat.diffuseTextureID)).xyz  * pbrMat.ambientOcclusion;

	vec3 finalColour = ambient + Lo;

	// HDR tonemapping
	finalColour = finalColour / (finalColour + vec3(1.0));
	// gamma correct
	finalColour = pow(finalColour, vec3(1.0/2.2)); 

	vec3 final_idv_colour =  finalColour * radiance * attenuation * spotEffect;// (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - spot.coneAngle));
	//finalColour += final_idv_colour ;
	
	return final_idv_colour ;

	

	//float dist = distance(spot.position, vertexPos);
	//float attenuation = 1 - smoothstep(0.0, spot.range, dist);
	//// Compute height attenuation based on distance from earlier.
	////float attenuation = smoothstep(spot.range, 0.0f, length(spot.position - vertexPos));
	
	

	//vec3 diffuse_intensity = DiffuseLight(spot.position, spot.intensity, attenuation);
				

	//colour += (diffuse_intensity * spotEffect);

	

	//return colour;
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
		vec3 specularIntensity = diffuse_intensity * pow(specularFactor, pbrMaterials[index].vertexShineyness);
		//if(useTextures)
		//	specularIntensity *= texture2DArray(specularTextureArray, vec3(text_coord, vert_diffuse_texture_ID)).rgb;
		return pbrMaterials[index].specularColour * specularIntensity;
	}
	return vec3(0, 0, 0);
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