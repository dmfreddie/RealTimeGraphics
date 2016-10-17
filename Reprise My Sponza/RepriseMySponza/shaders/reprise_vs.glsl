#version 440

uniform mat4 projection_view;


in vec3 vertex_position;
in vec3 vertex_normal;
in vec2 vertex_texcoord;
in vec3 vertex_diffuse_colour;
in vec3 vertex_specular_colour;
in float vertex_is_vertex_shiney;
in int vertex_diffuse_texture_ID;
in mat4 model_matrix;

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
	vertexPos = mat4x3(model_matrix) * vec4(vertex_position, 1.0);

	vert_diffuse_colour = vertex_diffuse_colour;
	vert_specular_colour = vertex_specular_colour;
	vert_is_vertex_shiney = vertex_is_vertex_shiney;
	vert_diffuse_texture_ID = vertex_diffuse_texture_ID;

	//Pass the texture coordinate for the vertex into the fragment shader
	text_coord = vertex_texcoord;

	//Set the position of the vertex to draw
	gl_Position = projection_view * model_matrix * vec4(vertex_position, 1.0);
}