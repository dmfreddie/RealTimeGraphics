#include "MyView.hpp"
#include <scene/scene.hpp>
#include <tygra/FileHelper.hpp>
#include <tsl/shapes.hpp>
#include <tcf/Image.hpp>
#include <tcf/tcf.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cassert>

// SMAA adapted from: https://github.com/scrawl/smaa-opengl
#define SMAA_GLSL_4 1
#include "smaa_glsl.h"

MyView::MyView()
{
}

MyView::~MyView() {
}

void MyView::setScene(const scene::Context * scene)
{
    scene_ = scene;
}

void MyView::Stop(tygra::Window * window)
{
	this->windowViewDidStop(window);
}

void MyView::UseTextures(const bool useTextures_)
{
	useTextures = useTextures_;
}

const bool MyView::UseTextures() const
{
	return useTextures;
}

void MyView::windowViewWillStart(tygra::Window * window)
{
    assert(scene_ != nullptr);

	/*
	*			This section of code creates a fullscreen quad to be used
	*           when computing global illumination effects (e.g. ambient)
	*/
#pragma region Create Fullscreen quad
		std::vector<glm::vec2> vertices(4);
		vertices[0] = glm::vec2(-1, -1);
		vertices[1] = glm::vec2(1, -1);
		vertices[2] = glm::vec2(1, 1);
		vertices[3] = glm::vec2(-1, 1);

		glGenBuffers(1, &light_quad_mesh_.vertex_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, light_quad_mesh_.vertex_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			vertices.size() * sizeof(glm::vec2),
			vertices.data(),
			GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &light_quad_mesh_.vao);
		glBindVertexArray(light_quad_mesh_.vao);
		glBindBuffer(GL_ARRAY_BUFFER, light_quad_mesh_.vertex_vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
			sizeof(glm::vec2), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
#pragma endregion 

	/*
		*           This code creates a sphere to use when deferred shading
		*           with a point light source.
		*/
#pragma region Create sphere mesh
		tsl::IndexedMeshPtr mesh = tsl::createSpherePtr(1.f, 12);
		mesh = tsl::cloneIndexedMeshAsTriangleListPtr(mesh.get());

		light_sphere_mesh_.element_count = mesh->indexCount();

		glGenBuffers(1, &light_sphere_mesh_.vertex_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, light_sphere_mesh_.vertex_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			mesh->vertexCount() * sizeof(glm::vec3),
			mesh->positionArray(),
			GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &light_sphere_mesh_.element_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_sphere_mesh_.element_vbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			mesh->indexCount() * sizeof(unsigned int),
			mesh->indexArray(),
			GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &light_sphere_mesh_.vao);
		glBindVertexArray(light_sphere_mesh_.vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_sphere_mesh_.element_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, light_sphere_mesh_.vertex_vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(glm::vec3), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
#pragma endregion 

	/*
		*           This code creates a cone to use when deferred shading
		*           with a spot light source.
		*/
#pragma region Create cone mesh
		tsl::IndexedMeshPtr coneMesh = tsl::createConePtr(1.0f, 1.0f, 12);
		coneMesh = tsl::cloneIndexedMeshAsTriangleListPtr(coneMesh.get());

		light_cone_mesh_.element_count = coneMesh->indexCount();

		glGenBuffers(1, &light_cone_mesh_.vertex_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, light_cone_mesh_.vertex_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			coneMesh->vertexCount() * sizeof(glm::vec3),
			coneMesh->positionArray(),
			GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &light_cone_mesh_.element_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_cone_mesh_.element_vbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			coneMesh->indexCount() * sizeof(unsigned int),
			coneMesh->indexArray(),
			GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &light_cone_mesh_.vao);
		glBindVertexArray(light_cone_mesh_.vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_cone_mesh_.element_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, light_cone_mesh_.vertex_vbo);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(glm::vec3), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
#pragma endregion 

#pragma region Shaders
	gbufferShadr = new Shader("Deffered_GBuffer_VS.glsl", "Deffered_GBuffer_FS.glsl");
	ambientLightShader = new Shader("Deffered_Ambient_VS.glsl", "Deffered_Ambient_FS.glsl");
	directionalLightShader = new Shader("Deffered_Directional_VS.glsl", "Deffered_Directional_FS.glsl");
	pointLightShader = new Shader("Deffered_Point_VS.glsl", "Deffered_Point_FS.glsl");
	spotlightShader = new Shader("Deffered_Spot_VS.glsl", "Deffered_Spot_FS.glsl");
	shadowDepth_Shader = new Shader("Deffered_Directional_Shadow_VS.glsl", "Deffered_Directional_Shadow_FS.glsl");
	gbufferShadr->Bind();
	gbufferShadr->GetUniformLocation("projection_view");
	gbufferShadr->Unbind();
	ambientLightShader->Bind();
	ambientLightShader->GetUniformLocation("projection_view");
	ambientLightShader->Unbind();
#pragma endregion

#pragma region TextureArrays

	std::vector<std::string> diffuseTextureNames;

	
	diffuseTextureNames.push_back("content:///vase_dif.png");			// 0
	diffuseTextureNames.push_back("content:///Hook.png");				// 1
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");	// 2
	diffuseTextureNames.push_back("content:///lion.png");				// 3
	diffuseTextureNames.push_back("content:///vase_round.png");			// 4
	diffuseTextureNames.push_back("content:///background.png");			// 5
	diffuseTextureNames.push_back("content:///flagPole.png");			// 6
	diffuseTextureNames.push_back("content:///spnza_bricks_a_diff.png");// 7
	diffuseTextureNames.push_back("content:///sponza_floor_a_diff.png");// 8
	diffuseTextureNames.push_back("content:///sponza_fabric_green_diff.png");// 9
	diffuseTextureNames.push_back("content:///sponza_roof_diff.png");	// 10
	diffuseTextureNames.push_back("content:///sponza_flagpole_diff.png");// 11
	diffuseTextureNames.push_back("content:///chain_texture.png");		// 12
	diffuseTextureNames.push_back("content:///sponza_curtain_diff.png");// 13

	LoadTextureArray(diffuseTextureNames, ambientLightShader, diffuse_texture_array_handle, "textureArray");
	SetFromExisteingTextureArray(diffuse_texture_array_handle, directionalLightShader, "textureArray");
	SetFromExisteingTextureArray(diffuse_texture_array_handle, pointLightShader, "textureArray");
	SetFromExisteingTextureArray(diffuse_texture_array_handle, spotlightShader, "textureArray");
#pragma endregion 

#pragma region Load the mesh into buffers
	scene::GeometryBuilder builder;
	std::vector<Vertex> vertices_;
	std::vector<unsigned int> elements;
	std::vector<Material> materials;
	
	

	const auto& scene_meshes = builder.getAllMeshes();
	for (const auto& scene_mesh : scene_meshes) {

		MeshGL& newMesh = meshes_[scene_mesh.getId()];
		const auto& source_mesh = builder.getMeshById(scene_mesh.getId());
		const auto& positions = source_mesh.getPositionArray();
		const auto& elementsArr = source_mesh.getElementArray();
		const auto& normals = source_mesh.getNormalArray();
		const auto& text_coord = source_mesh.getTextureCoordinateArray();

		bool hasTexCood = text_coord.size() > 0;

		newMesh.first_vertex_index = (GLuint)vertices_.size();
		vertices_.reserve(vertices_.size() + positions.size());

		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			Vertex vertex;
			vertex.position = (const glm::vec3&)positions[i];
			vertex.normal = (const glm::vec3&)normals[i];
			if (hasTexCood)
				vertex.texcoord = (const glm::vec2&)text_coord[i];
			vertices_.push_back(vertex);
		}

		newMesh.first_element_index = (GLuint)elements.size();
		elements.insert(elements.end(), elementsArr.begin(), elementsArr.end());
		newMesh.element_count = (GLuint)elementsArr.size();

	}

	CheckError();

	int materialIDCount = 0;
	int counter = 0;
	for (const auto &ent1 : meshes_)
	{

		auto& instances = scene_->getInstancesByMeshId(ent1.first);

		for (const auto& instance : instances)
		{
			const auto& inst = scene_->getInstanceById(instance);
			glm::mat4 model_xform = glm::mat4((const glm::mat4x3&)inst.getTransformationMatrix());
			matrices.push_back(model_xform);

			scene::Material material = scene_->getMaterialById(inst.getMaterialId());
			Material mat;
			mat.diffuseColour = (const glm::vec3&)material.getDiffuseColour();
			mat.vertexShineyness = material.getShininess();
			mat.specularColour = (const glm::vec3&)material.getSpecularColour();
			mat.diffuseTextureID = materialIDCount;
			materials.push_back(mat);
		}
		materialData.materials[materialIDCount] = materials[counter];
		counter += (int)instances.size();
		materialIDCount++;

	}

	// Set the materials diffuse texture id to the corresponsing texture
	materialData.materials[0].diffuseTextureID = 0;
	materialData.materials[1].diffuseTextureID = 1;
	materialData.materials[2].diffuseTextureID = 2;
	materialData.materials[3].diffuseTextureID = 3;
	materialData.materials[4].diffuseTextureID = 4;
	materialData.materials[5].diffuseTextureID = 2;
	materialData.materials[6].diffuseTextureID = 5;
	materialData.materials[7].diffuseTextureID = 6;
	materialData.materials[8].diffuseTextureID = 2;
	materialData.materials[9].diffuseTextureID = 7;
	
	materialData.materials[10].diffuseTextureID = 2;
	materialData.materials[11].diffuseTextureID = 8;
	materialData.materials[12].diffuseTextureID = 9;
	materialData.materials[13].diffuseTextureID = 10;
	materialData.materials[14].diffuseTextureID = 11;
	materialData.materials[15].diffuseTextureID = 2;
	materialData.materials[16].diffuseTextureID = 7;
	materialData.materials[17].diffuseTextureID = 7;
	materialData.materials[18].diffuseTextureID = 2;
	materialData.materials[19].diffuseTextureID = 12;
	
	materialData.materials[20].diffuseTextureID = 4;
	materialData.materials[21].diffuseTextureID = 2;
	materialData.materials[22].diffuseTextureID = 2;
	materialData.materials[23].diffuseTextureID = 2;
	materialData.materials[24].diffuseTextureID = 13;
	materialData.materials[25].diffuseTextureID = 10;
	materialData.materials[26].diffuseTextureID = 2;

	CheckError();

	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices_.size() * sizeof(Vertex), // size of data in bytes
		vertices_.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CheckError();

	glGenBuffers(1, &element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, element_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		elements.size() * sizeof(unsigned int), // size of data in bytes
		elements.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CheckError();

	glGenBuffers(1, &material_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, material_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		materials.size() * sizeof(Material),
		materials.data(),
		GL_STATIC_DRAW
		);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CheckError();

	glGenBuffers(1, &instance_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		matrices.size() * sizeof(glm::mat4),
		matrices.data(),
		GL_DYNAMIC_DRAW
		);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CheckError();

#pragma region Setting Up Buffer

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, texcoord));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, material_vbo);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Material), TGL_BUFFER_OFFSET_OF(Material, diffuseColour));
	glVertexAttribDivisor(3, 1);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Material), TGL_BUFFER_OFFSET_OF(Material, specularColour));
	glVertexAttribDivisor(4, 1);
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Material), TGL_BUFFER_OFFSET_OF(Material, vertexShineyness));
	glVertexAttribDivisor(5, 1);
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Material), TGL_BUFFER_OFFSET_OF(Material, diffuseTextureID));
	glVertexAttribDivisor(6, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(7 + i);
		glVertexAttribPointer(7 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(7 + i, 1);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

#pragma endregion 

	CheckError();
#pragma endregion 

#pragma region Command Data
	int commandInt = 0;
	counter = 0;
	int baseInstance = 0;
	for (const auto &mesh : meshes_)
	{
		const auto& mesh_ = mesh.second;
		auto& instances = scene_->getInstancesByMeshId(mesh.first);

		counter += (int)instances.size();

		commands[commandInt].vertexCount = mesh_.element_count;
		commands[commandInt].instanceCount = (int)instances.size(); // Just testing with 1 instance, ATM.
		commands[commandInt].firstVertex = mesh_.first_element_index;// +sizeof(GLuint);
		commands[commandInt].baseVertex = mesh_.first_vertex_index;
		commands[commandInt].baseInstance = baseInstance; // Shouldn't impact testing?
		commandInt++;
		baseInstance = counter;
	}

	glGenBuffers(1, &commandBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER,
		sizeof(commands),
		&commands,
		GL_DYNAMIC_DRAW
		);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
#pragma endregion

	/*
	*			All of the framebuffers, renderbuffers and texture objects
	*           that you'll need for this tutorial are gen'd here but not
	*           created until windowViewDidReset because they are usually
	*           window size dependent.
	*/
#pragma region GBuffer Setup
	glGenTextures(1, &gbuffer_position_tex_);
	glGenTextures(1, &gbuffer_normal_tex_);
	glGenTextures(1, &gbuffer_depth_tex_);
	glGenTextures(1, &gbuffer_material_tex_);

	glGenFramebuffers(1, &gbuffer_fbo_);

	glGenFramebuffers(1, &lbuffer_fbo_);
	glGenRenderbuffers(1, &lbuffer_colour_rbo_);

	glGenTextures(1, &shadowmap_tex);
	glGenFramebuffers(1, &shadowMapFrameBuffer);
#pragma endregion 

	CheckError();

#pragma region Lighting setup	

	//Create the light vector so there will be memory already reserved that can just be overwritten if values have been changed. This has been done on 
	//start for effiences in the constant render loop function.
	auto& pointLightsRef = scene_->getAllPointLights();
	for (unsigned int i = 0; i < pointLightsRef.size(); ++i)
	{
		PointLight light;
		light.position = (const glm::vec3&) pointLightsRef[i].getPosition();
		light.range = pointLightsRef[i].getRange();
		light.intensity = (const glm::vec3&) pointLightsRef[i].getIntensity();
		light.padding = 0.0f;
		lightingData.pointLight[i] = light;
	}
	lightingData.maxPointLights = (float)pointLightsRef.size();


	auto& directionalLightRef = scene_->getAllDirectionalLights();
	for (unsigned int i = 0; i < directionalLightRef.size(); ++i)
	{
		DirectionalLight light;
		light.direction = (const glm::vec3&) directionalLightRef[i].getDirection();
		light.intensity = (const glm::vec3&) directionalLightRef[i].getIntensity();
		light.padding1 = 0.0f;
		light.padding2 = 0.0f;
		lightingData.directionalLight[i] = light;
	}
	lightingData.maxDirectionalLights = (float)directionalLightRef.size();

	auto& spotlightRef = scene_->getAllSpotLights();
	for (unsigned int i = 0; i < spotlightRef.size(); ++i)
	{
		SpotLight light;
		light.position = (const glm::vec3&) spotlightRef[i].getPosition();
		light.direction = (const glm::vec3&) spotlightRef[i].getDirection();
		light.intensity = (const glm::vec3&) spotlightRef[i].getIntensity();
		light.range = spotlightRef[i].getRange();
		light.coneAngle = spotlightRef[i].getConeAngleDegrees();
		light.castShadow = spotlightRef[i].getCastShadow();

		lightingData.spotLight[i] = light;
	}
	lightingData.maxDirectionalLights = (float)directionalLightRef.size();


	lightingData.ambientLight.ambient_light = (const glm::vec3&)scene_->getAmbientLightIntensity();

	CheckError();
#pragma endregion 

#pragma region UBO 
	
	glGenBuffers(1, &lightDataUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, lightDataUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(DataBlock), &lightingData, GL_STREAM_DRAW);

	ambientLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightDataUBO);
	glUniformBlockBinding(ambientLightShader->GetShaderID(), glGetUniformBlockIndex(ambientLightShader->GetShaderID(), "DataBlock"), 0);
	ambientLightShader->Unbind();

	directionalLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightDataUBO);
	glUniformBlockBinding(directionalLightShader->GetShaderID(), glGetUniformBlockIndex(directionalLightShader->GetShaderID(), "DataBlock"), 0);
	directionalLightShader->Unbind();


	pointLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightDataUBO);
	glUniformBlockBinding(pointLightShader->GetShaderID(), glGetUniformBlockIndex(pointLightShader->GetShaderID(), "DataBlock"), 0);
	pointLightShader->Unbind();

	spotlightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, lightDataUBO);
	glUniformBlockBinding(spotlightShader->GetShaderID(), glGetUniformBlockIndex(spotlightShader->GetShaderID(), "DataBlock"), 0);
	spotlightShader->Unbind();

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glGenBuffers(1, &materialDataUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, materialDataUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialDataBlock), &materialData, GL_DYNAMIC_DRAW);

	ambientLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, materialDataUBO);
	glUniformBlockBinding(ambientLightShader->GetShaderID(), glGetUniformBlockIndex(ambientLightShader->GetShaderID(), "MaterialDataBlock"), 1);
	ambientLightShader->Unbind();

	directionalLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, materialDataUBO);
	glUniformBlockBinding(directionalLightShader->GetShaderID(), glGetUniformBlockIndex(directionalLightShader->GetShaderID(), "MaterialDataBlock"), 1);
	directionalLightShader->Unbind();


	pointLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, materialDataUBO);
	glUniformBlockBinding(pointLightShader->GetShaderID(), glGetUniformBlockIndex(pointLightShader->GetShaderID(), "MaterialDataBlock"), 1);
	pointLightShader->Unbind();

	spotlightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, materialDataUBO);
	glUniformBlockBinding(spotlightShader->GetShaderID(), glGetUniformBlockIndex(spotlightShader->GetShaderID(), "MaterialDataBlock"), 1);
	spotlightShader->Unbind();
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
#pragma endregion 

	CheckError();

