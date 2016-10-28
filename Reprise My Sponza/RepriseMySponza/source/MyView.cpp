#include "MyView.hpp"

#include <scene/scene.hpp>
#include <tygra/FileHelper.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cassert>

MyView::MyView() : shaderProgram(0), skybox_shaderProgram(0)
{
}

MyView::~MyView() {
}

void MyView::setScene(const scene::Context * scene)
{
    scene_ = scene;
}

void MyView::UseTextures(const bool useTextures_)
{
	useTextures = useTextures_;
}

const bool MyView::UseTextures() const
{
	return useTextures;
}

void MyView::
CompileShader(std::string shaderFileName, GLenum shaderType, GLuint& shaderVariable)
{
	GLint compile_status = 0;
	shaderVariable = glCreateShader(shaderType);
	std::string shader_string = tygra::createStringFromFile(shaderFileName);
	const char *shader_code = shader_string.c_str();
	glShaderSource(shaderVariable, 1, (const GLchar **)&shader_code, NULL);
	glCompileShader(shaderVariable);
	glGetShaderiv(shaderVariable, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetShaderInfoLog(shaderVariable, string_length, NULL, log);
		std::cerr << log << std::endl;
	}
}

bool MyView::
CheckLinkStatus(GLuint shaderProgram)
{
	GLint link_status = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		const int string_length = 1024;
		GLchar log[string_length] = "";
		glGetProgramInfoLog(shaderProgram, string_length, NULL, log);
		std::cerr << log << std::endl;
		return false;
	}
	return true;
}

