#version 330

layout (location = 0) out vec4 gbufferTexture0;
layout (location = 1) out vec4 gbufferTexture1;

in vec3 vertexPos;
in vec3 vertexNormal;
in vec2 text_coord;
in vec3 vert_diffuse_colour;
in vec3 vert_specular_colour;
in float vert_is_vertex_shiney;
in float vert_diffuse_texture_ID;


void main(void)
{
	gbufferTexture0 = vec4(vertexPos, vert_diffuse_texture_ID);
	gbufferTexture1 = vec4(vertexNormal, vert_is_vertex_shiney);
}
