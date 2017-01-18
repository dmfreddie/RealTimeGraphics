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
		glm::mat4 matrix = glm::mat4(1.0);
		matrix = glm::translate(matrix, light.position);
		matrix = glm::scale(matrix, glm::vec3(light.range, light.range, light.range));
		pointLightMatricies.push_back(matrix);
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
		light.intensity /= 2;
		light.range = spotlightRef[i].getRange();
		light.coneAngle = glm::radians(spotlightRef[i].getConeAngleDegrees());
		light.castShadow = spotlightRef[i].getCastShadow();

		lightingData.spotLight[i] = light;

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
		spotlightMatricies.push_back(model_matrix);
	}
	lightingData.maxDirectionalLights = (float)directionalLightRef.size();


	lightingData.ambientLight.ambient_light = (const glm::vec3&)scene_->getAmbientLightIntensity();

	//LoadBlankTextureArray(spotlightRef.size(), spotlightShader, shadowmap_tex, "shadowMap");

	CheckError();
#pragma endregion 

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

		
#pragma endregion 

	/*
		*           This code creates a cone to use when deferred shading
		*           with a spot light source.
		*/
#pragma region Create cone mesh
		float coneAngle = 0.0f;
		float maxRange = 0.0f;
		for (int i = 0; i < spotlightRef.size(); ++i)
		{
			if (spotlightRef[i].getConeAngleDegrees() > coneAngle)
				coneAngle = spotlightRef[i].getConeAngleDegrees();
			if (spotlightRef[i].getRange() > maxRange)
				maxRange = spotlightRef[i].getRange();
		}
		float radius = sin(coneAngle) * 3;
		tsl::IndexedMeshPtr coneMesh = tsl::createConePtr(radius, 1.0f, 12);
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
	std::vector<PBRMaterial> pbrMaterials;
	std::vector<PBRMaterial> materials;
	

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

			PBRMaterial pbrMat;
			pbrMat.diffuseColour = (const glm::vec3&)material.getDiffuseColour();
			pbrMat.vertexShineyness = material.getShininess();
			pbrMat.specularColour = (const glm::vec3&)material.getSpecularColour();
			pbrMat.diffuseTextureID = materialIDCount;
			pbrMaterials.push_back(pbrMat);
		}

		pbrMaterialData.materials[materialIDCount] = pbrMaterials[counter];
		counter += (int)instances.size();
		materialIDCount++;

	}

	// Set the materials diffuse texture id to the corresponsing texture
	pbrMaterialData.materials[0].diffuseTextureID = 0;
	pbrMaterialData.materials[1].diffuseTextureID = 1;
	pbrMaterialData.materials[2].diffuseTextureID = 2;
	pbrMaterialData.materials[3].diffuseTextureID = 3;
	pbrMaterialData.materials[4].diffuseTextureID = 4;
	pbrMaterialData.materials[5].diffuseTextureID = 2;
	pbrMaterialData.materials[6].diffuseTextureID = 5;
	pbrMaterialData.materials[7].diffuseTextureID = 6;
	pbrMaterialData.materials[8].diffuseTextureID = 2;
	pbrMaterialData.materials[9].diffuseTextureID = 7;

	pbrMaterialData.materials[10].diffuseTextureID = 2;
	pbrMaterialData.materials[11].diffuseTextureID = 8;
	pbrMaterialData.materials[12].diffuseTextureID = 9;
	pbrMaterialData.materials[13].diffuseTextureID = 10;
	pbrMaterialData.materials[14].diffuseTextureID = 11;
	pbrMaterialData.materials[15].diffuseTextureID = 2;
	pbrMaterialData.materials[16].diffuseTextureID = 7;
	pbrMaterialData.materials[17].diffuseTextureID = 7;
	pbrMaterialData.materials[18].diffuseTextureID = 2;
	pbrMaterialData.materials[19].diffuseTextureID = 12;

	pbrMaterialData.materials[20].diffuseTextureID = 4;
	pbrMaterialData.materials[21].diffuseTextureID = 2;
	pbrMaterialData.materials[22].diffuseTextureID = 2;
	pbrMaterialData.materials[23].diffuseTextureID = 2;
	pbrMaterialData.materials[24].diffuseTextureID = 13;
	pbrMaterialData.materials[25].diffuseTextureID = 10;
	pbrMaterialData.materials[26].diffuseTextureID = 2;

	for(int i = 0; i < 30; ++i)
	{
		//pbrMaterialData.materials[i] = materialData.materials[i];
		pbrMaterialData.materials[i].roughness = 0.5f;
		pbrMaterialData.materials[i].ambientOcclusion = 0.5f;
		pbrMaterialData.materials[i].metallic = 0.5f;
	}
	
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
		pbrMaterials.size() * sizeof(PBRMaterial),
		pbrMaterials.data(),
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


	glGenBuffers(1, &pointLightMatrix_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, pointLightMatrix_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		pointLightMatricies.size() * sizeof(glm::mat4),
		pointLightMatricies.data(),
		GL_DYNAMIC_DRAW
		);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	CheckError();
	
	glGenBuffers(1, &spotLightMatrix_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, spotLightMatrix_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		spotlightMatricies.size() * sizeof(glm::mat4),
		spotlightMatricies.data(),
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
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(PBRMaterial), TGL_BUFFER_OFFSET_OF(PBRMaterial, diffuseTextureID));
	glVertexAttribDivisor(3, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(4 + i);
		glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(4 + i, 1);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glGenVertexArrays(1, &light_sphere_mesh_.vao);
	glBindVertexArray(light_sphere_mesh_.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_sphere_mesh_.element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_sphere_mesh_.vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		sizeof(glm::vec3), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, pointLightMatrix_vbo);
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(1 + i);
		glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(1 + i, 1);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glGenVertexArrays(1, &light_cone_mesh_.vao);
	glBindVertexArray(light_cone_mesh_.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light_cone_mesh_.element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, light_cone_mesh_.vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		sizeof(glm::vec3), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, spotLightMatrix_vbo);
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(1 + i);
		glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(1 + i, 1);
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


	glGenBuffers(1, &pbrMaterialHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, pbrMaterialHandle);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(PBRMaterialDataBlock), &pbrMaterialData, GL_STATIC_DRAW);
	CheckError();
	ambientLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, pbrMaterialHandle);
	glUniformBlockBinding(ambientLightShader->GetShaderID(), glGetUniformBlockIndex(ambientLightShader->GetShaderID(), "PBRMaterialDataBlock"), 1);
	ambientLightShader->Unbind();

	directionalLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, pbrMaterialHandle);
	glUniformBlockBinding(directionalLightShader->GetShaderID(), glGetUniformBlockIndex(directionalLightShader->GetShaderID(), "PBRMaterialDataBlock"), 1);
	directionalLightShader->Unbind();

	pointLightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, pbrMaterialHandle);
	glUniformBlockBinding(pointLightShader->GetShaderID(), glGetUniformBlockIndex(pointLightShader->GetShaderID(), "PBRMaterialDataBlock"), 1);
	pointLightShader->Unbind();

	spotlightShader->Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, pbrMaterialHandle);
	glUniformBlockBinding(spotlightShader->GetShaderID(), glGetUniformBlockIndex(spotlightShader->GetShaderID(), "PBRMaterialDataBlock"), 1);
	spotlightShader->Unbind();
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
#pragma endregion 

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

