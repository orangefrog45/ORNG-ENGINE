#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 gTransform;

out vec3 TexCoord0;

void main() {
	vec4 pos = gTransform * vec4(position, 1.0);
	gl_Position = pos.xyzz;
	TexCoord0 = position;
}
