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
	gbufferShadr->Bind();
	gbufferShadr->GetUniformLocation("projection_view");
	gbufferShadr->Unbind();
	ambientLightShader->Bind();
	ambientLightShader->GetUniformLocation("projection_view");
	ambientLightShader->Unbind();
	//ambientLightShader.GetUniformLocation("useTextures");
	//pointLightShader.GetUniformLocation("projection_view");
	//pointLightShader.GetUniformLocation("useTextures");
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

		newMesh.first_vertex_index = vertices_.size();
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

		newMesh.first_element_index = elements.size();
		elements.insert(elements.end(), elementsArr.begin(), elementsArr.end());
		newMesh.element_count = elementsArr.size();

	}

	GLuint err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

	int materialIDCount = 0;
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
			mat.specularColour = (const glm::vec3&)material.getSpecularColour();
			mat.vertexShineyness = material.getShininess();
			mat.diffuseTextureID = materialIDCount;
			materials.push_back(mat);
		}
		materialIDCount++;

	}

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices_.size() * sizeof(Vertex), // size of data in bytes
		vertices_.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

	glGenBuffers(1, &element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, element_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		elements.size() * sizeof(unsigned int), // size of data in bytes
		elements.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

	glGenBuffers(1, &material_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, material_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		materials.size() * sizeof(Material),
		materials.data(),
		GL_STATIC_DRAW
		);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

	glGenBuffers(1, &instance_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		matrices.size() * sizeof(glm::mat4),
		matrices.data(),
		GL_DYNAMIC_DRAW
		);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

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

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;
#pragma endregion 

#pragma region Command Data
	int commandInt = 0;
	int counter = 0;
	int baseInstance = 0;
	for (const auto &mesh : meshes_)
	{
		const auto& mesh_ = mesh.second;
		auto& instances = scene_->getInstancesByMeshId(mesh.first);
		for (const auto& instance : instances)
			counter++;

		commands[commandInt].vertexCount = mesh_.element_count;
		commands[commandInt].instanceCount = instances.size(); // Just testing with 1 instance, ATM.
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
	glGenRenderbuffers(1, &gbuffer_colour_rbo_);

	glGenFramebuffers(1, &lbuffer_fbo_);
	glGenRenderbuffers(1, &lbuffer_colour_rbo_);
#pragma endregion 

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
	lightingData.maxPointLights = pointLightsRef.size();


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
	lightingData.maxDirectionalLights = directionalLightRef.size();

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
	lightingData.maxDirectionalLights = directionalLightRef.size();

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;


	lightingData.ambientLight.ambient_light = (const glm::vec3&)scene_->getAmbientLightIntensity();

	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;
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
#pragma endregion 
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

	ambientLightShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(ambientLightShader->GetShaderID(), "sampler_world_position"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(ambientLightShader->GetShaderID(), "sampler_world_normal"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_material_tex_);
	glUniform1i(glGetUniformLocation(ambientLightShader->GetShaderID(), "sampler_world_material"), 2);
	ambientLightShader->Unbind();
	directionalLightShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(directionalLightShader->GetShaderID(), "sampler_world_position"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(directionalLightShader->GetShaderID(), "sampler_world_normal"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_material_tex_);
	glUniform1i(glGetUniformLocation(directionalLightShader->GetShaderID(), "sampler_world_material"), 2);
	directionalLightShader->Unbind();
	pointLightShader->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(pointLightShader->GetShaderID(), "sampler_world_position"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(pointLightShader->GetShaderID(), "sampler_world_normal"), 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_material_tex_);
	glUniform1i(glGetUniformLocation(pointLightShader->GetShaderID(), "sampler_world_material"), 2);
	pointLightShader->Unbind();
	// --------------------------

	GLenum framebuffer_status = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer_fbo_);
	//glBindRenderbuffer(GL_RENDERBUFFER, lbuffer_colour_rbo_);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_RECTANGLE, gbuffer_depth_tex_, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE, gbuffer_position_tex_, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_RECTANGLE, gbuffer_material_tex_, 0);
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, lbuffer_colour_rbo_);
	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };


	glDrawBuffers(3, attachments);

	framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
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
		tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyView::windowViewDidStop(tygra::Window * window)
{
	delete gbufferShadr;
	delete ambientLightShader;
	delete directionalLightShader;
	delete pointLightShader;
	delete spotlightShader;

	GLuint vaos[4] = { vao , light_quad_mesh_.vao, light_sphere_mesh_.vao, light_cone_mesh_.vao };
	GLuint textures[4] = { gbuffer_position_tex_, gbuffer_normal_tex_, gbuffer_material_tex_, gbuffer_depth_tex_ }; 
	GLuint buffer[10] = { vertex_vbo, element_vbo, instance_vbo, material_vbo, commandBuffer, lightDataUBO, lbuffer_colour_rbo_, lbuffer_fbo_, gbuffer_colour_rbo_, gbuffer_fbo_ };

	glDeleteVertexArrays(4, vaos);
	glDeleteFramebuffers(1, &lbuffer_fbo_);
	glDeleteRenderbuffers(1, &lbuffer_colour_rbo_);
	glDeleteTextures(4, textures);
	glDeleteBuffers(10, buffer);
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
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
	lightingData.maxPointLights = pointLights.size();
	lightingData.maxDirectionalLights = directionalLights.size();
	lightingData.maxSpotlights = spotlightRef.size();
	glBindBuffer(GL_UNIFORM_BUFFER, lightDataUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(DataBlock), &lightingData, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
#pragma endregion 

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
#pragma region Draw call for rendering normal sponza
	gbufferShadr->Bind(); 
	glUniformMatrix4fv(glGetUniformLocation(gbufferShadr->GetShaderID(), "projection_view"), 1, GL_FALSE, glm::value_ptr(projection_view));

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, meshes_.size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

#pragma endregion 

	

	glDisable(GL_DEPTH_TEST);
	gbufferShadr->Unbind();

	// --------------------------------------------------------- LIGHTING -----------------------------------------------------------


	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);

	glBindVertexArray(light_quad_mesh_.vao);
	
	
	ambientLightShader->Bind();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	ambientLightShader->Unbind();
	

	

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	directionalLightShader->Bind();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	directionalLightShader->Unbind();
	

	glBindVertexArray(0);	
	
	

	glBindVertexArray(light_sphere_mesh_.vao);

	pointLightShader->Bind();
	pointLightShader->SetUniformMatrix4FValue("projection_view", projection_view);

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


	glBindVertexArray(light_cone_mesh_.vao);

	spotlightShader->Bind();
	spotlightShader->SetUniformMatrix4FValue("projection_view", projection_view);

	for (int i = 0; i < spotlightRef.size(); ++i)
	{
		glm::mat4 model_matrix = glm::lookAt((const glm::vec3&)spotlightRef[i].getPosition(), (const glm::vec3&)spotlightRef[i].getPosition() + (const glm::vec3&)spotlightRef[i].getDirection(), glm::vec3(0, 1, 0));
		model_matrix = glm::inverse(model_matrix);
		model_matrix = glm::translate(model_matrix, (const glm::vec3&)spotlightRef[i].getPosition());
		model_matrix = glm::scale(model_matrix, glm::vec3(spotlightRef[i].getRange()));
		
		
		
		spotlightShader->SetUniformMatrix4FValue("model_matrix", model_matrix);
		spotlightShader->SetUniformIntValue("currentSpotLight", i);
		glDrawElements(GL_TRIANGLES, light_cone_mesh_.element_count, GL_UNSIGNED_INT, nullptr);
	}
	 

	spotlightShader->Unbind();
	glBindVertexArray(0);
	

	
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, lbuffer_fbo_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