void MyView::CompileShaders()
{
	//NORMAL

	GLuint vertex_shader;
	GLuint fragment_shader;
	CompileShader("resource:///Multipass_amb_vs.glsl", GL_VERTEX_SHADER, vertex_shader);
	CompileShader("resource:///Multipass_amb_fs.glsl", GL_FRAGMENT_SHADER, fragment_shader);


	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertex_shader);
	glBindAttribLocation(shaderProgram, 0, "vertex_position");
	glBindAttribLocation(shaderProgram, 1, "vertex_normal");
	glBindAttribLocation(shaderProgram, 2, "vertex_texcoord");

	glDeleteShader(vertex_shader);
	glAttachShader(shaderProgram, fragment_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(shaderProgram);


	// POINT LIGHT
	CompileShader("resource:///Multipass_pl_vs.glsl", GL_VERTEX_SHADER, vertex_shader);
	CompileShader("resource:///Multipass_pl_fs.glsl", GL_FRAGMENT_SHADER, fragment_shader);


	pointLightShaderProgram = glCreateProgram();
	glAttachShader(pointLightShaderProgram, vertex_shader);
	glBindAttribLocation(pointLightShaderProgram, 0, "vertex_position");
	glBindAttribLocation(pointLightShaderProgram, 1, "vertex_normal");
	glBindAttribLocation(pointLightShaderProgram, 2, "vertex_texcoord");

	glDeleteShader(vertex_shader);
	glAttachShader(pointLightShaderProgram, fragment_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(pointLightShaderProgram);

	// SPOT LIGHT
	CompileShader("resource:///Multipass_sl_vs.glsl", GL_VERTEX_SHADER, vertex_shader);
	CompileShader("resource:///Multipass_sl_fs.glsl", GL_FRAGMENT_SHADER, fragment_shader);


	spotLightShaderProgram = glCreateProgram();
	glAttachShader(spotLightShaderProgram, vertex_shader);
	glBindAttribLocation(spotLightShaderProgram, 0, "vertex_position");
	glBindAttribLocation(spotLightShaderProgram, 1, "vertex_normal");
	glBindAttribLocation(spotLightShaderProgram, 2, "vertex_texcoord");

	glDeleteShader(vertex_shader);
	glAttachShader(spotLightShaderProgram, fragment_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(spotLightShaderProgram);

	// DIRECTIONAL LIGHT
	CompileShader("resource:///Multipass_dl_vs.glsl", GL_VERTEX_SHADER, vertex_shader);
	CompileShader("resource:///Multipass_dl_fs.glsl", GL_FRAGMENT_SHADER, fragment_shader);


	directionalLightShaderProgram = glCreateProgram();
	glAttachShader(directionalLightShaderProgram, vertex_shader);
	glBindAttribLocation(directionalLightShaderProgram, 0, "vertex_position");
	glBindAttribLocation(directionalLightShaderProgram, 1, "vertex_normal");
	glBindAttribLocation(directionalLightShaderProgram, 2, "vertex_texcoord");

	glDeleteShader(vertex_shader);
	glAttachShader(directionalLightShaderProgram, fragment_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(directionalLightShaderProgram);

	// SKYBOX

	GLuint skybox_vertex_shader;
	GLuint skybox_fragment_shader;
	CompileShader("resource:///SkyboxVertexShader.glsl", GL_VERTEX_SHADER, skybox_vertex_shader);
	CompileShader("resource:///SkyboxFragmentShader.glsl", GL_FRAGMENT_SHADER, skybox_fragment_shader);


	skybox_shaderProgram = glCreateProgram();
	glAttachShader(skybox_shaderProgram, skybox_vertex_shader);
	glBindAttribLocation(skybox_shaderProgram, 0, "vertex_position");

	glDeleteShader(skybox_vertex_shader);
	glAttachShader(skybox_shaderProgram, skybox_fragment_shader);
	glDeleteShader(skybox_fragment_shader);
	glLinkProgram(skybox_shaderProgram);

	
	if (CheckLinkStatus(shaderProgram) && CheckLinkStatus(skybox_shaderProgram))
		std::cout << "Shaders Compiled!" << std::endl;

	glUseProgram(shaderProgram);
	Getuniforms();
}

void MyView::Getuniforms()
{
#pragma region Get the uniform locations
	/*
	Get the uniform locations of the uniform variables in the shader for each program where the varibale needs to be placed and bind it to a GLuint
	inside the unordered map. An unordered map was used because every time a new unifrom Gluint is added it doesn't need to re-order the map. This is
	also performed on start because the locations of the uniforms wont need to be changed per frame so it will speed up the rendering function as it
	doesn't need to perform unnessacary computation.
	*/
	uniforms["projection_view"] = glGetUniformLocation(shaderProgram, "projection_view");
	uniforms["useTextures"] = glGetUniformLocation(shaderProgram, "useTextures");
	glUseProgram(pointLightShaderProgram);
	uniforms["projection_view_pl"] = glGetUniformLocation(pointLightShaderProgram, "projection_view");
	uniforms["useTextures_pl"] = glGetUniformLocation(pointLightShaderProgram, "useTextures");
	glUseProgram(spotLightShaderProgram);
	uniforms["projection_view_sl"] = glGetUniformLocation(spotLightShaderProgram, "projection_view");
	uniforms["useTextures_sl"] = glGetUniformLocation(spotLightShaderProgram, "useTextures");
	glUseProgram(directionalLightShaderProgram);
	uniforms["projection_view_dl"] = glGetUniformLocation(directionalLightShaderProgram, "projection_view");
#pragma endregion 
}

void MyView::
LoadTexture(std::string textureName)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLuint texture = GLuint(0);
	tygra::Image texture_image = tygra::createImageFromPngFile(textureName);
	if (texture_image.doesContainData()) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA,
			texture_image.width(),
			texture_image.height(),
			0,
			pixel_formats[texture_image.componentsPerPixel()],
			texture_image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE
			: GL_UNSIGNED_SHORT,
			texture_image.pixelData());
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		textures[textureName] = texture;
	}
}

void MyView::LoadTextureArray(std::vector<std::string>& textureNames, GLuint& shaderHandle, GLuint& textureArrayHandle, const char* samplerHandle)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &textureArrayHandle);
	glActiveTexture(GL_TEXTURE0 + textureArrayHandle - 1);
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayHandle);
	auto textureArrayLocation = glGetUniformLocation(shaderHandle, samplerHandle);
	glUniform1i(textureArrayLocation, 0);

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
	
}

void MyView::ResetConsole()
{
	system("cls");
	//Application instructions
	std::cout << "ABOUT" << std::endl;
	std::cout << "Reprise My Sponza - Real Time Graphics ICA 1" << std::endl;
	std::cout << "P4011584 - Frederic Babord 2016 - 2017" << std::endl << std::endl;
	std::cout << "Submission date: 28th October 2016" << std::endl << std::endl;
	std::cout << "INSTRUCTIONS" << std::endl;
	std::cout << "Press F1 to toggle material textures." << std::endl;
	std::cout << "Press F2 to enable camera animation." << std::endl;
	std::cout << "Press F5 to recompile the shader." << std::endl << std::endl;
	std::cout << "Press Esc to Close." << std::endl << std::endl;

}