#pragma region SMAA

	edge_shader = new Shader(edge_vs.c_str(), edge_ps.c_str(), true);
	edge_shader->Bind();
	edge_shader->SetUniformIntValue("albedo_tex", 0);
	edge_shader->Unbind();


	blend_shader = new Shader(blend_vs.c_str(), blend_ps.c_str(), true);
	blend_shader->Bind();
	blend_shader->SetUniformIntValue("albedo_tex", 0);
	blend_shader->SetUniformIntValue("area_tex", 1);
	blend_shader->SetUniformIntValue("search_tex", 2);
	blend_shader->Unbind();

	neighborhood_shader = new Shader(neighborhood_vs.c_str(), neighborhood_ps.c_str(), true);
	neighborhood_shader->Bind();
	neighborhood_shader->SetUniformIntValue("albedo_tex", 0);
	neighborhood_shader->SetUniformIntValue("blend_tex", 1);
	neighborhood_shader->Unbind();
#pragma  endregion 
	CheckError();
}


void MyView::LoadTextureArray(std::vector<std::string>& textureNames, Shader* shader, GLuint& textureArrayHandle, const char* samplerHandle)
{
	shader->Bind();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &textureArrayHandle);
	glActiveTexture(GL_TEXTURE3);

	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayHandle);
	auto textureArrayLocation = shader->GetUniformLocation(samplerHandle);
	glUniform1i(textureArrayLocation, 3);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 11, GL_RGBA8, 1024, 1024, (int)textureNames.size());
	GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
	for (unsigned int i = 0; i < textureNames.size(); ++i)
	{
		tygra::Image texture_image = tygra::createImageFromPngFile(textureNames[i]);

		//Specify i-essim image
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
			0,                     //Mipmap number
			0, 0, i,                 //xoffset, yoffset, zoffset
			1024, 1024, 1,                 //width, height, depth
			pixel_formats[texture_image.componentsPerPixel()],                //format
			texture_image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,      //type
			texture_image.pixelData());                //pointer to data
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
			std::cerr << err << std::endl;
	}
	shader->Unbind();
}

