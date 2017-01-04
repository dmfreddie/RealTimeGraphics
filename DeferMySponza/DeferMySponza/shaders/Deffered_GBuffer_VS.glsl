#version 440

uniform mat4 projection_view;


layout(location = 0)in vec3 vertex_position;
layout(location = 1)in vec3 vertex_normal;
layout(location = 2)in vec2 vertex_texcoord;
layout(location = 3)in int vertex_diffuse_texture_ID;
layout(location = 4)in mat4 model_matrix;


out vec3 vertexPos;
out vec3 vertexNormal;
out vec2 text_coord;
flat out int vert_diffuse_texture_ID;

void main(void)
{

	vertexNormal = normalize(mat3(model_matrix) *  vertex_normal);
	vertexPos = (mat4x3(model_matrix) * vec4(vertex_position, 1.0)).xyz;
	text_coord = vertex_texcoord;
	vert_diffuse_texture_ID = vertex_diffuse_texture_ID;

	//Set the position of the vertex to draw
	gl_Position = projection_view * model_matrix * vec4(vertex_position, 1.0);
}