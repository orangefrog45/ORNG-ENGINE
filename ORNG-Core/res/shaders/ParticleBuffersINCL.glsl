ORNG_INCLUDE "UtilINCL.glsl"
ORNG_INCLUDE "CommonINCL.glsl"
#define VELOCITY_SCALE 35.0

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
    InterpolatorV3 velocity_over_life;


    vec4 acceleration;


};

struct Particle {
    vec4 velocity_life;
    uint emitter_index;
};

struct ParticleTransform {
    vec4 pos;
    vec4 quat;
    vec4 scale;
};

// CHANGE NAMES TO SSBO
layout(std140, binding = 5) buffer ParticleEmitters {
    ParticleEmitter emitters[];
} ssbo_particle_emitters;

layout(std140, binding = 6) buffer ParticleTransforms {
    ParticleTransform transforms[];
} ssbo_particle_transforms;

layout(std140, binding = 7) buffer Particles {
    Particle particles[];
} ssbo_particles;

 vec3 qtransform( vec4 q, vec3 v ){ 
	return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
} 



#define EMITTER ssbo_particle_emitters.emitters[ssbo_particles.particles[index].emitter_index]

void InitializeParticle(highp uint index) {

    float az = rnd(vec2(ssbo_particle_transforms.transforms[index].pos.z, ssbo_particle_transforms.transforms[index].pos.x)) * 2.0 * PI;
    float pol = rnd(vec2(ssbo_particle_transforms.transforms[index].pos.y, -ssbo_particle_transforms.transforms[index].pos.z)) * EMITTER.spread;

    float rnd_vel = rnd(vec2(index, pol)) * (EMITTER.spawn_extents_vel_scale_max.w - EMITTER.vel_scale_min) + EMITTER.vel_scale_min;
    vec3 vel = mat3(EMITTER.rotation_matrix) * (normalize(vec3( sin(pol) * cos(az), sin(pol) * sin(az), cos(pol))) * abs(rnd_vel));

    vec3 spawn_pos = EMITTER.pos.xyz + mat3(EMITTER.rotation_matrix) * (vec3(rnd(vec2(index, ubo_common.delta_time)), rnd(vec2(az, index)), rnd(vec2(ssbo_particle_transforms.transforms[index].pos.x, ubo_common.delta_time))) *  EMITTER.spawn_extents_vel_scale_max.xyz);


    #ifdef FIRST_INITIALIZATION
        // Give initial offset to life to factor in spawn delay
        ssbo_particles.particles[index].velocity_life = vec4(vel, (index - EMITTER.start_index) * EMITTER.spawn_cooldown);

        ssbo_particle_transforms.transforms[index].scale = vec4(0);
    #else
        ssbo_particles.particles[index].velocity_life.xyz = vel;
        ssbo_particles.particles[index].velocity_life.w += EMITTER.lifespan;

        ssbo_particle_transforms.transforms[index].scale = vec4(1);

    #endif


    ssbo_particle_transforms.transforms[index].pos.xyz = spawn_pos;

}

#undef EMITTER

// This function runs just before the memory for the emitter is deleted and the buffers are restructured
void OnEmitterDelete(uint emitter_index) {
    for (uint i = emitter_index + 1; i < ssbo_particle_emitters.emitters.length(); i++) {
        ssbo_particle_emitters.emitters[i].start_index -= ssbo_particle_emitters.emitters[emitter_index].num_particles;
    }

      for (uint i = ssbo_particle_emitters.emitters[emitter_index].start_index + ssbo_particle_emitters.emitters[emitter_index].num_particles; i < ssbo_particles.particles.length(); i++) {
        ssbo_particles.particles[i].emitter_index--;
    }
}