void MyView::SetFromExisteingTextureArray(GLuint& textureArrayHandle, Shader* shader, const char* samplerHandle)
{
	shader->Bind();
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayHandle);
	auto textureArrayLocation = shader->GetUniformLocation(samplerHandle);
	glUniform1i(textureArrayLocation, 3);
	shader->Unbind();
}

bool MyView::CheckError()
{
	GLuint err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cerr << err << std::endl;
		return true;
	}
	return false;
}

void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
    glViewport(0, 0, width, height);

	/*
	* Tutorial: This is where you'll recreate texture and renderbuffer objects
	*           and attach them to framebuffer objects.
	*/
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_material_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_depth_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	Shader* shaders[4] = { ambientLightShader, directionalLightShader, spotlightShader, pointLightShader };
	for(int i = 0; i < 4; ++i)
	{
		shaders[i]->Bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
		glUniform1i(glGetUniformLocation(shaders[i]->GetShaderID(), "sampler_world_position"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
		glUniform1i(glGetUniformLocation(shaders[i]->GetShaderID(), "sampler_world_normal"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_material_tex_);
		glUniform1i(glGetUniformLocation(shaders[i]->GetShaderID(), "sampler_world_material"), 2);
		shaders[i]->Unbind();
	}

	// --------------------------

	GLenum framebuffer_status = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer_fbo_);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_RECTANGLE, gbuffer_depth_tex_, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, gbuffer_position_tex_, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, gbuffer_material_tex_, 0);
	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };


	glDrawBuffers(3, attachments);

	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "gbuffer framebuffer not complete");
	}
	 
	framebuffer_status = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);
	glBindRenderbuffer(GL_RENDERBUFFER, lbuffer_colour_rbo_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, width, height);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_RECTANGLE, gbuffer_depth_tex_, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, lbuffer_colour_rbo_);
	GLuint kattachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, kattachments);

	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "lbuffer framebuffer not complete");
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	CheckError();

