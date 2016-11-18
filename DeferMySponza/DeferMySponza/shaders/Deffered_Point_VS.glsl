#version 440

in vec3 vertex_position;
uniform mat4 projection_view;
uniform mat4 model_matrix;

struct PointLight
{
	vec3 position;
	float range;
	vec3 intensity;
	float padding;
	mat4 modelMatrix;
};

layout(std140) uniform DataBlock{
	PointLight pointLights[22];
	vec3 camera_position;
	float maxPointLights;
};


void main(void)
{
	gl_Position = projection_view * pointLights[] * vec4(vertex_position, 1.0);
}