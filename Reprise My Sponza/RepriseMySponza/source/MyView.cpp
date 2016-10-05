#include "MyView.hpp"

#include <scene/scene.hpp>
#include <tygra/FileHelper.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cassert>

//TODO: Use radians

MyView::MyView() : shaderProgram(0)
{
}

MyView::~MyView() {
}

void MyView::setScene(const scene::Context * scene)
{
    scene_ = scene;
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
	CompileShader("resource:///reprise_vs.glsl", GL_VERTEX_SHADER, vertex_shader);
	CompileShader("resource:///reprise_fs.glsl", GL_FRAGMENT_SHADER, fragment_shader);


	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertex_shader);
	glBindAttribLocation(shaderProgram, 0, "vertex_position");
	glBindAttribLocation(shaderProgram, 1, "vertex_normal");
	glBindAttribLocation(shaderProgram, 2, "vertex_tangent");
	glBindAttribLocation(shaderProgram, 3, "vertex_texcoord");
	glDeleteShader(vertex_shader);
	glAttachShader(shaderProgram, fragment_shader);
	glDeleteShader(fragment_shader);
	glLinkProgram(shaderProgram);

	
	if (CheckLinkStatus(shaderProgram))
		std::cout << "Shaders Compiled!" << std::endl;

	Getuniforms();
}



void MyView::Getuniforms()
{
#pragma region
	/*
	Get the uniform locations of the uniform variables in the shader for each program where the varibale needs to be placed and bind it to a GLuint
	inside the unordered map. An unordered map was used because every time a new unifrom Gluint is added it doesn't need to re-order the map. This is
	also performed on start because the locations of the uniforms wont need to be changed per frame so it will speed up the rendering function as it
	doesn't need to perform unnessacary computation.
	*/
	uniforms["projection_view_model_xform"] = glGetUniformLocation(shaderProgram, "projection_view_model_xform");
	uniforms["model_xform"] = glGetUniformLocation(shaderProgram, "model_xform");
	uniforms["vertex_diffuse_colour"] = glGetUniformLocation(shaderProgram, "vertex_diffuse_colour");
	uniforms["vertex_ambient_colour"] = glGetUniformLocation(shaderProgram, "vertex_ambient_colour");
	uniforms["vertex_spec_colour"] = glGetUniformLocation(shaderProgram, "vertex_spec_colour");
	uniforms["vertex_shininess"] = glGetUniformLocation(shaderProgram, "vertex_shininess");
	uniforms["specular_smudge_factor"] = glGetUniformLocation(shaderProgram, "specular_smudge_factor");
	uniforms["is_vertex_shiney"] = glGetUniformLocation(shaderProgram, "is_vertex_shiney");
	uniforms["camera_position"] = glGetUniformLocation(shaderProgram, "camera_position");

	uniforms["global_ambient_light"] = glGetUniformLocation(shaderProgram, "global_ambient_light");

	uniforms["MAX_LIGHTS"] = glGetUniformLocation(shaderProgram, "MAX_LIGHTS");
	uniforms["MAX_SPOT_LIGHTS"] = glGetUniformLocation(shaderProgram, "MAX_SPOT_LIGHTS");
	uniforms["MAX_DIR_LIGHTS"] = glGetUniformLocation(shaderProgram, "MAX_DIR_LIGHTS");

	uniforms["outline"] = glGetUniformLocation(shaderProgram, "outline");
	uniforms["has_diff_tex"] = glGetUniformLocation(shaderProgram, "has_diff_tex");

#pragma endregion // Get the uniform locations
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

void MyView::ResetConsole()
{
	system("cls");
	//Application instructions
	std::cout << "ABOUT" << std::endl;
	std::cout << "Spice My Sponza - 3D Graphics Programming ICA1" << std::endl;
	std::cout << "P4011584 - Frederic Babord 2015 - 2016" << std::endl << std::endl;
	std::cout << "Submission date: 04th February 2016" << std::endl << std::endl;
	std::cout << "INSTRUCTIONS" << std::endl;
	std::cout << "Press F1 to enable camera animation." << std::endl;
	std::cout << "Press F2 to view the direction of the vertex normals." << std::endl;
	std::cout << "Press F3 to enable wireframe mode." << std::endl;
	std::cout << "Press F4 to change rendering mode (Fill, Line, Point)." << std::endl;
	std::cout << "Press F5 to recompile the shader." << std::endl << std::endl;
	std::cout << "Press Q to reduce the normal line length." << std::endl;
	std::cout << "Press E to increase the normal line length." << std::endl;
	std::cout << "Press Z to reduce the specular intensity smudge factor." << std::endl;
	std::cout << "Press C to increase the specular intensity smudge factor." << std::endl << std::endl;
	std::cout << "Press Esc to Close." << std::endl << std::endl;
	CompileShaders();
}

void MyView::windowViewWillStart(tygra::Window * window)
{
    assert(scene_ != nullptr);

	ResetConsole();



#pragma region
	scene::GeometryBuilder builder;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> elements;
	const auto& scene_meshes = builder.getAllMeshes();
	for (const auto& scene_mesh : scene_meshes) {

		MeshGL& newMesh = meshes_[scene_mesh.getId()];
		const auto& source_mesh = builder.getMeshById(scene_mesh.getId());
		const auto& positions = source_mesh.getPositionArray();
		const auto& elementsArr = source_mesh.getElementArray();
		const auto& normals = source_mesh.getNormalArray();
		//const auto& tangents = source_mesh.getTangentArray();
		const auto& text_coord = source_mesh.getTextureCoordinateArray();

		bool hasTexCood = text_coord.size() > 0;

		newMesh.first_vertex_index = vertices.size();
		vertices.reserve(vertices.size() + positions.size());

		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			Vertex vertex;
			vertex.position = (const glm::vec3&)positions[i];
			vertex.normal = (const glm::vec3&)normals[i];
			//vertex.tangent = (const glm::vec3&)tangents[i];
			if(hasTexCood)
				vertex.texcoord = (const glm::vec2&)text_coord[i];
			vertices.push_back(vertex);
		}

		newMesh.first_element_index = elements.size();
		elements.insert(elements.end(), elementsArr.begin(), elementsArr.end());
		newMesh.element_count = elementsArr.size();
	}

	glGenBuffers(1, &vertex_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(Vertex), // size of data in bytes
		vertices.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, element_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		elements.size() * sizeof(unsigned int), // size of data in bytes
		elements.data(), // pointer to the data
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, tangent));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), TGL_BUFFER_OFFSET_OF(Vertex, texcoord));

	// make nothing active (deactivate vbo and vao)
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint pointLightIndex = glGetUniformBlockIndex(shaderProgram, "PointLightingBlock");
	GLuint spotLightIndex = glGetUniformBlockIndex(shaderProgram, "SpotLightingBlock");
	GLuint dirLightIndex = glGetUniformBlockIndex(shaderProgram, "DirectionalLightBlock");

	glUniformBlockBinding(shaderProgram, pointLightIndex, 0);
	glUniformBlockBinding(shaderProgram, spotLightIndex, 1);
	glUniformBlockBinding(shaderProgram, dirLightIndex, 2);


	glGenBuffers(1, &ubo);

	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SpotLight) * 7 + sizeof(PointLight) * 22 + sizeof(DirectionalLight) * 3, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo, 0, sizeof(PointLight) * 22);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, ubo, sizeof(PointLight) * 22, sizeof(SpotLight) * 7);
	glBindBufferRange(GL_UNIFORM_BUFFER, 2, ubo, sizeof(SpotLight) * 7 + sizeof(PointLight) * 22, sizeof(DirectionalLight) * 3);
	glBindVertexArray(0);

