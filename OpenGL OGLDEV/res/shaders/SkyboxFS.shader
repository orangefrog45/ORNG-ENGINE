#version 330 core

in vec3 TexCoord0;

out vec4 FragColor;

uniform samplerCube gSampler;

void main()
{
	FragColor = texture(gSampler, TexCoord0);
};