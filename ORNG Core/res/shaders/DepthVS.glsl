R""(#version 430 core

in layout(location = 0) vec3 pos;
in layout(location = 2) vec3 normal;

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

uniform mat4 u_light_pv_matrix;

out vec3 vs_normal;
out vec3 world_pos;

void main() {
	vs_normal = transpose(inverse(mat3(transform_ssbo.transforms[gl_InstanceID]))) * normal;
	vec4 world_transformed_pos = transform_ssbo.transforms[gl_InstanceID] * vec4(pos, 1.0);
	vec4 light_pos = u_light_pv_matrix * world_transformed_pos;
	gl_Position = light_pos;
	world_pos = world_transformed_pos.xyz;
}


)""