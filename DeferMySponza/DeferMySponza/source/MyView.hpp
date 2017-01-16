#pragma once
#define TGL_TARGET_GL_4_5
#include <scene/scene_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

#include "Shader.h"
#include <map>


#define AREATEX_WIDTH 160
#define AREATEX_HEIGHT 560
#define SEARCHTEX_WIDTH 66
#define SEARCHTEX_HEIGHT 33

struct DrawElementsIndirectCommand
{
	GLuint vertexCount;
	GLuint instanceCount;
	GLuint firstVertex;
	GLuint baseVertex;
	GLuint baseInstance;
};

struct MeshVertex
{
	glm::vec2 uvcoords;
	glm::vec2 position;
	
};

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texcoord;

};

struct MeshGL
{
	GLuint first_element_index;
	GLuint element_count;
	GLuint first_vertex_index;
};



struct DirectionalLight
{
	glm::vec3 direction;
	float padding1;
	glm::vec3 intensity;
	float padding2;
};

struct PointLight
{
	glm::vec3 position;
	float range;
	glm::vec3 intensity;
	float padding;
};

struct SpotLight
{
	glm::vec3 position;
	float range;
	glm::vec3 direction;
	float coneAngle;
	glm::vec3 intensity;
	bool castShadow;
};

struct AmbientLightBlock
{
	glm::vec3 ambient_light;
	float padding;
};

struct DataBlock
{
	PointLight pointLight[20];
	AmbientLightBlock ambientLight;
	DirectionalLight directionalLight[2];
	SpotLight spotLight[5];
	glm::vec3 cameraPosition;
	float maxPointLights;	
	float maxDirectionalLights;
	float maxSpotlights;
};

struct PBRMaterial
{
	glm::vec3 diffuseColour;
	float metallic;
	glm::vec3 specularColour;
	float roughness = 0.5f;
	float vertexShineyness ;
	float ambientOcclusion;
	int diffuseTextureID;
	float padding = 0.0f;
};


struct PBRMaterialDataBlock
{
	PBRMaterial materials[30];
};

const glm::mat4 BiasMatrix(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
);

class MyView : public tygra::WindowViewDelegate
{
public:

    MyView();

    ~MyView();

    void setScene(const scene::Context * scene);

	void Stop(tygra::Window* window);
	void UseTextures(const bool useTextures_);
	const bool UseTextures() const;
private:

    void windowViewWillStart(tygra::Window * window) override;

    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;

    void windowViewRender(tygra::Window * window) override;
	void LoadTextureArray(std::vector<std::string>& textureNames, Shader* shader, GLuint& textureArrayHandle, const char* samplerHandle);
	void LoadBlankTextureArray(int textureCount, Shader* shader, GLuint& textureArrayHandle, const char* samplerHandle);
	void SetFromExisteingTextureArray(GLuint& textureArrayHandle, Shader* shader, const char* samplerHandle);

	bool CheckError();

    const scene::Context * scene_;
	DrawElementsIndirectCommand commands[30];
	std::map<scene::MeshId, MeshGL> meshes_;
	std::unordered_map<std::string, GLuint> textures;
	std::unordered_map<std::string, GLuint> uniforms;
	std::vector<glm::mat4> matrices, pointLightMatricies, spotlightMatricies;

	GLuint vao; // VertexArrayObject for the shape's vertex array settings
	GLuint vertex_vbo;
	GLuint element_vbo; // VertexBufferObject for the elements (indices)
	GLuint instance_vbo; // VertexBufferObject for the model xforms
	GLuint material_vbo;
	GLuint commandBuffer;

	GLuint pointLightMatrix_vbo;
	GLuint spotLightMatrix_vbo;

#pragma region GBuffer
	struct Mesh
	{
		GLuint vertex_vbo{ 0 };
		GLuint element_vbo{ 0 };
		GLuint vao{ 0 };
		int element_count{ 0 };
	};

	Mesh light_quad_mesh_; // vertex array of vec2 position
	Mesh light_sphere_mesh_; // element array into vec3 position
	Mesh light_cone_mesh_; // element array into vec3 position

	GLuint gbuffer_position_tex_{ 0 };
	GLuint gbuffer_normal_tex_{ 0 };
	GLuint gbuffer_depth_tex_{ 0 };
	GLuint gbuffer_material_tex_{ 0 };

	GLuint gbuffer_fbo_{ 0 };

	GLuint lbuffer_fbo_{ 0 };
	GLuint lbuffer_colour_rbo_{ 0 };

	GLuint shadowMapFrameBuffer{ 0 };
	GLuint shadowmap_tex;
	
	GLuint aaTexture;
	GLuint albedo_tex;
	GLuint edge_tex;
	GLuint blend_tex;
	GLuint area_tex;
	GLuint search_tex;

	GLuint albedo_fbo;
	GLuint albedo_rbo;
	GLuint edge_fbo;
	GLuint edge_rbo;
	GLuint blend_fbo;
	GLuint blend_rbo;
#pragma endregion 

#pragma  region Shaders
	Shader *gbufferShadr, *ambientLightShader, *pointLightShader, *directionalLightShader, *spotlightShader, *edge_shader, *blend_shader, *neighborhood_shader, *shadowDepth_Shader;
#pragma endregion 

	DataBlock lightingData; 
	GLuint lightDataUBO;
	
	PBRMaterialDataBlock pbrMaterialData;
	GLuint pbrMaterialHandle;
	bool usePBRMaterials = true;


	GLuint diffuse_texture_array_handle;
	bool useTextures = false;

	bool enableSMAA = false;

	const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
};
