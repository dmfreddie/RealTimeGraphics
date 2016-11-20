#version 440

in vec3 vertex_position;
uniform mat4 projection_view;
uniform mat4 model_matrix;


void main(void)
{
	gl_Position = projection_view * model_matrix * vec4(vertex_position, 1.0);
}