void MyView::windowViewWillStart(tygra::Window * window)
{
    assert(scene_ != nullptr);

	ResetConsole();
	CompileShaders();
	glUseProgram(shaderProgram);

#pragma region Textures and Lights

	// Load the textures into an array

	std::vector<std::string> diffuseTextureNames;
	diffuseTextureNames.push_back("content:///vase_dif.png");
	diffuseTextureNames.push_back("content:///Hook.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///lion.png");
	diffuseTextureNames.push_back("content:///vase_round.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///background.png");
	diffuseTextureNames.push_back("content:///flagPole.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///spnza_bricks_a_diff.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///sponza_floor_a_diff.png");
	diffuseTextureNames.push_back("content:///sponza_fabric_green_diff.png");
	diffuseTextureNames.push_back("content:///sponza_roof_diff.png");
	diffuseTextureNames.push_back("content:///sponza_flagpole_diff.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///spnza_bricks_a_diff.png");
	diffuseTextureNames.push_back("content:///spnza_bricks_a_diff.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///chain_texture.png");
	diffuseTextureNames.push_back("content:///vase_round.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");
	diffuseTextureNames.push_back("content:///sponza_curtain_diff.png");
	diffuseTextureNames.push_back("content:///sponza_roof_diff.png");
	diffuseTextureNames.push_back("content:///sponza_thorn_diff.png");

	

	LoadTextureArray(diffuseTextureNames, shaderProgram, diffuse_texture_array_handle, "textureArray");
	LoadTextureArray(diffuseTextureNames, spotLightShaderProgram, sl_diffuse_texture_array_handle, "textureArray");
	LoadTextureArray(diffuseTextureNames, pointLightShaderProgram, pl_diffuse_texture_array_handle, "textureArray");


	// SKYBOX

	skybox = new Skybox(
		"content:///Skybox/SunSetRight2048.png",
		"content:///Skybox/SunSetLeft2048.png",
		"content:///Skybox/SunSetFront2048.png",
		"content:///Skybox/SunSetBack2048.png",
		"content:///Skybox/SunSetUp2048.png",
		"content:///Skybox/SunSetDown2048.png",
		skybox_shaderProgram,
		"skybox");
	



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
		//dataBlock.pointLights[i] = light;
		pointLightBlock.pointLights[i] = light;
	}
	//dataBlock.maxPointLights = pointLightsRef.size();
	pointLightBlock.maxPointLights = pointLightsRef.size();

	auto& directionalLightRef = scene_->getAllDirectionalLights();
	for (unsigned int i = 0; i < directionalLightRef.size(); ++i)
	{
		DirectionalLight light;
		light.direction = (const glm::vec3&) directionalLightRef[i].getDirection();
		light.intensity = (const glm::vec3&) directionalLightRef[i].getIntensity();
		light.padding1 = 0.0f;
		light.padding2 = 0.0f;
		//dataBlock.directionalLights[i] = light;
		directionalLightDataBlock.directionalLights[i] = light;
	}
	//dataBlock.maxDirectionalLights = directionalLightRef.size();
	directionalLightDataBlock.maxDirectional = directionalLightRef.size();

	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

	auto& spotLightRef = scene_->getAllSpotLights();
	for (unsigned int i = 0; i < spotLightRef.size(); ++i)
	{
		SpotLight light;
		light.direction = (const glm::vec3&) spotLightRef[i].getDirection();
		light.intensity = (const glm::vec3&) spotLightRef[i].getIntensity();
		light.position = (const glm::vec3&) spotLightRef[i].getPosition();
		light.coneAngle = spotLightRef[i].getConeAngleDegrees();
		light.range = spotLightRef[i].getRange();
		light.castShadow = spotLightRef[i].getCastShadow();
		//dataBlock.spotLights[i] = light;
		spotLightDataBlock.spotLights[i] = light;
	}
	//dataBlock.maxSpotlights = spotLightRef.size();
	spotLightDataBlock.maxSpotLights = spotLightRef.size();

	ambientLightBlock.ambient_light = (const glm::vec3&)scene_->getAmbientLightIntensity();
	
	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;
#pragma endregion

#pragma region Load the mesh into buffers
	scene::GeometryBuilder builder;
	std::vector<Vertex> vertices;
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

		newMesh.first_vertex_index = vertices.size();
		vertices.reserve(vertices.size() + positions.size());

		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			Vertex vertex;
			vertex.position = (const glm::vec3&)positions[i];
			vertex.normal = (const glm::vec3&)normals[i];
			if(hasTexCood)
				vertex.texcoord = (const glm::vec2&)text_coord[i];
			vertices.push_back(vertex);
		}

		newMesh.first_element_index = elements.size();
		elements.insert(elements.end(), elementsArr.begin(), elementsArr.end());
		newMesh.element_count = elementsArr.size();

	}

	err = glGetError();
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
		vertices.size() * sizeof(Vertex), // size of data in bytes
		vertices.data(), // pointer to the data
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