#pragma region ShadowMaps

	//SetPlayer : bob ToPos : (3, 4, 5) When : time > 9;
	/*glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/


	glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);

	for (int i = 0; i < 4; ++i)
	{
		shaders[i]->Bind(); 
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
		glUniform1i(glGetUniformLocation(shaders[i]->GetShaderID(), "shadowMap"), 4);
		shaders[i]->Unbind();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowmap_tex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	
	
	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "shadowmap framebuffer not complete");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#pragma endregion

#pragma region SMAA

	return;

	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &albedo_tex);
	glBindTexture(GL_TEXTURE_2D, albedo_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	

	glGenTextures(1, &edge_tex);
	glBindTexture(GL_TEXTURE_2D, edge_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	

	glGenTextures(1, &blend_tex);
	glBindTexture(GL_TEXTURE_2D, blend_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	

	CheckError();

	unsigned char* buffer = 0;
	FILE* f = 0;

	buffer = new unsigned char[1024 * 1024];
	f = fopen("smaa_area.raw", "rb"); //rb stands for "read binary file"

	if (!f)
	{
		std::cerr << "Couldn't open smaa_area.raw.\n";
		exit(1);
	}

	fread(buffer, AREATEX_WIDTH * AREATEX_HEIGHT * 2, 1, f);
	fclose(f);

	f = 0;

	glGenTextures(1, &area_tex);
	glBindTexture(GL_TEXTURE_2D, area_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, (GLsizei)AREATEX_WIDTH, (GLsizei)AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, buffer);


	f = fopen("smaa_search.raw", "rb");

	if (!f)
	{
		std::cerr << "Couldn't open smaa_search.raw.\n";
		exit(1);
	}

	fread(buffer, SEARCHTEX_WIDTH * SEARCHTEX_HEIGHT, 1, f);
	fclose(f);

	f = 0;

	glGenTextures(1, &search_tex);
	glBindTexture(GL_TEXTURE_2D, search_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, (GLsizei)AREATEX_WIDTH, (GLsizei)AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, buffer);


	GLenum modes[] = { GL_COLOR_ATTACHMENT0 };

	glGenFramebuffers(1, &albedo_fbo);
	glGenRenderbuffers(1, &albedo_rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, albedo_fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, albedo_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, width, height);
	glDrawBuffers(1, modes);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_tex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, albedo_rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "FBO not complete.\n";
		exit(1);
	}

	glGenFramebuffers(1, &edge_fbo);
	glGenRenderbuffers(1, &edge_rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, edge_fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, edge_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, width, height);
	glDrawBuffers(1, modes);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, edge_tex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, edge_rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "FBO not complete.\n";
		exit(1);
	}

	glGenFramebuffers(1, &blend_fbo);
	glGenRenderbuffers(1, &blend_rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, blend_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, width, height);
	glDrawBuffers(1, modes);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blend_tex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, blend_rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "FBO not complete.\n";
		exit(1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);


#pragma endregion 

}

void MyView::windowViewDidStop(tygra::Window * window)
{
	delete gbufferShadr;
	delete ambientLightShader;
	delete directionalLightShader;
	delete pointLightShader;
	delete spotlightShader;
	delete edge_shader;
	delete blend_shader;
	delete neighborhood_shader;

	GLuint vaos[4] = { vao , light_quad_mesh_.vao, light_sphere_mesh_.vao, light_cone_mesh_.vao };
	GLuint textures[9] = { gbuffer_position_tex_, gbuffer_normal_tex_, gbuffer_material_tex_, gbuffer_depth_tex_, albedo_tex, edge_tex, blend_tex, area_tex, search_tex }; 
	GLuint buffer[6] = { vertex_vbo, element_vbo, instance_vbo, material_vbo, commandBuffer, lightDataUBO};
	GLuint rbos[4] = {lbuffer_colour_rbo_, edge_rbo, blend_rbo, albedo_rbo};
	GLuint fbos[5]{lbuffer_fbo_, gbuffer_fbo_, edge_fbo, blend_fbo, albedo_fbo};
	glDeleteVertexArrays(4, vaos);
	glDeleteFramebuffers(5, fbos);
	glDeleteRenderbuffers(4, rbos);
	glDeleteTextures(9, textures);
	glDeleteBuffers(6, buffer);
}

void MyView::windowViewRender(tygra::Window * window)
{
    assert(scene_ != nullptr);

    glClearColor(0.29f, 0.f, 0.51f, 0.f);

	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];

	const glm::vec3 camera_position = (const glm::vec3&)scene_->getCamera().getPosition();
	const glm::vec3 camera_direction = (const glm::vec3&)scene_->getCamera().getDirection();
	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()), aspect_ratio, 1.f, 1000.f);
	glm::mat4 view_xform = glm::lookAt(camera_position, camera_position + camera_direction, glm::vec3(0, 1, 0));
	glm::mat4 projection_view = projection_xform * view_xform;

	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer_fbo_);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	glBindVertexArray(vao);
	
#pragma region Update data changed this frame
	int counter = 0;
	for (const auto &mesh : meshes_)
	{

		auto& instances = scene_->getInstancesByMeshId(mesh.first);

		for (const auto& instance : instances)
		{
			const auto& inst = scene_->getInstanceById(instance);
			glm::mat4 model_xform = glm::mat4((const glm::mat4x3&)inst.getTransformationMatrix());

			matrices[counter] = model_xform;
			counter++;
		}

	}
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * (matrices.size()), glm::value_ptr(matrices[0]));
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const auto& pointLights = scene_->getAllPointLights();
	for (int i = 0; i < pointLights.size(); ++i)
	{
		lightingData.pointLight[i].position = (const glm::vec3&)pointLights[i].getPosition();
		lightingData.pointLight[i].range = pointLights[i].getRange();
		lightingData.pointLight[i].intensity = (const glm::vec3&)pointLights[i].getIntensity();		
	}
	lightingData.ambientLight.ambient_light = (const glm::vec3&)scene_->getAmbientLightIntensity();
	
	const auto& directionalLights = scene_->getAllDirectionalLights();
	for (int i = 0; i < directionalLights.size(); ++i)
	{
		lightingData.directionalLight[i].direction = (const glm::vec3&)directionalLights[i].getDirection();
		lightingData.directionalLight[i].intensity = (const glm::vec3&)directionalLights[i].getIntensity();
	}

	auto& spotlightRef = scene_->getAllSpotLights();
	for (unsigned int i = 0; i < spotlightRef.size(); ++i)
	{
		lightingData.spotLight[i].position = (const glm::vec3&) spotlightRef[i].getPosition();
		lightingData.spotLight[i].direction = (const glm::vec3&) spotlightRef[i].getDirection();
	}
	
	lightingData.cameraPosition = (const glm::vec3&)scene_->getCamera().getPosition();
	lightingData.maxPointLights = (float)pointLights.size();
	lightingData.maxDirectionalLights = (float)directionalLights.size();
	lightingData.maxSpotlights = (float)spotlightRef.size();
	glBindBuffer(GL_UNIFORM_BUFFER, lightDataUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(DataBlock), &lightingData, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
#pragma endregion 

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);

#pragma region Draw call for rendering normal sponza
	gbufferShadr->Bind(); 
	glUniformMatrix4fv(glGetUniformLocation(gbufferShadr->GetShaderID(), "projection_view"), 1, GL_FALSE, glm::value_ptr(projection_view));

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshes_.size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

#pragma endregion 

	glDisable(GL_DEPTH_TEST);


	gbufferShadr->Unbind();

	//// --------------------------------------------------------- SHADOWS -----------------------------------------------------------

	//glDisable(GL_STENCIL_TEST);
	//glEnable(GL_DEPTH_TEST);
	//glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	//glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	//glClear(GL_DEPTH_BUFFER_BIT);

	//glm::mat4 lightProjection = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, -200.0f, 200.0f);
	//glm::mat4 lightPresPorojection =  glm::perspective(glm::radians(spotlightRef[0].getConeAngleDegrees()), (float)(SHADOW_WIDTH/ SHADOW_HEIGHT), 1.f, 1000.f);
	//glm::mat4 lightViewMat = glm::lookAt((const glm::vec3&)directionalLights[0].getDirection(), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	////glm::mat4 lightViewMat = glm::lookAt((const glm::vec3&)spotlightRef[0].getDirection(), (const glm::vec3&)spotlightRef[0].getPosition(), glm::vec3(0, 1, 0));
	//
	//glm::mat4 rotationMatrix = glm::lookAt((const glm::vec3&)spotlightRef[0].getPosition(), (const glm::vec3&)spotlightRef[0].getPosition() + (const glm::vec3&)spotlightRef[0].getDirection(), glm::vec3(0, 1, 0));
	//rotationMatrix = glm::inverse(rotationMatrix);
	//auto transDir = (const glm::vec3&)spotlightRef[0].getPosition();
	//transDir *= -1;
	//glm::mat4 translationMatrix = glm::mat4(1.0);
	//translationMatrix = glm::translate(translationMatrix, transDir);
	//glm::mat4 scaleMatrix = glm::mat4(1.0);
	//scaleMatrix = glm::scale(scaleMatrix, glm::vec3(spotlightRef[0].getRange()));

	//glm::mat4 model_matrix = glm::mat4(1.0);
	//model_matrix = translationMatrix * rotationMatrix * scaleMatrix;

	//glm::mat4 lightSpaceMatrix = lightPresPorojection * model_matrix;
	//
	//shadowDepth_Shader->Bind();

	//shadowDepth_Shader->SetUniformMatrix4FValue("lightSpaceMatrix", lightSpaceMatrix);

	//glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	//glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshes_.size(), 0);
	//glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	//shadowDepth_Shader->Unbind();
	//
	//glViewport(0, 0, viewport_size[2], viewport_size[3]);

	//glDisable(GL_DEPTH_TEST);
	//// --------------------------------------------------------- SHADOWS -----------------------------------------------------------
	// --------------------------------------------------------- LIGHTING -----------------------------------------------------------

	

	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);

	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0xFF);

	glBindBuffer(GL_UNIFORM_BUFFER, materialDataUBO);
	glBindVertexArray(light_quad_mesh_.vao);
	
	
	ambientLightShader->Bind();
	ambientLightShader->SetUniformIntValue("useTextures", useTextures);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	ambientLightShader->Unbind();
	

	

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	directionalLightShader->Bind();
	directionalLightShader->SetUniformIntValue("useTextures", useTextures);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	directionalLightShader->Unbind();
	

	glBindVertexArray(0);	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	

	glBindVertexArray(light_sphere_mesh_.vao);

	pointLightShader->Bind();
	pointLightShader->SetUniformMatrix4FValue("projection_view", projection_view);
	pointLightShader->SetUniformIntValue("useTextures", useTextures);
	for (int i = 0; i < pointLights.size(); ++i)
	{
		glm::mat4 model_matrix = glm::mat4(1);
		model_matrix = glm::translate(model_matrix, (const glm::vec3&)pointLights[i].getPosition());
		model_matrix = glm::scale(model_matrix, glm::vec3(pointLights[i].getRange()));

		pointLightShader->SetUniformMatrix4FValue("model_matrix", model_matrix);
		pointLightShader->SetUniformIntValue("currentPointLight", i);
		glDrawElements(GL_TRIANGLES, light_sphere_mesh_.element_count, GL_UNSIGNED_INT, nullptr);
	}

	
	pointLightShader->Unbind();
	glBindVertexArray(0);


	

	spotlightShader->Bind();
	spotlightShader->SetUniformMatrix4FValue("projection_view", projection_view);
	spotlightShader->SetUniformIntValue("useTextures", useTextures);

	for (int i = 0; i < spotlightRef.size(); ++i)
	{
		// --------------------------------------------------------- SHADOWS -----------------------------------------------------------

		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);

		glBindVertexArray(vao);

		//glm::mat4 lightProjection = glm::ortho(-200.0f, 200.0f, -200.0f, 200.0f, -200.0f, 200.0f);
		glm::mat4 lightPresPorojection = glm::perspective(glm::radians(spotlightRef[i].getConeAngleDegrees()), (float)(SHADOW_WIDTH / SHADOW_HEIGHT), 1.f, 1000.f);
		//glm::mat4 lightViewMat = glm::lookAt((const glm::vec3&)directionalLights[0].getDirection(), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 lightViewMat = glm::lookAt((const glm::vec3&)spotlightRef[i].getPosition(), (const glm::vec3&)spotlightRef[i].getPosition() + (const glm::vec3&)spotlightRef[i].getDirection(), glm::vec3(0, 1, 0));

		glm::mat4 rotationMatrix = glm::lookAt((const glm::vec3&)spotlightRef[i].getPosition(), (const glm::vec3&)spotlightRef[i].getPosition() + (const glm::vec3&)spotlightRef[i].getDirection(), glm::vec3(0, 1, 0));
		rotationMatrix = glm::inverse(rotationMatrix);
		auto transDir = (const glm::vec3&)spotlightRef[i].getPosition();
		transDir *= -1;
		glm::mat4 translationMatrix = glm::mat4(1.0);
		translationMatrix = glm::translate(translationMatrix, transDir);
		glm::mat4 scaleMatrix = glm::mat4(1.0);
		scaleMatrix = glm::scale(scaleMatrix, glm::vec3(spotlightRef[i].getRange()));

		glm::mat4 model_matrix = glm::mat4(1.0);
		model_matrix = translationMatrix * rotationMatrix * scaleMatrix;

		glm::mat4 lightSpaceMatrix = lightPresPorojection * model_matrix;

		shadowDepth_Shader->Bind();

		shadowDepth_Shader->SetUniformMatrix4FValue("lightSpaceMatrix", lightSpaceMatrix);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshes_.size(), 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glGenerateMipmap(GL_TEXTURE_2D);

		shadowDepth_Shader->Unbind();
		glBindVertexArray(0);
		glViewport(0, 0, viewport_size[2], viewport_size[3]);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST);
		glDepthMask(GL_FALSE);
		// --------------------------------------------------------- SHADOWS -----------------------------------------------------------

		glBindVertexArray(light_cone_mesh_.vao);	
		

		glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);
		spotlightShader->Bind();
		spotlightShader->SetUniformMatrix4FValue("model_matrix", model_matrix);
		spotlightShader->SetUniformIntValue("currentSpotLight", i);
		glDrawElements(GL_TRIANGLES, light_cone_mesh_.element_count, GL_UNSIGNED_INT, nullptr);
		spotlightShader->Unbind();
	}
	
	glBindVertexArray(0);
	
	 

	
	
	glCullFace(GL_BACK);

	
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);



	glBindFramebuffer(GL_READ_FRAMEBUFFER, lbuffer_fbo_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);

	return;

	if (enableSMAA)
	{


		// SMAA EDGE PASS
		glBindFramebuffer(GL_READ_FRAMEBUFFER, lbuffer_fbo_);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, edge_fbo);
		glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
		//glClearColor(0, 0, 0, 0);
		//glClear(GL_COLOR_BUFFER_BIT);

		edge_shader->Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedo_tex);

		glBindVertexArray(light_quad_mesh_.vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);

		edge_shader->Unbind();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// SMAA BLEND WEIGHT PASS

//		glBindFramebuffer(GL_FRAMEBUFFER, blend_fbo);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, edge_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blend_fbo);

		glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);

		//glClearColor(0, 0, 0, 0);
		//glClear(GL_COLOR_BUFFER_BIT);

		blend_shader->Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, edge_tex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, area_tex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, search_tex);

		glBindVertexArray(light_quad_mesh_.vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);

		blend_shader->Unbind();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// SMAA NEIGHBORHOOD BLEND PASS

		neighborhood_shader->Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, albedo_tex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, blend_tex);

		glEnable(GL_FRAMEBUFFER_SRGB);

		glBindVertexArray(light_quad_mesh_.vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);

		glDisable(GL_FRAMEBUFFER_SRGB);

		neighborhood_shader->Unbind();

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, albedo_tex);
		//glBindVertexArray(light_quad_mesh_.vao);
		//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		//glBindVertexArray(0);

	}




	glBindFramebuffer(GL_READ_FRAMEBUFFER, blend_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