#pragma endregion //Load the mesh into buffers



#pragma region // Uniform Buffers

	


#pragma endregion 



#pragma region

	//Load the textures into a map of handles
	LoadTexture("resource:///hex.png");
	/*LoadTexture("diff1.png");
	LoadTexture("spec1.png");
	LoadTexture("spec2.png");
*/
	//Create the light vector so there will be memory already reserved that can just be overwritten if values have been changed. This has been done on 
	//start for effiences in the constant render loop function.
	auto& pointLightsRef = scene_->getAllPointLights();
	for (unsigned int i = 0; i < pointLightsRef.size(); ++i)
	{
		PointLight light;
		light.position = (const glm::vec3&) pointLightsRef[i].getPosition();
		light.range = pointLightsRef[i].getRange();
		light.intensity = (const glm::vec3&) pointLightsRef[i].getIntensity();
		pointLights.push_back(light);
	}
	auto& directionalLightRef = scene_->getAllDirectionalLights();
	for (unsigned int i = 0; i < directionalLightRef.size(); ++i)
	{
		DirectionalLight light;
		light.direction = (const glm::vec3&) directionalLightRef[i].getDirection();
		light.intensity = (const glm::vec3&) directionalLightRef[i].getIntensity();
		directionalLights.push_back(light);
	}
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
		spotLights.push_back(light);
	}
	//TODO: FIX
	glBindVertexArray(vao);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData( GL_UNIFORM_BUFFER, 2 * sizeof(glm::vec3) + 2 * sizeof(float), (3 * sizeof(glm::vec3) + 3 * sizeof(float)) * spotLights.size(), glm::value_ptr(spotLights[0].position));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindVertexArray(0);