#pragma region UBO 
	glGenBuffers(1, &ambientLightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, ambientLightUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(DataBlock), &ambientLightBlock, GL_STREAM_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ambientLightUBO);
	glUniformBlockBinding(shaderProgram, glGetUniformBlockIndex(shaderProgram, "DataBlock"), 0);





	glGenBuffers(1, &pointLightBlockUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, pointLightBlockUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(PointLightDataBlock), &pointLightBlock, GL_STREAM_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, pointLightBlockUBO);
	glUniformBlockBinding(pointLightShaderProgram, glGetUniformBlockIndex(pointLightShaderProgram, "DataBlock"), 1);


	glGenBuffers(1, &spotLightBlockUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, spotLightBlockUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SpotLightDataBlock), &spotLightDataBlock, GL_STREAM_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 2, spotLightBlockUBO);
	glUniformBlockBinding(spotLightShaderProgram, glGetUniformBlockIndex(spotLightShaderProgram, "DataBlock"), 2);


	glGenBuffers(1, &directionalLightBlockUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, directionalLightBlockUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(DirectionalLightDataBlock), &directionalLightDataBlock, GL_STREAM_DRAW);

	glBindBufferBase(GL_UNIFORM_BUFFER, 3, directionalLightBlockUBO);
	glUniformBlockBinding(directionalLightShaderProgram, glGetUniformBlockIndex(directionalLightShaderProgram, "DataBlock"), 3);

#pragma endregion 

	glBindVertexArray(0);
	
	err = glGetError();
	if (err != GL_NO_ERROR)
		std::cerr << err << std::endl;

}

void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
    glViewport(0, 0, width, height);
}

void MyView::windowViewDidStop(tygra::Window * window)
{
	glDeleteProgram(shaderProgram);
	glDeleteProgram(skybox_shaderProgram);
	glDeleteProgram(pointLightShaderProgram);
	glDeleteProgram(spotLightShaderProgram);
	glDeleteProgram(directionalLightShaderProgram);
	delete skybox;
	glDeleteBuffers(1, &vertex_vbo);
	glDeleteBuffers(1, &element_vbo);
	glDeleteBuffers(1, &instance_vbo);
	glDeleteBuffers(1, &material_vbo);
	glDeleteBuffers(1, &ambientLightUBO);
	glDeleteBuffers(1, &pointLightBlockUBO);
	glDeleteBuffers(1, &spotLightBlockUBO);
	glDeleteBuffers(1, &directionalLightBlockUBO);
	glDeleteBuffers(1, &commandBuffer);
	glDeleteVertexArrays(1, &vao);
}

void MyView::windowViewRender(tygra::Window * window)
{
    assert(scene_ != nullptr);
	
	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];

	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.556f, 0.822f, 1.0f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()), aspect_ratio, scene_->getCamera().getNearPlaneDistance(), scene_->getCamera().getFarPlaneDistance());
	glm::mat4 view_xform = glm::lookAt((const glm::vec3&)scene_->getCamera().getPosition(), (const glm::vec3&)scene_->getCamera().getPosition() + (const glm::vec3&)scene_->getCamera().getDirection(), (const glm::vec3&)scene_->getUpDirection());
	glm::mat4 projection_view = projection_xform * view_xform;

#pragma region Draw call for rendering the skybox


	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glUseProgram(skybox_shaderProgram);
	glm::mat4 mvp = glm::mat4(glm::mat3(projection_view));
	glUniformMatrix4fv(glGetUniformLocation(skybox_shaderProgram, "MVP"), 1, GL_FALSE, &mvp[0][0]);

	glm::mat4 m = glm::mat4(1.0);
	glUniformMatrix4fv(glGetUniformLocation(skybox_shaderProgram, "model"), 1, GL_FALSE, &m[0][0]);
	glm::mat4 v = view_xform;
	glUniformMatrix4fv(glGetUniformLocation(skybox_shaderProgram, "view"), 1, GL_FALSE, &v[0][0]);
	glm::mat4 p = projection_xform;
	glUniformMatrix4fv(glGetUniformLocation(skybox_shaderProgram, "projection"), 1, GL_FALSE, &p[0][0]);
	skybox->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->GetTextureID());
	glDrawArrays(GL_TRIANGLES, 0, 36);
	skybox->Unbind();
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
#pragma endregion 

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	

	//Use the initial shader program to render sponza normally
	
	glBindVertexArray(vao);

	glUseProgram(shaderProgram);

