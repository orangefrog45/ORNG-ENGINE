#shader vertex
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 inColor;

uniform mat4 gTransform;

out vec4 Color;

void main() {
    gl_Position = gTransform * vec4(position, 1.0);
    Color = vec4(inColor, 1.0f);
}


#shader fragment
#version 330 core

in vec4 Color;

out vec4 FragColor;

void main()
{
    FragColor = Color;
};