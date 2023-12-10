#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"


#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[i].emitter_index]
#define PTCL ssbo_particles.particles[i]

    
void main() {
    for (uint i = gl_GlobalInvocationID.x * 4; i < gl_GlobalInvocationID.x * 4 + 4; i++) {
        ssbo_particles.particles[i].velocity_life.w -= ubo_common.delta_time;

        if (ssbo_particles.particles[i].velocity_life.w <= 0.0 && bool(ssbo_particle_emitters.emitters[ssbo_particles.particles[i].emitter_index].is_active)) {
            InitializeParticle(i);
        }

	    float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan;


        //ssbo_particles.particles[i].velocity_life.xyz = InterpolateV3(interpolation, EMITTER.velocity_over_life) ;

        ssbo_particles.particles[i].velocity_life.xyz += ssbo_particle_emitters.emitters[ssbo_particles.particles[i].emitter_index].acceleration.xyz  * ubo_common.delta_time * 0.001; 
        //ssbo_particles.particles[i].velocity_life.xyz += ssbo_particle_emitters.emitters[ssbo_particles.particles[i].emitter_index].acceleration.xyz  * (EMITTER.lifespan - PTCL.velocity_life.w) * 0.001; 

        // Update positions with velocity
        ssbo_particle_transforms.transforms[i].pos.xyz += ssbo_particles.particles[i].velocity_life.xyz * ubo_common.delta_time * 0.001;

    }

}