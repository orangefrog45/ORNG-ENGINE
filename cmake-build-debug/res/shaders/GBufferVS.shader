#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) mat4 world_transform;
in layout(location = 7) vec3 in_tangent;



const unsigned int NUM_SHADOW_CASCADES = 3;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_tex_coord;
out vec3 vs_tangent;

uniform bool terrain_mode;

void main() {

	vs_tex_coord = tex_coord;
	vs_tangent = in_tangent;

	if (terrain_mode) {
		vs_normal = vertex_normal;
		vs_position = vec4(position, 1.0f).xyz;
	}
	else {
		vs_normal = transpose(inverse(mat3(world_transform))) * vertex_normal;
		vs_position = (world_transform * vec4(position, 1.0f)).xyz;
	}

	gl_Position = PVMatrices.proj_view * vec4(vs_position, 1);

}

