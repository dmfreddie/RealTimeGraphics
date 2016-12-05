#version 440

layout (location = 0) out vec3 gbufferTexture0;
layout (location = 1) out vec3 gbufferTexture1;
layout (location = 2) out vec3 gbufferTexture2;


in vec3 vertexPos;
in vec3 vertexNormal;
in vec2 text_coord;
in vec3 vert_diffuse_colour;
in vec3 vert_specular_colour;
in float vert_is_vertex_shiney;
flat in int vert_diffuse_texture_ID;


void main(void)
{
	gbufferTexture0 = vec3 (vertexPos);
	gbufferTexture1 = vec3 (vertexNormal);
	gbufferTexture2 = vec3 (text_coord, vert_diffuse_texture_ID);
}
