#version 330 core

layout (location = 0) in vec3 vertex_position;

out vec3 TexCoords;

uniform mat4 MVP;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	//vec4 pos = projection * view * vec4(vertex_position, 1.0);
 //   gl_Position = pos.xyww;

 //   TexCoords = vertex_position;

	//vec4 pos = projection * view * vec4(vertex_position, 1.0);
    gl_Position = vec4(vertex_position, 1.0);

    TexCoords = mat3(inverse(view)) * vertex_position;
}