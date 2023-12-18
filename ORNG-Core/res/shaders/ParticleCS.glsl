#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"


#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[ gl_GlobalInvocationID.x].emitter_index]
#define PTCL ssbo_particles.particles[ gl_GlobalInvocationID.x]

    
void main() {
        ssbo_particles.particles[ gl_GlobalInvocationID.x].velocity_life.w -= ubo_common.delta_time;

        if (ssbo_particles.particles[ gl_GlobalInvocationID.x].velocity_life.w <= 0.0 && bool(ssbo_particle_emitters.emitters[ssbo_particles.particles[ gl_GlobalInvocationID.x].emitter_index].is_active)) {
            InitializeParticle( gl_GlobalInvocationID.x);
        }

	    float interpolation = 1.0 - clamp(PTCL.velocity_life.w, 0.0, EMITTER.lifespan) / EMITTER.lifespan + float(ssbo_particle_append.num_particles_appended) * 0.000001 ;

        //ssbo_particles.particles[i].velocity_life.xyz = InterpolateV3(interpolation, EMITTER.velocity_over_life) ;

        ssbo_particles.particles[ gl_GlobalInvocationID.x].velocity_life.xyz += ssbo_particle_emitters.emitters[ssbo_particles.particles[ gl_GlobalInvocationID.x].emitter_index].acceleration.xyz  * ubo_common.delta_time * 0.001; 
        //ssbo_particles.particles[i].velocity_life.xyz += ssbo_particle_emitters.emitters[ssbo_particles.particles[i].emitter_index].acceleration.xyz  * (EMITTER.lifespan - PTCL.velocity_life.w) * 0.001; 

        // Update positions with velocity
        ssbo_particles.particles[ gl_GlobalInvocationID.x].pos.xyz += ssbo_particles.particles[ gl_GlobalInvocationID.x].velocity_life.xyz * ubo_common.delta_time * 0.001;


}