#version 440

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in mat4 model_matrix;
uniform mat4 projection_view;
uniform mat4 model_matrix_Uniform;
uniform mat4 lightSpaceMatrixUniform;

uniform int currentSL;
flat out int InstanceID;
out vec4 FragPosLightSpace;

void main(void)
{
	gl_Position = projection_view * model_matrix_Uniform * vec4(vertex_position, 1.0);
	InstanceID = currentSL;
	FragPosLightSpace = lightSpaceMatrixUniform * model_matrix_Uniform * vec4(vertex_position, 1.0);
}