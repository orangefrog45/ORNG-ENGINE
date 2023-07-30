R""(
	#version 430 core

	in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) vec3 in_tangent;



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
	float time_elapsed;
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


vec2 fade(vec2 t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
vec4 permute(vec4 x) { return mod(((x * 34.0) + 1.0) * x, 289.0); }
float cnoise(vec2 P) {
	vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
	vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
	Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
	vec4 ix = Pi.xzxz;
	vec4 iy = Pi.yyww;
	vec4 fx = Pf.xzxz;
	vec4 fy = Pf.yyww;
	vec4 i = permute(permute(ix) + iy);
	vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
	vec4 gy = abs(gx) - 0.5;
	vec4 tx = floor(gx + 0.5);
	gx = gx - tx;
	vec2 g00 = vec2(gx.x, gy.x);
	vec2 g10 = vec2(gx.y, gy.y);
	vec2 g01 = vec2(gx.z, gy.z);
	vec2 g11 = vec2(gx.w, gy.w);
	vec4 norm = 1.79284291400159 - 0.85373472095314 *
		vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
	g00 *= norm.x;
	g01 *= norm.y;
	g10 *= norm.z;
	g11 *= norm.w;
	float n00 = dot(g00, vec2(fx.x, fy.x));
	float n10 = dot(g10, vec2(fx.y, fy.y));
	float n01 = dot(g01, vec2(fx.z, fy.z));
	float n11 = dot(g11, vec2(fx.w, fy.w));
	vec2 fade_xy = fade(Pf.xy);
	vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
	float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
	return 2.3 * n_xy;
}

mat3 CalculateTbnMatrixTransform() {
	vec3 t = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * in_tangent));
	vec3 n = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * vertex_normal));

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

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
		vs_position = vec4(position + vec3(0.0, cnoise(position.xz * 0.01 + ubo_common.time_elapsed), 0.0) * 10.f, 1.f);
		gl_Position = PVMatrices.proj_view * vs_position;

		mat3 tbn = CalculateTbnMatrix();
		vs_view_dir_tangent_space = tbn * (ubo_common.camera_pos.xyz - vs_position.xyz);

	}
	else if (u_skybox_mode)
	{
		vec4 view_pos = vec4(mat3(PVMatrices.view) * position, 1.0);
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
)""