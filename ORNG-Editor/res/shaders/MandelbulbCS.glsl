#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // invocations
layout(binding = 1, rgba16f) writeonly uniform image2D output_tex;
layout(binding = 2, rgba16f) writeonly uniform image2D normals;
layout(binding = 3, rgba16f) writeonly uniform image2D albedo;
layout(binding = 4, rgba16f) writeonly uniform image2D roughness_metallic_ao;
layout(binding = 5, r16ui) writeonly uniform uimage2D shader_ids;
layout(binding = 6, r32f) writeonly uniform image2D depth_image;
layout(binding = 16) uniform sampler2D depth_sampler;


layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	float time_elapsed;
	float render_resolution_x;
	float render_resolution_y;
	float cam_zfar;
	float cam_znear;
} ubo_common;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
	mat4 inv_projection;
	mat4 inv_view;
} PVMatrices;


vec3 GetWorldSpacePos(vec2 tex_coords, float depth) {
	vec2 normalized_tex_coords = tex_coords / (vec2(imageSize(output_tex)));
	vec4 clipSpacePosition = vec4(normalized_tex_coords, depth, 1.0) * 2.0 - 1.0;
	vec4 viewSpacePosition = PVMatrices.inv_projection * clipSpacePosition;
	vec4 worldSpacePosition = PVMatrices.inv_view * viewSpacePosition;
	// Perspective division
	worldSpacePosition.xyz /= worldSpacePosition.w;
	return worldSpacePosition.xyz;
}



/*float map(vec3 pos) {
	float dz = 1.0;
	float length2 = dot(pos, pos);
	vec3 w = pos;

	for (int i = 0; i < 4; i++) {
		dz = 8.0 * pow(length2, 3.5) * dz + 1.0;

		// extract polar coordinates
		float r = length(w);
		float b = 8.0 * acos(w.y / r);
		float a = 8.0 * atan(w.x, w.z);
		//float b = (7.0 + 1 * sin(ubo_common.time_elapsed * 0.01)) * acos(w.y / r);
		//float a = (7.0 + 1 * cos(ubo_common.time_elapsed * 0.01)) * atan(w.x, w.z);
		w = pos + pow(r, 8.0) * vec3(sin(b) * sin(a), cos(b), sin(b) * cos(a));



		length2 = dot(w, w);
		if (length2 > 256)
			break;
	}

	// distance estimator
	return 0.25 * log(length2) * sqrt(length2) / dz;
}*/


float map(vec3 p) {
	vec3 CSize = vec3(1., 1., 1.3);
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 12; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p,p+sin(p.z*.3)); //Alternate fractal
		float k = max((2.) / (r2), .027);
		p *= k;
		scale *= k;
	}
	float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z;
	rxy = max(rxy, -(n) / 4.);
	return (rxy) / abs(scale);
}

vec3 calcNormal(in vec3 pos, float t)
{
	float precis = 0.001 * t;

	vec2 e = vec2(1.0, -1.0) * precis;
	return normalize(e.xyy * map(pos + e.xyy) +
		e.yyx * map(pos + e.yyx) +
		e.yxy * map(pos + e.yxy) +
		e.xxx * map(pos + e.xxx));
}


void main() {
	ivec2 tex_coords = ivec2(gl_GlobalInvocationID.xy);

	float depth = texelFetch(depth_sampler, tex_coords, 0).r;
	vec3 world_pos = GetWorldSpacePos(tex_coords, depth);
	vec3 march_dir = normalize(GetWorldSpacePos(tex_coords, depth) - ubo_common.camera_pos.xyz);
	vec3 step_pos = ubo_common.camera_pos.xyz;

	float max_dist = length(world_pos - ubo_common.camera_pos.xyz);
	float t = 0.01;
	float d = 0.01;
	vec3 r;
	int i = 0;
	for (i; i < 256; i++) {
		d = map(step_pos);
		step_pos += d * 0.5 * march_dir;
		t += d;
		if (d < 0.001 * t) {
			vec3 norm = calcNormal(step_pos, t);
			imageStore(albedo, tex_coords, vec4(1, 1, 1, 1.0));
			imageStore(normals, tex_coords, vec4(normalize(norm) / (i / 512.0), 1.0));
			vec4 clip = PVMatrices.view * vec4(step_pos, 1.0);
			vec4 proj = PVMatrices.projection * clip;
			proj.z /= proj.w;
			imageStore(depth_image, tex_coords, vec4(proj.z));
			imageStore(shader_ids, tex_coords, uvec4(1));
			imageStore(roughness_metallic_ao, tex_coords, vec4(0.1, 0.9, 0.2, 1.0));

			break;
		}

		if (length(ubo_common.camera_pos.xyz - step_pos) > max_dist) {
			break;
		}
	}
}