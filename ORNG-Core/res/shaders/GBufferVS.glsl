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


#ifdef UNIFORM_TRANSFORM
uniform mat4 u_transform;
#endif


out VSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} vert_data;

#ifdef TESSELLATE
out int vs_instance_id;
#endif

out flat mat4 vs_transform;

#ifdef PARTICLE
uniform uint u_transform_start_index;
flat out uint vs_particle_index;
#endif

#ifdef VOXELIZE
uniform mat4 u_orth_proj_view_matrix;
uniform vec3 u_aligned_camera_pos;
#endif

uniform Material u_material;

mat3 CalculateTbnMatrixTransform() {

#ifdef PARTICLE
	vec3 t = normalize(qtransform(PARTICLE_SSBO.particles[u_transform_start_index + gl_InstanceID].quat, in_tangent));
	vec3 n = normalize(qtransform(PARTICLE_SSBO.particles[u_transform_start_index + gl_InstanceID].quat, vertex_normal));
#else
	vec3 t = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * in_tangent));
	vec3 n = normalize(vec3(mat3(transform_ssbo.transforms[gl_InstanceID]) * vertex_normal));
	#endif

	t = normalize(t - dot(t, n) * n);
	vec3 b = normalize(cross(n, t));

	mat3 tbn = mat3(t, b, n);

	return transpose(tbn);
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
#ifdef TESSELLATE
	vs_instance_id = gl_InstanceID;
#endif

	vert_data.tangent = in_tangent;
	vert_data.original_normal = vertex_normal;

#ifdef TERRAIN_MODE

		vert_data.tex_coord = tex_coord;
		vert_data.normal = vertex_normal;
		vert_data.position = vec4(position, 1.f);
		gl_Position = PVMatrices.proj_view * vert_data.position;

#elif defined SKYBOX_MODE
		vert_data.position = vec4(position, 0.0);
		vec4 view_pos = vec4(mat3(PVMatrices.view) * vert_data.position.xyz, 1.0);
		vec4 proj_pos = PVMatrices.projection * view_pos;
		gl_Position = proj_pos.xyww;
#else
		#ifdef PARTICLE
				#define PTCL PARTICLE_SSBO.particles[vs_particle_index]
				#ifndef PARTICLES_DETACHED
					vs_particle_index = u_transform_start_index + gl_InstanceID;
					#define EMITTER ssbo_particle_emitters.emitters[PARTICLE_SSBO.particles[vs_particle_index].emitter_index]
					float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan;
					vec3 interpolated_scale = InterpolateV3(interpolation, EMITTER.scale_over_life);
				#else
					vs_particle_index = gl_InstanceID;
				#endif
		#endif

		vert_data.tex_coord = tex_coord;
		if (bool(u_material.flags & MAT_FLAG_SPRITESHEET))
		{
			vec2 step_size = vec2(1.0) / vec2(u_material.sprite_data.num_cols, u_material.sprite_data.num_rows);
			vert_data.tex_coord.xy *= step_size;

			#if defined PARTICLE && !defined(PARTICLES_DETACHED)
				uint index = uint(float(u_material.sprite_data.num_cols * u_material.sprite_data.num_rows) * interpolation); 
			#else
				uint index = uint(float(u_material.sprite_data.num_cols * u_material.sprite_data.num_rows) * (float(uint(ubo_common.time_elapsed) % 1000) / 1000.0)); 
			#endif

			uint row = (u_material.sprite_data.num_cols * u_material.sprite_data.num_rows - index - 1) / u_material.sprite_data.num_cols;
			uint col = index - (index / u_material.sprite_data.num_cols) * u_material.sprite_data.num_cols;

			vert_data.tex_coord.x += step_size.x * float(col);
			vert_data.tex_coord.y += step_size.y * float(row);
		}



		#if defined PARTICLE && defined BILLBOARD
			#ifndef PARTICLES_DETACHED
			#endif

			#define TRANSFORM PARTICLE_SSBO.particles[vs_particle_index]

			vec3 t_pos = TRANSFORM.pos.xyz;
			vec3 cam_up = vec3(PVMatrices.view[0][1], PVMatrices.view[1][1], PVMatrices.view[2][1]);
			vec3 cam_right = vec3(PVMatrices.view[0][0], PVMatrices.view[1][0], PVMatrices.view[2][0]);
			
			#ifndef PARTICLES_DETACHED
				vert_data.position = vec4(t_pos + position.x * cam_right * TRANSFORM.scale.x * interpolated_scale.x + position.y * cam_up * TRANSFORM.scale.y * interpolated_scale.y, 1.0);
			#else
				vert_data.position = vec4(t_pos + position.x * cam_right * TRANSFORM.scale.x + position.y * cam_up * TRANSFORM.scale.y, 1.0);
			#endif

			#undef TRANSFORM

		#elif defined PARTICLE

			#ifdef PARTICLES_DETACHED
				vs_particle_index = gl_InstanceID;
				#define TRANSFORM ssbo_particles_detached.particles[vs_particle_index]
			#else
				vs_particle_index = u_transform_start_index + gl_InstanceID;
				#define TRANSFORM ssbo_particles.particles[vs_particle_index]
			#endif

			vert_data.position = vec4(qtransform(TRANSFORM.quat, (interpolated_scale * position * TRANSFORM.scale.xyz)) + TRANSFORM.pos.xyz, 1.0);
			vert_data.normal = qtransform(vec4(TRANSFORM.quat.xyz, 1.0), vertex_normal);

			#undef TRANSFORM

		#elif defined BILLBOARD
			vec3 t_pos = vec3(transform_ssbo.transforms[gl_InstanceID][3][0], transform_ssbo.transforms[gl_InstanceID][3][1], transform_ssbo.transforms[gl_InstanceID][3][2]);
			vec3 cam_up = vec3(PVMatrices.view[0][1], PVMatrices.view[1][1], PVMatrices.view[2][1]);
			vec3 cam_right = vec3(PVMatrices.view[0][0], PVMatrices.view[1][0], PVMatrices.view[2][0]);

			vert_data.position = vec4(t_pos + position.x * cam_right * transform_ssbo.transforms[gl_InstanceID][0][0] + position.y * cam_up * transform_ssbo.transforms[gl_InstanceID][1][1], 1.0);

		#else
			#ifdef UNIFORM_TRANSFORM
			vs_transform = u_transform;
			#else
			vs_transform = transform_ssbo.transforms[gl_InstanceID];
			#endif
			
			vert_data.position = vs_transform * (vec4(position, 1.0f));
			vert_data.normal = transpose(inverse(mat3(vs_transform))) * vertex_normal;
		#endif




		#ifdef BILLBOARD
			vert_data.normal = ubo_common.camera_pos.xyz - t_pos;
		#endif
		
		#ifndef BILLBOARD
			mat3 tbn = CalculateTbnMatrixTransform();
			vert_data.view_dir_tangent_space = tbn * ( ubo_common.camera_pos.xyz - vert_data.position.xyz);
		#endif

		#ifdef VOXELIZE
			gl_Position = u_orth_proj_view_matrix * vec4(vert_data.position.xyz, 1);
		#else
			#ifdef TESSELLATE
			gl_Position = vert_data.position;
			#else
			gl_Position = PVMatrices.proj_view * vert_data.position;
			#endif
		#endif
#endif
}