#pragma endregion // Textures and Lights



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

	glDeleteBuffers(1, &vertex_vbo);
	glDeleteBuffers(1, &element_vbo);
	glDeleteVertexArrays(1, &vao);
}

void MyView::windowViewRender(tygra::Window * window)
{
    assert(scene_ != nullptr);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glClearColor(0.556f, 0.822f, 1.0f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLint viewport_size[4];
	glGetIntegerv(GL_VIEWPORT, viewport_size);
	const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];

	//Use the initial shader program to render sponza normally
	glUseProgram(shaderProgram);

	glm::mat4 projection_xform = glm::perspective(glm::radians(scene_->getCamera().getVerticalFieldOfViewInDegrees()), aspect_ratio, scene_->getCamera().getNearPlaneDistance(), scene_->getCamera().getFarPlaneDistance());
	glm::mat4 view_xform = glm::lookAt((const glm::vec3&)scene_->getCamera().getPosition(), (const glm::vec3&)scene_->getCamera().getPosition() + (const glm::vec3&)scene_->getCamera().getDirection(), (const glm::vec3&)scene_->getUpDirection());

	glUniform3fv(uniforms["camera_position"], 1, glm::value_ptr((const glm::vec3&)scene_->getCamera().getPosition()));


#pragma region Update sponzas lighting for this frame

	/*
	Get the light data from the scene and pass out the data into a uniform array of light structs in the vertex shader
	for each point light and the ambient light for the scene. The shader also needs to know how many lights are currently
	active and so that value is also passed out into the shader.
	*/

	glUniform1f(uniforms["MAX_LIGHTS"], (GLfloat)scene_->getAllPointLights().size());
	glUniform1f(uniforms["MAX_SPOT_LIGHTS"], (GLfloat)scene_->getAllSpotLights().size());
	glUniform1f(uniforms["MAX_DIR_LIGHTS"], (GLfloat)scene_->getAllDirectionalLights().size());

	for (unsigned int i = 0; i < scene_->getAllPointLights().size(); ++i)
	{
		pointLights[i].position = (const glm::vec3&)scene_->getAllPointLights()[i].getPosition();
		pointLights[i].range = scene_->getAllPointLights()[i].getRange();
		pointLights[i].intensity = (const glm::vec3&)scene_->getAllPointLights()[i].getIntensity();
	}

	GLuint lightPosID = 0;
	GLuint lightRangeID = 0;
	GLuint lightIntensityID = 0;

	for (unsigned int i = 0; i < scene_->getAllPointLights().size(); ++i)
	{
		std::string pos = "LightSource[" + std::to_string(i) + "].position";
		std::string range = "LightSource[" + std::to_string(i) + "].range";
		std::string intensity = "LightSource[" + std::to_string(i) + "].intensity";

		lightPosID = glGetUniformLocation(shaderProgram, pos.c_str());
		glUniform3fv(lightPosID, 1, glm::value_ptr(pointLights[i].position));
		lightRangeID = glGetUniformLocation(shaderProgram, range.c_str());
		glUniform1f(lightRangeID, pointLights[i].range);
		lightIntensityID = glGetUniformLocation(shaderProgram, intensity.c_str());
		glUniform3fv(lightIntensityID, 1, glm::value_ptr(pointLights[i].intensity));
	}

	auto& directionalLightRef = scene_->getAllDirectionalLights();
	for (unsigned int i = 0; i < directionalLightRef.size(); ++i)
	{
		directionalLights[i].direction = (const glm::vec3&) directionalLightRef[i].getDirection();
		directionalLights[i].intensity = (const glm::vec3&) directionalLightRef[i].getIntensity();
	}

	GLuint dirlightDirectionID = 0;
	GLuint dirLightIntensityID = 0;

	for (unsigned int i = 0; i < scene_->getAllDirectionalLights().size(); ++i)
	{
		std::string dir = "DirectionalLightSources[" + std::to_string(i) + "].direction";
		std::string intensity = "DirectionalLightSources[" + std::to_string(i) + "].intensity";

		dirlightDirectionID = glGetUniformLocation(shaderProgram, dir.c_str());
		dirLightIntensityID = glGetUniformLocation(shaderProgram, intensity.c_str());

		glUniform3fv(dirlightDirectionID, 1, glm::value_ptr(directionalLights[i].direction));
		glUniform3fv(dirLightIntensityID, 1, glm::value_ptr(directionalLights[i].intensity));
	}


	auto& spotLightRef = scene_->getAllSpotLights();
	for (unsigned int i = 0; i < spotLightRef.size(); ++i)
	{
		spotLights[i].direction = (const glm::vec3&) spotLightRef[i].getDirection();
		spotLights[i].intensity = (const glm::vec3&) spotLightRef[i].getIntensity();
		spotLights[i].position = (const glm::vec3&) spotLightRef[i].getPosition();
		spotLights[i].coneAngle = spotLightRef[i].getConeAngleDegrees();
		spotLights[i].range = spotLightRef[i].getRange();
		spotLights[i].castShadow = spotLightRef[i].getCastShadow();
	}

	GLuint spotlightPosID = 0;
	GLuint spotlightRangeID = 0;
	GLuint spotlightIntensityID = 0;
	GLuint spotlightConeAngleID = 0;
	GLuint spotlightdirectionID = 0;

	for (unsigned int i = 0; i < scene_->getAllSpotLights().size(); ++i)
	{
		std::string pos =		"SpotLightSources[" + std::to_string(i) + "].position";
		std::string range =		"SpotLightSources[" + std::to_string(i) + "].range";
		std::string direction = "SpotLightSources[" + std::to_string(i) + "].direction";
		std::string coneAngle = "SpotLightSources[" + std::to_string(i) + "].coneAngle";
		std::string intensity = "SpotLightSources[" + std::to_string(i) + "].intensity";

		spotlightPosID = glGetUniformLocation(shaderProgram, pos.c_str());
		spotlightRangeID = glGetUniformLocation(shaderProgram, range.c_str());
		spotlightIntensityID = glGetUniformLocation(shaderProgram, intensity.c_str());
		spotlightConeAngleID = glGetUniformLocation(shaderProgram, coneAngle.c_str());
		spotlightdirectionID = glGetUniformLocation(shaderProgram, direction.c_str());

		glUniform3fv(spotlightPosID, 1, glm::value_ptr(spotLights[i].position));
		glUniform1f(spotlightRangeID, spotLights[i].range);
		glUniform3fv(spotlightdirectionID, 1, glm::value_ptr(spotLights[i].direction));
		glUniform1f(spotlightConeAngleID, spotLights[i].coneAngle);
		glUniform3fv(spotlightIntensityID, 1, glm::value_ptr(spotLights[i].intensity));
	}


	


	glUniform3fv(uniforms["global_ambient_light"], 1, glm::value_ptr((const glm::vec3&)scene_->getAmbientLightIntensity()));

#pragma endregion 

	glBindVertexArray(vao);

	glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointLight) * pointLights.size(), glm::value_ptr(pointLights[0].position));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(PointLight) * 22, sizeof(SpotLight) * spotLights.size(), glm::value_ptr(spotLights[0].position));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(PointLight) * 22 + sizeof(SpotLight) *7, sizeof(DirectionalLight) * directionalLights.size(), glm::value_ptr(directionalLights[0].direction));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

