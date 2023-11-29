#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define FIRST_INITIALIZATION
ORNG_INCLUDE "ParticleBuffersINCL.glsl"


uniform uint u_start_index;
uniform uint u_emitter_index;


void main() {
    #ifdef EMITTER_DELETION
    OnEmitterDelete(u_emitter_index);
    #else
    ubo_particles.particles[u_start_index + gl_GlobalInvocationID.x].emitter_index = u_emitter_index;
    InitializeParticle(u_start_index + gl_GlobalInvocationID.x);
    #endif
}