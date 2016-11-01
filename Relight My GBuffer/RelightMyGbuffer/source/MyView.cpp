#include "MyView.hpp"
#include <tygra/FileHelper.hpp>
#include <tsl/shapes.hpp>
#include <tcf/Image.hpp>
#include <tcf/tcf.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

MyView::MyView()
{
}

MyView::~MyView() {
}

void MyView::windowViewWillStart(tygra::Window * window)
{
    /*
     * Tutorial: this section of code creates a fullscreen quad to be used
     *           when computing global illumination effects (e.g. ambient)
     */
    {
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
    }

    /*
     * Tutorial: this code creates a sphere to use when deferred shading
     *           with a point light source.
     */
    {
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
    }

    /*
     * Tutorial: this code creates a shader program for the global lighting
     */
    {
        GLint compile_status = 0;

        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        std::string vertex_shader_string
            = tygra::createStringFromFile("resource:///global_light_vs.glsl");
        const char *vertex_shader_code = vertex_shader_string.c_str();
        glShaderSource(vertex_shader, 1,
            (const GLchar **)&vertex_shader_code, NULL);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
        if (compile_status != GL_TRUE) {
            const int string_length = 1024;
            GLchar log[string_length] = "";
            glGetShaderInfoLog(vertex_shader, string_length, NULL, log);
            std::cerr << log << std::endl;
        }

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        std::string fragment_shader_string =
            tygra::createStringFromFile("resource:///global_light_fs.glsl");
        const char *fragment_shader_code = fragment_shader_string.c_str();
        glShaderSource(fragment_shader, 1,
            (const GLchar **)&fragment_shader_code, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
        if (compile_status != GL_TRUE) {
            const int string_length = 1024;
            GLchar log[string_length] = "";
            glGetShaderInfoLog(fragment_shader, string_length, NULL, log);
            std::cerr << log << std::endl;
        }

        global_light_prog_ = glCreateProgram();
        glAttachShader(global_light_prog_, vertex_shader);
        glBindAttribLocation(global_light_prog_, 0, "vertex_position");
        glDeleteShader(vertex_shader);
        glAttachShader(global_light_prog_, fragment_shader);
        glBindFragDataLocation(global_light_prog_, 0, "reflected_light");
        glDeleteShader(fragment_shader);
        glLinkProgram(global_light_prog_);

        GLint link_status = 0;
        glGetProgramiv(global_light_prog_, GL_LINK_STATUS, &link_status);
        if (link_status != GL_TRUE) {
            const int string_length = 1024;
            GLchar log[string_length] = "";
            glGetProgramInfoLog(global_light_prog_, string_length, NULL, log);
            std::cerr << log << std::endl;
        }
    }

	{
		GLint compile_status = 0;

		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		std::string vertex_shader_string
			= tygra::createStringFromFile("resource:///point_light_vs.glsl");
		const char *vertex_shader_code = vertex_shader_string.c_str();
		glShaderSource(vertex_shader, 1,
			(const GLchar **)&vertex_shader_code, NULL);
		glCompileShader(vertex_shader);
		glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
		if (compile_status != GL_TRUE) {
			const int string_length = 1024;
			GLchar log[string_length] = "";
			glGetShaderInfoLog(vertex_shader, string_length, NULL, log);
			std::cerr << log << std::endl;
		}

		GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		std::string fragment_shader_string =
			tygra::createStringFromFile("resource:///point_light_fs.glsl");
		const char *fragment_shader_code = fragment_shader_string.c_str();
		glShaderSource(fragment_shader, 1,
			(const GLchar **)&fragment_shader_code, NULL);
		glCompileShader(fragment_shader);
		glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
		if (compile_status != GL_TRUE) {
			const int string_length = 1024;
			GLchar log[string_length] = "";
			glGetShaderInfoLog(fragment_shader, string_length, NULL, log);
			std::cerr << log << std::endl;
		}

		point_light_prog_ = glCreateProgram();
		glAttachShader(point_light_prog_, vertex_shader);
		glBindAttribLocation(point_light_prog_, 0, "vertex_position");
		glDeleteShader(vertex_shader);
		glAttachShader(point_light_prog_, fragment_shader);
		glBindFragDataLocation(point_light_prog_, 0, "reflected_light");
		glDeleteShader(fragment_shader);
		glLinkProgram(point_light_prog_);

		GLint link_status = 0;
		glGetProgramiv(point_light_prog_, GL_LINK_STATUS, &link_status);
		if (link_status != GL_TRUE) {
			const int string_length = 1024;
			GLchar log[string_length] = "";
			glGetProgramInfoLog(point_light_prog_, string_length, NULL, log);
			std::cerr << log << std::endl;
		}
	}

    /*
     * Tutorial: All of the framebuffers, renderbuffers and texture objects
     *           that you'll need for this tutorial are gen'd here but not
     *           created until windowViewDidReset because they are usually
     *           window size dependent.
     */

    glGenTextures(1, &gbuffer_position_tex_);
    glGenTextures(1, &gbuffer_normal_tex_);
    glGenTextures(1, &gbuffer_depth_tex_);

    glGenFramebuffers(1, &lbuffer_fbo_);
    glGenRenderbuffers(1, &lbuffer_colour_rbo_);

    /*
     * Tutorial: The gbuffer data in this tutorial is read from file and
     *           inserted into texture objects. These settings ensure the
     *           pixel data is uploaded in the correct format.
     */

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
}

void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
    /*
     * Tutorial: Load the gbuffer data from file.  Not normally done
     *           with deferred shading.  Usually the gbuffer is drawn
     *           to each frame.
     */

	

    std::vector<tcf::ImagePtr> gbuffer_images;
    try {
        tcf::ReaderPtr tcfReader = tcf::createReaderPtr();
        tcfReader->openFile("sponza_gbuffer.tcf");
        while (tcfReader->hasChunk()) {
            tcfReader->openChunk();
            if (tcf::chunkIsImage(tcfReader.get())) {
                tcf::Image * image = tcf::readImage(tcfReader.get());
                tcf::ImagePtr ptr = tcf::ImagePtr(image, tcf::deleteImage);
                gbuffer_images.push_back(std::move(ptr));
            } else {
                tcfReader->closeChunk();
            }
        }
    }
    catch (...) {
        std::cerr << "Error reading Gbuffer data file" << std::endl;
        return;
    }

    // override actual window size to match gbuffer image data
    width = gbuffer_images[0]->width();
    height = gbuffer_images[0]->height();


    glViewport(0, 0, width, height);


    /*
     * Tutorial: This is where you'll recreate texture and renderbuffer objects
     *           and attach them to framebuffer objects.
     */
	GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0,
		GL_RGB,
		GL_FLOAT,
		gbuffer_images[0]->pixelData());
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGB32F, width, height, 0,
		GL_RGB,
		GL_FLOAT,
		gbuffer_images[1]->pixelData());
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_depth_tex_);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_DEPTH24_STENCIL8, width, height, 0,
		GL_DEPTH_STENCIL,
		GL_UNSIGNED_INT_24_8,
		gbuffer_images[2]->pixelData());
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	
	glUseProgram(global_light_prog_);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_position_tex_);
	glUniform1i(glGetUniformLocation(global_light_prog_, "sampler_world_position"), 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE, gbuffer_normal_tex_);
	glUniform1i(glGetUniformLocation(global_light_prog_, "sampler_world_normal"), 1);

    GLenum framebuffer_status = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);
    // TODO: attach textures and buffers here
	glBindRenderbuffer(GL_RENDERBUFFER, lbuffer_colour_rbo_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, width, height);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_RECTANGLE, gbuffer_depth_tex_, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, lbuffer_colour_rbo_);


    framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
        tglDebugMessage(GL_DEBUG_SEVERITY_HIGH_ARB, "framebuffer not complete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MyView::windowViewDidStop(tygra::Window * window)
{
    glDeleteProgram(global_light_prog_);
    glDeleteProgram(point_light_prog_);

    glDeleteBuffers(1, &light_quad_mesh_.vertex_vbo);
    glDeleteBuffers(1, &light_quad_mesh_.element_vbo);
    glDeleteVertexArrays(1, &light_quad_mesh_.vao);

    glDeleteBuffers(1, &light_sphere_mesh_.vertex_vbo);
    glDeleteBuffers(1, &light_sphere_mesh_.element_vbo);
    glDeleteVertexArrays(1, &light_sphere_mesh_.vao);

    glDeleteTextures(1, &gbuffer_position_tex_);
    glDeleteTextures(1, &gbuffer_normal_tex_);
    glDeleteTextures(1, &gbuffer_depth_tex_);

    glDeleteFramebuffers(1, &lbuffer_fbo_);
    glDeleteRenderbuffers(1, &lbuffer_colour_rbo_);
}

void MyView::windowViewRender(tygra::Window * window)
{
    GLint viewport_size[4];
    glGetIntegerv(GL_VIEWPORT, viewport_size);
    const float aspect_ratio = viewport_size[2] / (float)viewport_size[3];

    /*
     * Tutorial: These constants match how the gbuffer was originally rendered.
     *           Do not modify or ignore them.
     */

    const glm::vec3 camera_position = glm::vec3(96.f, 22.f, -8.f);
    const glm::vec3 camera_direction = glm::vec3(-0.98f, 0.18f, 0.11f);
    glm::mat4 projection_xform = glm::perspective(75.f,
                                                  aspect_ratio,
                                                  1.f, 1000.f);
    glm::mat4 view_xform = glm::lookAt(camera_position,
                                       camera_position + camera_direction,
                                       glm::vec3(0, 1, 0));
	glm::mat4 projection_view = projection_xform * view_xform;
    const glm::vec3 global_light_direction
        = glm::normalize(glm::vec3(-3.f, -2.f, 1.f));
    const unsigned int point_light_count = 3;
    const glm::vec3 point_light_position[3] =
    {
        glm::vec3(-80, 20, 0),
        glm::vec3(0, 20, 0),
        glm::vec3(80, 20, 0)
    };
    const float point_light_range[3] = { 40, 40, 40 };

    /*
     * Tutorial: Add your drawing code here as directed in the worksheet.
     */

	glBindFramebuffer(GL_FRAMEBUFFER, lbuffer_fbo_);
	glClearColor(1.0, 0.05, 0.6, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(global_light_prog_);
	glUniform3fv(glGetUniformLocation(global_light_prog_, "light_direction"), 1, glm::value_ptr(global_light_direction));
	glBindVertexArray(light_quad_mesh_.vao);

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0x00);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindVertexArray(0);

	glDisable(GL_DEPTH_TEST);
	glUseProgram(point_light_prog_);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);
	glDepthFunc(GL_EQUAL);
	glDepthMask(GL_FALSE);

	glUniformMatrix4fv(glGetUniformLocation(point_light_prog_, "projection_view"), 1, GL_FALSE, glm::value_ptr(projection_view));

	for (int i = 0; i < point_light_count; ++i)
	{
		glUniform3fv(glGetUniformLocation(point_light_prog_, "point_light_position"), 1, glm::value_ptr(point_light_position[i]));

		glUniform1f(glGetUniformLocation(point_light_prog_, "point_light_range"), point_light_range[i]);

		glm::mat4 model_matrix = glm::mat4(0);
		model_matrix = glm::translate(model_matrix, point_light_position[i]);
		glUniformMatrix4fv(glGetUniformLocation(point_light_prog_, "model_matrix"), 1, GL_FALSE, glm::value_ptr(model_matrix));

		glBindVertexArray(light_sphere_mesh_.vao);
		glDrawElements(GL_TRIANGLE_FAN, light_sphere_mesh_.element_count, GL_UNSIGNED_INT, nullptr);
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, lbuffer_fbo_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, viewport_size[2], viewport_size[3], 0, 0, viewport_size[2], viewport_size[3], GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
