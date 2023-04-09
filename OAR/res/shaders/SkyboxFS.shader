#version 430 core

in vec3 TexCoord0;

out vec4 FragColor;

layout(binding = 1) uniform samplerCube gSampler;

void main()
{
	FragColor = texture(gSampler, TexCoord0);
};