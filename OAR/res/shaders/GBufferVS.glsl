#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 7) vec3 in_tangent;



const unsigned int NUM_SHADOW_CASCADES = 3;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
} PVMatrices;

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
} ubo_common;



out vec4 vs_position;
out vec3 vs_normal;
out vec3 vs_tex_coord;
out vec3 vs_tangent;
out mat4 vs_transform;
out vec3 vs_original_normal;
out vec3 vs_view_dir_tangent_space;

uniform bool u_terrain_mode;
uniform bool u_skybox_mode;
uniform uint u_material_id;


mat3 CalculateTbnMatrixTransform() {
	vec3 t = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * in_tangent));
	vec3 n = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * vertex_normal));

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = transpose(mat3(t, b, n));

	return tbn;
}


mat3 CalculateTbnMatrix() {
	vec3 t = normalize(in_tangent);
	vec3 n = normalize(vertex_normal);

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}



void main() {

	vs_tangent = in_tangent;
	vs_original_normal = vertex_normal;

	if (u_terrain_mode) {

		vs_tex_coord = vec3(tex_coord, 0.f);
		vs_normal = vertex_normal;
		vs_position = vec4(position, 1.f);
		gl_Position = PVMatrices.proj_view * vs_position;

		mat3 tbn = CalculateTbnMatrix();
		vs_view_dir_tangent_space = tbn * (ubo_common.camera_pos.xyz - vs_position.xyz);

	}
	else if (u_skybox_mode)
	{
		vec4 view_pos = vec4((mat3(PVMatrices.view) * position), 1.0);
		vec4 proj_pos = PVMatrices.projection * view_pos;
		vs_tex_coord = position;
		vs_position = vec4(position, 1.f);

		gl_Position = proj_pos.xyww;
	}
	else {

		vs_tex_coord = vec3(tex_coord, 0.f);
		vs_transform = transform_ssbo.transforms[gl_InstanceID];
		vs_normal = transpose(inverse(mat3(vs_transform))) * vertex_normal;
		vs_position = vs_transform * vec4(position, 1.0f);

		mat3 tbn = CalculateTbnMatrixTransform();
		vs_view_dir_tangent_space = tbn * (ubo_common.camera_pos.xyz - vs_position.xyz);


		gl_Position = PVMatrices.proj_view * vs_position;
	}


}

