ORNG_INCLUDE "UtilINCL.glsl"
ORNG_INCLUDE "CommonINCL.glsl"
#define VELOCITY_SCALE 35.0

// Emitters both manage the memory for particles as well as the update/render functionality
// The update/render functionality can be overidden in a custom layer
// EmitParticle() does not work with emitters
struct ParticleEmitter {
    vec4 pos;

    uint start_index;
    uint num_particles;
    float spread;
    float vel_scale_min;

    mat4 rotation_matrix;
    vec4 spawn_extents_vel_scale_max;

    float lifespan;
    float spawn_cooldown;
    int is_active;
    float pad1;

    InterpolatorV3 colour_over_life;
    InterpolatorV3 scale_over_life;
    InterpolatorV1 alpha_over_life;

    vec4 acceleration;


};

struct ParticleBufferData {
    uint buffer_id;


};


struct Particle {
    vec4 pos;
    vec4 quat;
    vec4 scale;

    vec4 velocity_life;
    uint emitter_index;
    uint flags;
};



// CHANGE NAMES TO SSBO
layout(std140, binding = 5) buffer ParticleEmitters {
    ParticleEmitter emitters[];
} ssbo_particle_emitters;



#ifndef PARTICLES_DETACHED

layout(std140, binding = 6) buffer Particles {
    Particle particles[];
} ssbo_particles;

#define PARTICLE_SSBO ssbo_particles

#else

layout(std140, binding = 6) buffer ParticlesDetached {
    uint current_index;
    
    // General data that can be used by an implementation for anything, will not be modified by the engine
    vec4 data[8];
    int data_int[8];

    Particle particles[];
} ssbo_particles_detached;

#define PARTICLE_SSBO ssbo_particles_detached

#endif

// Used for copy operations
layout(std140, binding = 7) buffer SecondaryParticles {
    uint current_index;
    Particle particles[];
} ssbo_secondary_particles;



layout(std140, binding = 7) buffer ParticleAppend {
    coherent uint num_particles_appended;
    Particle particles[];
} ssbo_particle_append;

#ifdef PARTICLES_DETACHED

// Only use if particle buffer to emit to is currently bound
// Particle buffers treated as ring buffers, particles might get overwritten
void EmitParticleDirect(Particle p) {
    ssbo_particles_detached.particles[atomicAdd(ssbo_particles_detached.current_index, 1) % ssbo_particles_detached.particles.length()] = p;
}

#endif

// Slower, use if particle buffer is not bound, requires copying
// Particle buffers treated as ring buffers, particles might get overwritten
void EmitParticleAppend(Particle p, uint buffer_index) {
    p.emitter_index = buffer_index;
    ssbo_particle_append.particles[atomicAdd(ssbo_particle_append.num_particles_appended, 1)] = p;
}





 vec3 qtransform( vec4 q, vec3 v ){ 
	return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
} 


#ifndef PARTICLES_DETACHED

#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[index].emitter_index]

void InitializeParticle(uint index) {

    float az = rnd(vec2(index,rnd(vec2(PI, index)))) * 2.0 * PI;
    float pol = rnd(vec2(PI,rnd(vec2(index,PI)))) * EMITTER.spread;

    float rnd_vel = rnd(vec2(index, pol)) * (EMITTER.spawn_extents_vel_scale_max.w - EMITTER.vel_scale_min) + EMITTER.vel_scale_min;
    vec3 vel = mat3(EMITTER.rotation_matrix) * (normalize(vec3( sin(pol) * cos(az), sin(pol) * sin(az), cos(pol))) * abs(rnd_vel));

    vec3 spawn_pos = EMITTER.pos.xyz + mat3(EMITTER.rotation_matrix) * (vec3(rnd(vec2(index, ubo_common.delta_time)), rnd(vec2(az, index)), rnd(vec2(ssbo_particles.particles[index].pos.x, ubo_common.delta_time))) *  EMITTER.spawn_extents_vel_scale_max.xyz);


    #ifdef FIRST_INITIALIZATION
        // Give initial offset to life to factor in spawn delay
        ssbo_particles.particles[index].velocity_life = vec4(vel, (index - EMITTER.start_index) * EMITTER.spawn_cooldown);

        ssbo_particles.particles[index].scale = vec4(0);
    #else
        ssbo_particles.particles[index].velocity_life.xyz = vel;
        ssbo_particles.particles[index].velocity_life.w += EMITTER.lifespan;

        ssbo_particles.particles[index].scale = vec4(1);

    #endif


    ssbo_particles.particles[index].pos.xyz = spawn_pos;

}

#undef EMITTER

#endif // ifndef PARTICLES_DETACHED


