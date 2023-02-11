#version 330 core

in vec2 TexCoord0;

out vec4 FragColor;

uniform sampler2D gSampler;

void main()
{
	FragColor = texture2D(gSampler, TexCoord0) + vec4(0.6, 0.6, 0.6, 0);
};