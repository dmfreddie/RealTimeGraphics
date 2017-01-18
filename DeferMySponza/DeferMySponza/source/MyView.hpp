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
#include "Light.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

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


const glm::mat4 BiasMatrix(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
);

struct MaterialDatabase
{
	std::vector<PBRMaterial> materials;
	std::vector<std::string> textures;
	int materialCount, textureCount;
};

class MyView : public tygra::WindowViewDelegate
{
public:

    MyView();

    ~MyView();

    void setScene(const scene::Context * scene);

	void Stop(tygra::Window* window);
	void UseTextures(const bool useTextures_);
	const bool UseTextures() const;
	void EnableShadows(const bool enableShadows_);
	const bool ShadowStatus() const;

private:
	MaterialDatabase LoadMaterialDatabase(const char* path);

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
	
#pragma endregion 

#pragma  region Shaders
	Shader *gbufferShadr, *ambientLightShader, *pointLightShader, *directionalLightShader, *spotlightShader, *shadowDepth_Shader;
	//Shader* shaders[6] = { gbufferShadr, ambientLightShader, pointLightShader, directionalLightShader, spotlightShader, shadowDepth_Shader };
#pragma endregion 

	DataBlock lightingData; 
	GLuint lightDataUBO;
	
	PBRMaterialDataBlock pbrMaterialData;
	GLuint pbrMaterialHandle;
	bool usePBRMaterials = true;


	GLuint diffuse_texture_array_handle;
	bool useTextures = false;
	bool enableShadows = false;
	const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
};