void MyView::LoadBlankTextureArray(int textureCount, Shader* shader, GLuint& textureArrayHandle, const char* samplerHandle)
{
	shader->Bind();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &textureArrayHandle);
	glActiveTexture(GL_TEXTURE3);

	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayHandle);
	auto textureArrayLocation = shader->GetUniformLocation(samplerHandle);
	glUniform1i(textureArrayLocation, 3);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 11, GL_RGBA8, SHADOW_WIDTH, SHADOW_HEIGHT, textureCount);
	GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
	for (unsigned int i = 0; i < textureCount; ++i)
	{
		//Specify i-essim image
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
			0,                     //Mipmap number
			0, 0, i,                 //xoffset, yoffset, zoffset
			SHADOW_WIDTH, SHADOW_HEIGHT, 1,                 //width, height, depth
			GL_DEPTH_COMPONENT,                //format
			GL_FLOAT,      //type
			nullptr);                //pointer to data
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
	CheckError();
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
	CheckError();
	
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
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, albedo_tex, 0);
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
	
	glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	for (int i = 0; i < 4; ++i)
	{
		shaders[i]->Bind(); 
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
		glUniform1i(glGetUniformLocation(shaders[i]->GetShaderID(), "shadowMap"), 4);
		shaders[i]->Unbind();
	}

	//glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
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


}

