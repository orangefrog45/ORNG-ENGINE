#version 430 core

in vec3 TexCoord0;

out vec4 FragColor;

layout(binding = 1) uniform samplerCube sky_texture;

void main()
{
	FragColor = texture(sky_texture, TexCoord0);
};