#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec2 tex_coord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) vec3 in_tangent;

ORNG_INCLUDE "BuffersINCL.glsl"

ORNG_INCLUDE "ParticleBuffersINCL.glsl"

ORNG_INCLUDE "CommonINCL.glsl"

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;



out vec4 vs_position;
out vec3 vs_normal;
out vec3 vs_tex_coord;
out vec3 vs_tangent;
out mat4 vs_transform;
out vec3 vs_original_normal;
out vec3 vs_view_dir_tangent_space;

#ifdef NO_TRANSFORM_BUFFERS
uniform mat4 u_transform;
#endif

#ifdef PARTICLE
uniform uint u_transform_start_index;
flat out ParticleTransform vs_particle_transform;
flat out uint vs_particle_index;
#endif

uniform Material u_material;

mat3 CalculateTbnMatrixTransform() {

#ifdef PARTICLE
	vec3 t = normalize(qtransform(ubo_particle_transforms.transforms[u_transform_start_index + gl_InstanceID].quat, in_tangent));
	vec3 n = normalize(qtransform(ubo_particle_transforms.transforms[u_transform_start_index + gl_InstanceID].quat, vertex_normal));
#else
	vec3 t = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * in_tangent));
	vec3 n = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * vertex_normal));
	#endif

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

#ifdef TERRAIN_MODE

		vs_tex_coord = vec3(tex_coord, 0.f);
		vs_normal = vertex_normal;
		vs_position = vec4(position, 1.f);
		gl_Position = PVMatrices.proj_view * vs_position;
		mat3 tbn = CalculateTbnMatrix();
		vs_view_dir_tangent_space = tbn * (ubo_common.camera_pos.xyz - vs_position.xyz);

#elif defined SKYBOX_MODE
		vs_position = vec4(position, 0.0);
		vec4 view_pos = vec4(mat3(PVMatrices.view) * vs_position.xyz, 1.0);
		vec4 proj_pos = PVMatrices.projection * view_pos;
		vs_tex_coord = position;
		gl_Position = proj_pos.xyww;
#else
		vs_tex_coord = vec3(tex_coord, 0.f);

		if (u_material.using_spritesheet) {
			vec2 step_size = vec2(1.0) / vec2(u_material.sprite_data.num_cols, u_material.sprite_data.num_rows);
			vs_tex_coord.xy *= step_size;
			
			uint index = uint(float(u_material.sprite_data.num_cols * u_material.sprite_data.num_rows) * (float(uint(ubo_common.time_elapsed) % 1000) / 1000.0)); 
			uint row = (u_material.sprite_data.num_cols * u_material.sprite_data.num_rows - index - 1) / u_material.sprite_data.num_cols;
			uint col = index - (index / u_material.sprite_data.num_cols) * u_material.sprite_data.num_cols;

			vs_tex_coord.x += step_size.x * float(col);
			vs_tex_coord.y += step_size.y * float(row);
			}


		#if defined PARTICLE && defined BILLBOARD

			#define EMITTER ubo_particle_emitters.emitters[ubo_particles.particles[vs_particle_index].emitter_index]
			#define PTCL ubo_particles.particles[vs_particle_index]
			vs_particle_index = u_transform_start_index + gl_InstanceID;

			float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan;
			vec3 interpolated_scale = InterpolateV3(interpolation, EMITTER.scale_over_life);

			ParticleTransform t = ubo_particle_transforms.transforms[vs_particle_index];
			vec3 t_pos = t.pos.xyz;
			vec3 cam_up = vec3(PVMatrices.view[0][1], PVMatrices.view[1][1], PVMatrices.view[2][1]);
			vec3 cam_right = vec3(PVMatrices.view[0][0], PVMatrices.view[1][0], PVMatrices.view[2][0]);
			vs_particle_transform = t;
			

			vs_position = vec4(t_pos + position.x * cam_right * t.scale.x * interpolated_scale.x + position.y * cam_up * t.scale.y * interpolated_scale.y, 1.0);
			vs_normal = qtransform(t.quat, vertex_normal);

		#elif defined NO_TRANSFORM_BUFFERS
			vs_transform = u_transform;
		#elif defined PARTICLE
			vs_particle_index = u_transform_start_index + gl_InstanceID;
			ParticleTransform t = ubo_particle_transforms.transforms[vs_particle_index];
			vs_position = vec4(qtransform(t.quat, (position * t.scale.xyz)) + t.pos.xyz, 1.0);
			vs_normal = qtransform(t.quat, vertex_normal);
			vs_particle_transform = t;

		#elif defined BILLBOARD
			vec3 t_pos = vec3(transform_ssbo.transforms[gl_InstanceID][3][0], transform_ssbo.transforms[gl_InstanceID][3][1], transform_ssbo.transforms[gl_InstanceID][3][2]);
			vec3 cam_up = vec3(PVMatrices.view[0][1], PVMatrices.view[1][1], PVMatrices.view[2][1]);
			vec3 cam_right = vec3(PVMatrices.view[0][0], PVMatrices.view[1][0], PVMatrices.view[2][0]);

			vs_position = vec4(t_pos + position.x * cam_right * transform_ssbo.transforms[gl_InstanceID][0][0] + position.y * cam_up * transform_ssbo.transforms[gl_InstanceID][1][1], 1.0);
		#else
			vs_transform = transform_ssbo.transforms[gl_InstanceID];
			vs_position = vs_transform * (vec4(position, 1.0f));
			vs_normal = transpose(inverse(mat3(vs_transform))) * vertex_normal;

		#endif

		
		#ifndef BILLBOARD
		mat3 tbn = CalculateTbnMatrixTransform();
		vs_view_dir_tangent_space = tbn * (ubo_common.camera_pos.xyz - vs_position.xyz);
		#endif

		gl_Position = PVMatrices.proj_view * vs_position;
#endif
}