void MyView::windowViewDidStop(tygra::Window * window)
{
	

	GLuint vaos[4] = { vao , light_quad_mesh_.vao, light_sphere_mesh_.vao, light_cone_mesh_.vao };
	GLuint textures[9] = { gbuffer_position_tex_, gbuffer_normal_tex_, gbuffer_material_tex_, gbuffer_depth_tex_ }; 
	GLuint buffer[6] = { vertex_vbo, element_vbo, instance_vbo, material_vbo, commandBuffer, lightDataUBO};
	GLuint rbos[4] = {lbuffer_colour_rbo_};
	GLuint fbos[5]{lbuffer_fbo_, gbuffer_fbo_};
	glDeleteVertexArrays(4, vaos);
	glDeleteFramebuffers(5, fbos);
	glDeleteRenderbuffers(4, rbos);
	glDeleteTextures(9, textures);
	glDeleteBuffers(6, buffer);
}

void MyView::windowViewRender(tygra::Window * window)
{
	assert(scene_ != nullptr);

	// glClearColor(0.29f, 0.f, 0.51f, 0.f);
	glClearColor(0.f, 0.f, 0.f, 0.f);

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





#pragma region Update data changed this frame
	glBindVertexArray(vao);
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
	glBindVertexArray(0);

	glBindVertexArray(light_sphere_mesh_.vao);
	const auto& pointLights = scene_->getAllPointLights();
	for (int i = 0; i < pointLights.size(); ++i)
	{
		lightingData.pointLight[i].position = (const glm::vec3&)pointLights[i].getPosition();
		lightingData.pointLight[i].range = pointLights[i].getRange();
		lightingData.pointLight[i].intensity = (const glm::vec3&)pointLights[i].getIntensity();
		glm::mat4 matrix = glm::mat4(1.0);
		matrix = glm::translate(matrix, lightingData.pointLight[i].position);
		matrix = glm::scale(matrix, glm::vec3(lightingData.pointLight[i].range, lightingData.pointLight[i].range, lightingData.pointLight[i].range));
		pointLightMatricies[i] = matrix;
	}
	lightingData.ambientLight.ambient_light = (const glm::vec3&)scene_->getAmbientLightIntensity();
	glBindBuffer(GL_ARRAY_BUFFER, pointLightMatrix_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * (pointLightMatricies.size()), glm::value_ptr(pointLightMatricies[0]));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(light_sphere_mesh_.vao);

	const auto& directionalLights = scene_->getAllDirectionalLights();
	for (int i = directionalLights.size()-1; i < directionalLights.size(); ++i)
	{
		lightingData.directionalLight[i].direction = (const glm::vec3&)directionalLights[i].getDirection();
		lightingData.directionalLight[i].intensity = (const glm::vec3&)directionalLights[i].getIntensity();
	}

	auto& spotlightRef = scene_->getAllSpotLights();
	for (unsigned int i = 0; i < spotlightRef.size(); ++i)
	{
		lightingData.spotLight[i].position = (const glm::vec3&) spotlightRef[i].getPosition();
		lightingData.spotLight[i].direction = (const glm::vec3&) spotlightRef[i].getDirection();

		glm::mat4 rotationMatrix = glm::lookAt((const glm::vec3&)spotlightRef[i].getPosition(), (const glm::vec3&)spotlightRef[i].getPosition() + (const glm::vec3&)spotlightRef[i].getDirection(), glm::vec3(0, 1, 0));
		rotationMatrix = glm::inverse(rotationMatrix);
		
		glm::mat4 translationMatrix = glm::mat4(1.0);
		translationMatrix = glm::translate(translationMatrix, (const glm::vec3&)spotlightRef[i].getDirection() * lightingData.spotLight[i].range);
		glm::mat4 scaleMatrix = glm::mat4(1.0);
		scaleMatrix = glm::scale(scaleMatrix, glm::vec3(lightingData.spotLight[i].range));

		glm::mat4 model_matrix = translationMatrix * rotationMatrix * scaleMatrix;
		spotlightMatricies[i] = model_matrix;
	}
	glBindBuffer(GL_ARRAY_BUFFER, spotLightMatrix_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * (spotlightMatricies.size()), glm::value_ptr(spotlightMatricies[0]));
	glBindBuffer(GL_ARRAY_BUFFER, 0);

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
	glBindVertexArray(vao);
	gbufferShadr->Bind();
	glUniformMatrix4fv(glGetUniformLocation(gbufferShadr->GetShaderID(), "projection_view"), 1, GL_FALSE, glm::value_ptr(projection_view));

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshes_.size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

#pragma endregion 

	glDisable(GL_DEPTH_TEST);


	gbufferShadr->Unbind();

	//// --------------------------------------------------------- SHADOWS -----------------------------------------------------------
	/*glDisable(GL_STENCIL_TEST);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 lightPorjection = glm::perspective(glm::radians(spotlightRef[0].getConeAngleDegrees()), 1.0f, 0.01f, spotlightRef[0].getRange());
	glm::mat4 lightView = glm::lookAt((const glm::vec3&)spotlightRef[0].getPosition(), (const glm::vec3&)spotlightRef[0].getPosition() + (const glm::vec3&)spotlightRef[0].getDirection(), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 light_projection_view = lightPorjection * lightView;

	shadowDepth_Shader->Bind();

	shadowDepth_Shader->SetUniformMatrix4FValue("lightSpaceMatrix", light_projection_view);
	glBindVertexArray(vao);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshes_.size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);
	shadowDepth_Shader->Unbind();

	glViewport(0, 0, viewport_size[2], viewport_size[3]);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	*/
	//// --------------------------------------------------------- SHADOWS -----------------------------------------------------------
	// --------------------------------------------------------- LIGHTING -----------------------------------------------------------



	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);

	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0xFF);

	glBindBuffer(GL_UNIFORM_BUFFER, pbrMaterialHandle);
	glBindVertexArray(light_quad_mesh_.vao);


	ambientLightShader->Bind();
	glBindBuffer(GL_UNIFORM_BUFFER, pbrMaterialHandle);
	ambientLightShader->SetUniformIntValue("useTextures", useTextures);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	ambientLightShader->Unbind();


	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	directionalLightShader->Bind();
	glBindBuffer(GL_UNIFORM_BUFFER, pbrMaterialHandle);
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
	glBindBuffer(GL_UNIFORM_BUFFER, pbrMaterialHandle);

	glDrawElementsInstanced(GL_TRIANGLES, light_sphere_mesh_.element_count, GL_UNSIGNED_INT, nullptr, pointLightMatricies.size());

	pointLightShader->Unbind();
	glBindVertexArray(0);

	glm::mat4 light_projection_view;
	// ----------------------------------------- SPOTLIGHT SHADOWS -------------------------------------------------------
	for (int i = 0; i < lightingData.maxSpotlights; ++i)
	{
		if (spotlightRef[i].getCastShadow())
		{
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);



			glClear(GL_DEPTH_BUFFER_BIT);

			glm::mat4 lightPorjection = glm::perspective(glm::radians(spotlightRef[i].getConeAngleDegrees()), 1.0f, 0.01f, 1000.0f/*spotlightRef[i].getRange()*/);
			glm::mat4 lightView = glm::lookAt((const glm::vec3&)spotlightRef[i].getPosition(), (const glm::vec3&)spotlightRef[i].getPosition() + (const glm::vec3&)spotlightRef[i].getDirection(), glm::vec3(0.0f, 1.0f, 0.0f));
			light_projection_view = lightPorjection * lightView;

			shadowDepth_Shader->Bind();

			shadowDepth_Shader->SetUniformMatrix4FValue("lightSpaceMatrix", light_projection_view);

			glBindVertexArray(vao);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
			glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)meshes_.size(), 0);
			glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
			glBindVertexArray(0);
			shadowDepth_Shader->Unbind();




			glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);
			glViewport(0, 0, viewport_size[2], viewport_size[3]);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_STENCIL_TEST);
			glEnable(GL_CULL_FACE);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
		}
		// ----------------------------------------- SPOTLIGHT SHADOWS -------------------------------------------------------

		glBindVertexArray(light_cone_mesh_.vao);
		
		spotlightShader->Bind();
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadowmap_tex);
		glUniform1i(glGetUniformLocation(spotlightShader->GetShaderID(), "shadowMap"), 4);
		spotlightShader->SetUniformMatrix4FValue("projection_view", projection_view);
		spotlightShader->SetUniformMatrix4FValue("model_matrix_Uniform", spotlightMatricies[i]);
		spotlightShader->SetUniformMatrix4FValue("lightSpaceMatrixUniform", light_projection_view);
		spotlightShader->SetUniformIntValue("useTextures", useTextures);
		spotlightShader->SetUniformIntValue("currentSL", i);
		glBindBuffer(GL_UNIFORM_BUFFER, pbrMaterialHandle);
		glDrawElements(GL_TRIANGLES, light_cone_mesh_.element_count, GL_UNSIGNED_INT, nullptr);
		//glDrawElementsInstanced(GL_TRIANGLES, light_cone_mesh_.element_count, GL_UNSIGNED_INT, nullptr, spotlightMatricies.size());

		spotlightShader->Unbind();
		glBindVertexArray(0);

	}

	
	
	glCullFace(GL_BACK);

	
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);



	glBindFramebuffer(GL_READ_FRAMEBUFFER, lbuffer_fbo_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);



}
