#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "ParticleBuffersINCL.glsl"



void main() {
    for (uint i = gl_GlobalInvocationID.x * 4; i < gl_GlobalInvocationID.x * 4 + 4; i++) {
        ubo_particles.particles[i].velocity_life.w -= ubo_common.delta_time;

        if (ubo_particles.particles[i].velocity_life.w <= 0.0) {
            InitializeParticle(i);
        }

        // Update positions with velocity
        ubo_particle_transforms.transforms[i][3][0] += ubo_particles.particles[i].velocity_life.x * ubo_common.delta_time * 0.001;
        ubo_particle_transforms.transforms[i][3][1] += ubo_particles.particles[i].velocity_life.y * ubo_common.delta_time * 0.001;
        ubo_particle_transforms.transforms[i][3][2] += ubo_particles.particles[i].velocity_life.z * ubo_common.delta_time * 0.001;
    }

}