#version 330

layout (location = 0) out vec4 gbufferTexture0;
layout (location = 1) out vec4 gbufferTexture1;
layout (location = 2) out vec4 gbufferTexture2;


in vec3 vertexPos;
in vec3 vertexNormal;
in vec2 text_coord;
in vec3 vert_diffuse_colour;
in float vert_is_vertex_shiney;
in vec3 vert_specular_colour;
in float vert_diffuse_texture_ID;


void main(void)
{
	gbufferTexture0 = vec4(vertexPos, 0.0);
	gbufferTexture1 = vec4(vertexNormal, 0.0);
	gbufferTexture2 = vec4(vert_diffuse_texture_ID, vert_diffuse_texture_ID, vert_diffuse_texture_ID, vert_diffuse_texture_ID);
}
