#shader vertex
#version 330 core

layout(location = 0) in vec4 position;

uniform float gScale;

void main() {
    gl_Position = vec4(position.x * gScale, position.y * gScale, position.z * gScale, 1.0);
}


#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform vec4 u_Color;

void main()
{
    color = u_Color;
};