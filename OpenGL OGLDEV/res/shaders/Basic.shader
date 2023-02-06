#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;

uniform mat4 gTransform;

out vec2 TexCoord0;

void main() {
    gl_Position = gTransform * vec4(position, 1.0);
    TexCoord0 = TexCoord;
}


#shader fragment
#version 330 core

in vec2 TexCoord0;

out vec4 FragColor;

uniform sampler2D gSampler;

void main()
{
    FragColor = texture2D(gSampler, TexCoord0);
};