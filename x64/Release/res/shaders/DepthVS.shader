#version 430 core

in layout(location = 0) vec3 pos;

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

uniform mat4 u_light_pv_matrix;
uniform bool u_terrain_mode;

void main() {
	if (u_terrain_mode) {
		gl_Position = u_light_pv_matrix * vec4(pos, 1.0);
	}
	else {
		gl_Position = u_light_pv_matrix * transform_ssbo.transforms[gl_InstanceID] * vec4(pos, 1.0);
	}
}


