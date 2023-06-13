#version 430 core

in layout(location = 0) vec3 pos;
in layout(location = 2) vec3 normal;

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

uniform mat4 u_light_pv_matrix;
uniform bool u_terrain_mode;

out vec3 vs_normal;
out vec3 world_pos;

void main() {
	if (u_terrain_mode) {
		vec4 light_pos = u_light_pv_matrix * vec4(pos, 1.0);
		gl_Position = light_pos;
		vs_normal = normal;
	}
	else {
		vs_normal = transpose(inverse(mat3(transform_ssbo.transforms[gl_InstanceID]))) * normal;
		vec4 light_pos = u_light_pv_matrix * transform_ssbo.transforms[gl_InstanceID] * vec4(pos, 1.0);
		gl_Position = light_pos;
	}
	world_pos = pos;
}


