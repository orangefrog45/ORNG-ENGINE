#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define PARTICLES_DETACHED


ORNG_INCLUDE "ParticleBuffersINCL.glsl"

uniform uint u_buffer_index;



void main() {
    if (ssbo_particle_append.particles[gl_GlobalInvocationID.x].emitter_index == u_buffer_index) {
        
        ssbo_particles_detached.particles[atomicAdd(ssbo_particles_detached.current_index, 1) % ssbo_particles_detached.particles.length()] = ssbo_particle_append.particles[gl_GlobalInvocationID.x];
    }
}