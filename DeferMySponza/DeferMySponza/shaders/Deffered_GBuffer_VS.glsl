#version 440

uniform mat4 projection_view;


layout(location = 0)in vec3 vertex_position;
layout(location = 1)in vec3 vertex_normal;
layout(location = 2)in vec2 vertex_texcoord;
layout(location = 3)in vec3 vertex_diffuse_colour;
layout(location = 4)in vec3 vertex_specular_colour;
layout(location = 5)in float vertex_is_vertex_shiney;
layout(location = 6)in int vertex_diffuse_texture_ID;
layout(location = 7)in mat4 model_matrix;

out vec3 vertexPos;
out vec3 vertexNormal;
out vec2 text_coord;
out vec3 vert_diffuse_colour;
out vec3 vert_specular_colour;
out float vert_is_vertex_shiney;
out float vert_diffuse_texture_ID;

void main(void)
{
	//Convert the noormals and positions into world space
	vertexNormal = mat3(model_matrix) *  vertex_normal;
	vertexPos = mat3(model_matrix) * vertex_position;

	vert_diffuse_colour = vertex_diffuse_colour;
	vert_specular_colour = vertex_specular_colour;
	vert_is_vertex_shiney = vertex_is_vertex_shiney;
	vert_diffuse_texture_ID = vertex_diffuse_texture_ID;

	//Pass the texture coordinate for the vertex into the fragment shader
	text_coord = vertex_texcoord;

	//Set the position of the vertex to draw
	gl_Position = projection_view * model_matrix * vec4(vertex_position, 1.0);
}