#pragma region Draw call for rendering normal sponza

	//float outLineInt = 0;
	//glUniform1f(uniforms["outline"], outLineInt);

	glm::mat4 projection_view_mod_xform;
	glm::mat4 inverse_normal_xform;

	//Initialised to first element as there is no default constructor for a material
	scene::Material material = scene_->getAllMaterials()[0];

	/*
	Populate the material uniform variables and the model uniform variables and then draw sponza normally
	*/
	for (const auto& instance : scene_->getAllInstances())
	{
		glm::mat4 model_xform = glm::mat4((const glm::mat4x3&)instance.getTransformationMatrix());
		const MeshGL& mesh = meshes_[instance.getMeshId()];

		projection_view_mod_xform = projection_xform * view_xform * model_xform;
		inverse_normal_xform = view_xform * model_xform;

		material = scene_->getMaterialById(instance.getMaterialId());

		glUniformMatrix4fv(uniforms["projection_view_model_xform"], 1, GL_FALSE, glm::value_ptr(projection_view_mod_xform));
		glUniformMatrix4fv(uniforms["model_xform"], 1, GL_FALSE, glm::value_ptr(model_xform));
		glUniform3fv(uniforms["vertex_diffuse_colour"], 1, glm::value_ptr((const glm::mat3&)material.getDiffuseColour()));
		glUniform3fv(uniforms["vertex_spec_colour"], 1, glm::value_ptr((const glm::mat3&)material.getSpecularColour()));
		glUniform1f(uniforms["vertex_shininess"], material.getShininess());
		glUniform1f(uniforms["is_vertex_shiney"], (float)material.isShiny());

		glUniform1f(uniforms["has_diff_tex"], 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures["resource:///hex.png"]);
		glUniform1i(glGetUniformLocation(shaderProgram, "diffuse_texture"), 0);
		
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.element_count, GL_UNSIGNED_INT, (GLvoid*)(mesh.first_element_index * sizeof(int)), mesh.first_vertex_index);


	}

#pragma endregion 
}
