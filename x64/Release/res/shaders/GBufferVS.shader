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


out vec3 vs_position;
out vec3 vs_normal;
out vec3 vs_tex_coord;
out vec3 vs_tangent;
out mat4 vs_transform;

uniform bool u_terrain_mode;
uniform bool u_skybox_mode;
uniform uint u_material_id;


void main() {

	vs_tangent = in_tangent;

	if (u_terrain_mode) {
		vs_tex_coord = vec3(tex_coord, 0.f);
		vs_normal = vertex_normal;
		vs_position = position;
		gl_Position = PVMatrices.proj_view * vec4(vs_position, 1);

	}
	else if (u_skybox_mode)
	{
		vec4 view_pos = vec4((mat3(PVMatrices.view) * position), 1.0);
		vec4 proj_pos = PVMatrices.projection * view_pos;
		vs_tex_coord = position;
		vec3 camera_pos = -vec3(PVMatrices.view[0][3], PVMatrices.view[1][3], PVMatrices.view[2][3]);
		vs_position = camera_pos + position * 5000.f; //just set skybox pos very far away and move with camera, should give correct illusion (currently used for fog)
		gl_Position = proj_pos.xyww;
	}
	else {
		vs_tex_coord = vec3(tex_coord, 0.f);
		vs_transform = transform_ssbo.transforms[gl_InstanceID];
		vs_normal = transpose(inverse(mat3(vs_transform))) * vertex_normal; // inverse/transpose to undo unwanted transforms to normal
		vs_position = (vs_transform * vec4(position, 1.0f)).xyz;
		gl_Position = PVMatrices.proj_view * vec4(vs_position, 1);
	}


}

