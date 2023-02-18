#version 330 core

in vec2 TexCoord0;

out vec4 FragColor;

struct BaseLight {
	vec3 color;
	float ambient_intensity;
};

struct Material {
	vec3 ambient_color;
};

uniform BaseLight g_light;
//uniform Material g_material;


uniform sampler2D gSampler;

void main()
{
	FragColor = texture2D(gSampler, TexCoord0) * vec4(g_light.color, 1.0f) * g_light.ambient_intensity;

	//FragColor = texture2D(gSampler, TexCoord0) * vec4(g_material.ambient_color, 1.0f) * vec4(g_light.color, 1.0f) * g_light.ambient_intensity;
};