#version 440

layout(location = 0) out float fragmentdepth;

void main(void)
{
	fragmentdepth = gl_FragCoord.z;
}
