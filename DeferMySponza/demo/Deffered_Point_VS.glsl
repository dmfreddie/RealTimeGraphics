#version 440

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in mat4 model_matrix;
uniform mat4 projection_view;

flat out int InstanceID;

void main(void)
{
	gl_Position = projection_view * model_matrix * vec4(vertex_position, 1.0);
	InstanceID = gl_InstanceID;
}