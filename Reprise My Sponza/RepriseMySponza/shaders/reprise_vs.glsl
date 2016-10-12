#version 440

uniform mat4 projection_view_xform;


in vec3 vertex_position;
in vec3 vertex_normal;
in vec2 vertex_texcoord;
in mat4 model_matrix;

out vec3 vertexPos;
out vec3 vertexNormal;
out vec2 text_coord;


void main(void)
{
	//Convert the noormals and positions into world space
	vertexNormal = mat3(model_matrix) *  vertex_normal;
	vertexPos = mat4x3(model_matrix) * vec4(vertex_position, 1.0);


	//Pass the texture coordinate for the vertex into the fragment shader
	text_coord = vertex_texcoord;

	//Set the position of the vertex to draw
	gl_Position = projection_view_xform * model_matrix * vec4(vertex_position, 1.0);
}