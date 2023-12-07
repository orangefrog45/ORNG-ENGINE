#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"



void main() {
    for (uint i = gl_GlobalInvocationID.x * 4; i < gl_GlobalInvocationID.x * 4 + 4; i++) {
        ubo_particles.particles[i].velocity_life.w -= ubo_common.delta_time;

        if (ubo_particles.particles[i].velocity_life.w <= 0.0 && bool(ubo_particle_emitters.emitters[ubo_particles.particles[i].emitter_index].is_active)) {
            InitializeParticle(i);
        }

        ubo_particles.particles[i].velocity_life.xyz += ubo_particle_emitters.emitters[ubo_particles.particles[i].emitter_index].acceleration.xyz  * ubo_common.delta_time * 0.001; 

        // Update positions with velocity
        ubo_particle_transforms.transforms[i].pos.xyz += ubo_particles.particles[i].velocity_life.xyz * ubo_common.delta_time * 0.001;

    }

}