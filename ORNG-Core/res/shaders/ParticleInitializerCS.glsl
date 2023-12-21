#version 460 core

#ifdef EMITTER_DELETE_DECREMENT_EMITTERS
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
uniform uint u_num_emitters;

#else
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
#endif

#define FIRST_INITIALIZATION
uniform uint u_start_index;
uniform uint u_emitter_index;
ORNG_INCLUDE "ParticleBuffersINCL.glsl"


// This function runs just before the memory for the emitter is deleted and the buffers are restructured
void OnEmitterDelete(uint start_index, uint num_emitters) {
    #ifdef EMITTER_DELETE_DECREMENT_EMITTERS
    for (uint i = start_index + 1; i < num_emitters; i++) {
        ssbo_particle_emitters.emitters[i].start_index -= ssbo_particle_emitters.emitters[start_index].num_particles;
    }

    #else
    ssbo_particles.particles[start_index + gl_GlobalInvocationID.x].emitter_index--;
    #endif
}

void main() {
    
    #ifdef EMITTER_DELETE_DECREMENT_EMITTERS
    OnEmitterDelete(u_emitter_index, u_num_emitters);
    #elif defined EMITTER_DELETE_DECREMENT_PARTICLES
    OnEmitterDelete(u_start_index, 0);
    #elif defined INITIALIZE_AS_DEAD
    ssbo_particles.particles[u_start_index + gl_GlobalInvocationID.x].flags = 0;
    #else
    ssbo_particles.particles[u_start_index + gl_GlobalInvocationID.x].emitter_index = u_emitter_index;
    InitializeParticle(u_start_index + gl_GlobalInvocationID.x);
    #endif
}