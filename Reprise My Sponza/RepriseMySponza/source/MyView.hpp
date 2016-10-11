#pragma once
#define TGL_TARGET_GL_4_4
#include <scene/scene_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include <map>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 texcoord;
};

struct MeshGL
{
	GLuint first_element_index;
	GLuint element_count;
	GLuint first_vertex_index;
	int mesh_id;
};

struct PointLight
{
	glm::vec3 position;
	float range;
	glm::vec3 direction;
	float field_of_view;
	glm::vec3 intensity;
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

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 intensity;
};

class MyView : public tygra::WindowViewDelegate
{
public:

    MyView();

    ~MyView();

    void setScene(const scene::Context * scene);

private:

    void windowViewWillStart(tygra::Window * window) override;

    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;

    void windowViewRender(tygra::Window * window) override;

	void CompileShader(std::string shaderFileName, GLenum shaderType, GLuint& shaderVariable);
	bool CheckLinkStatus(GLuint shaderProgram);
	void CompileShaders();
	void Getuniforms();
	void LoadTexture(std::string textureName);
	void ResetConsole();


    const scene::Context * scene_;

	std::map<scene::MeshId, MeshGL> meshes_;
	std::unordered_map<std::string, GLuint> textures;
	std::unordered_map<std::string, GLuint> uniforms;

	std::vector<PointLight> pointLights;
	std::vector<DirectionalLight> directionalLights;
	std::vector<SpotLight> spotLights;

	GLuint shaderProgram;
	GLuint vertex_shader;
	GLuint fragment_shader;



	GLuint vertex_vbo;
	GLuint element_vbo; // VertexBufferObject for the elements (indices)
	GLuint instance_vbo; // VertexBufferObject for the model xforms
	GLuint vao; // VertexArrayObject for the shape's vertex array settings
	GLuint ubo = 0;
};
