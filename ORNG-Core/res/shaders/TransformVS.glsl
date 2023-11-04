#version 430 core

in layout(location = 0) vec3 pos;

#ifdef COLOR
in layout(location = 1) vec3 icol;
out vec3 col;
#endif

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
	mat4 inv_projection;
	mat4 inv_view;
} PVMatrices;

uniform mat4 transform;
void main() {
#ifdef COLOR
col = icol;
#endif
	gl_Position = PVMatrices.proj_view * transform * vec4(pos, 1.f);
}