#pragma region Update sponzas lighting for this frame

	/*
	Get the light data from the scene and pass out the data into a uniform array of light structs in the vertex shader
	for each point light and the ambient light for the scene. The shader also needs to know how many lights are currently
	active and so that value is also passed out into the shader.
	*/


	glUniform1i(uniforms["useTextures"], useTextures);


	auto& spotLightRef = scene_->getAllSpotLights();
	for (unsigned int i = 0; i < spotLightRef.size(); ++i)
	{
		spotLightDataBlock.spotLights[i].direction = glm::normalize((const glm::vec3&) spotLightRef[i].getDirection());
		spotLightDataBlock.spotLights[i].intensity = (const glm::vec3&) spotLightRef[i].getIntensity();
		spotLightDataBlock.spotLights[i].position = (const glm::vec3&) spotLightRef[i].getPosition();
		spotLightDataBlock.spotLights[i].coneAngle = spotLightRef[i].getConeAngleDegrees();
		spotLightDataBlock.spotLights[i].range = spotLightRef[i].getRange();
		spotLightDataBlock.spotLights[i].castShadow = spotLightRef[i].getCastShadow();
	}

	for (unsigned int i = 0; i < scene_->getAllPointLights().size(); ++i)
	{
		pointLightBlock.pointLights[i].position = (const glm::vec3&)scene_->getAllPointLights()[i].getPosition();
		pointLightBlock.pointLights[i].range = scene_->getAllPointLights()[i].getRange();
		pointLightBlock.pointLights[i].intensity = (const glm::vec3&)scene_->getAllPointLights()[i].getIntensity();
	}
	pointLightBlock.cameraPosition = (const glm::vec3&)scene_->getCamera().getPosition();

	

	

	glBindBuffer(GL_UNIFORM_BUFFER, ambientLightUBO);
	ambientLightBlock.cameraPosition = (const glm::vec3&)scene_->getCamera().getPosition();
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(AmbientLightBlock), &ambientLightBlock);
#pragma endregion 

#pragma region Update data changed this frame


	
	glUniformMatrix4fv(uniforms["projection_view"], 1, GL_FALSE, glm::value_ptr(projection_view));
	
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
		glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * (instances.size()), glm::value_ptr(matrices[0]));
	}
#pragma endregion 

#pragma region Draw call for rendering normal sponza

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, meshes_.size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

#pragma endregion 

	glDepthMask(GL_FALSE);
	glDepthFunc(GL_EQUAL);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	

	glDisable(GL_BLEND);
#pragma region Draw call for rendering normal sponza
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, meshes_.size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
#pragma endregion



	
#pragma region Multipass rednering

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glDepthFunc(GL_EQUAL);
	glDepthMask(GL_FALSE);


		glUseProgram(directionalLightShaderProgram);
		glUniformMatrix4fv(uniforms["projection_view_dl"], 1, GL_FALSE, glm::value_ptr(projection_view));
		glBindBuffer(GL_UNIFORM_BUFFER, directionalLightBlockUBO);
		directionalLightDataBlock.cameraPosition = (const glm::vec3&)scene_->getCamera().getPosition();
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(DirectionalLightDataBlock), &directionalLightDataBlock);
	#pragma region Draw call for rendering normal sponza
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, meshes_.size(), 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	#pragma endregion


		glUseProgram(pointLightShaderProgram);
		glUniformMatrix4fv(uniforms["projection_view_pl"], 1, GL_FALSE, glm::value_ptr(projection_view));
		glBindBuffer(GL_UNIFORM_BUFFER, pointLightBlockUBO);
		pointLightBlock.cameraPosition = (const glm::vec3&)scene_->getCamera().getPosition();
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointLightDataBlock), &pointLightBlock);
		glUniform1i(uniforms["useTextures_pl"], useTextures);
	#pragma region Draw call for rendering normal sponza

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, meshes_.size(), 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	#pragma endregion




		glUseProgram(spotLightShaderProgram);
		glUniformMatrix4fv(uniforms["projection_view_sl"], 1, GL_FALSE, glm::value_ptr(projection_view));
		glBindBuffer(GL_UNIFORM_BUFFER, spotLightBlockUBO);
		spotLightDataBlock.cameraPosition = (const glm::vec3&)scene_->getCamera().getPosition();
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SpotLightDataBlock), &spotLightDataBlock);
		glUniform1i(uniforms["useTextures_sl"], useTextures);
	#pragma region Draw call for rendering normal sponza
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, meshes_.size(), 0);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	#pragma endregion


	



#pragma endregion 
}
