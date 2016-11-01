#version 330

in vec2 vertex_position;
uniform mat4 projection_view;
uniform mat4 model_matrix;

void main(void)
{
    gl_Position = projection_view * model_matrix * vec4(vertex_position, 0.0, 1.0);
}
