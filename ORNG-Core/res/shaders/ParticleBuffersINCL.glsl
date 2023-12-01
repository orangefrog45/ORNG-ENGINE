ORNG_INCLUDE "UtilINCL.glsl"
ORNG_INCLUDE "CommonINCL.glsl"
#define VELOCITY_SCALE 35.0

struct ParticleEmitter {
    vec4 pos;
    float start_index;
    float num_particles;
    float spread;
    float vel_scale_min;
    mat4 rotation_matrix;
    vec4 spawn_extents_vel_scale_max;
};

struct Particle {
    vec4 velocity_life;
    float emitter_index;
    vec2 seed; // seed for generating initial random position
};

// CHANGE NAMES TO SSBO
layout(std140, binding = 5) buffer ParticleEmitters {
    ParticleEmitter emitters[];
} ubo_particle_emitters;

layout(std140, binding = 6) buffer ParticleTransforms {
    mat4 transforms[];
} ubo_particle_transforms;

layout(std140, binding = 7) buffer Particles {
    Particle particles[];
} ubo_particles;


void InitializeParticle(highp uint index) {

    vec3 emitter_pos = ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].pos.xyz;
    vec3 pos = vec3(ubo_particle_transforms.transforms[index][3][0], ubo_particle_transforms.transforms[index][3][1], ubo_particle_transforms.transforms[index][3][2]);
    
    float az = rnd(vec2(ubo_particle_transforms.transforms[index][3][2], ubo_particle_transforms.transforms[index][3][0])) * ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].spread;
    float pol = rnd(vec2(ubo_particle_transforms.transforms[index][3][1], -ubo_particle_transforms.transforms[index][3][2])) * ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].spread;

    float rnd_vel = rnd(vec2(index, pol)) * (ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].spawn_extents_vel_scale_max.w - ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].vel_scale_min);
    vec3 vel = mat3(ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].rotation_matrix) * ((vec3( sin(pol) * cos(az), sin(pol) * sin(az), cos(pol)))) * abs(rnd_vel);
    
    #ifdef FIRST_INITIALIZATION
        ubo_particles.particles[index].velocity_life = vec4(vel, (index % 64000) * 10000.0 / 64000.0);
        ubo_particle_transforms.transforms[index] =
        mat4(0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            emitter_pos.x, emitter_pos.y, emitter_pos.z, 1
        );
    #else
        ubo_particles.particles[index].velocity_life = vec4(vel, 10000.0);
        ubo_particle_transforms.transforms[index] =
        mat4(1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            emitter_pos.x,  emitter_pos.y, emitter_pos.z, 1
        );
    #endif

}

// This function runs just before the memory for the emitter is deleted and the buffers are restructured
void OnEmitterDelete(uint emitter_index) {
    for (uint i = emitter_index + 1; i < ubo_particle_emitters.emitters.length(); i++) {
        ubo_particle_emitters.emitters[i].start_index -= ubo_particle_emitters.emitters[emitter_index].num_particles;
    }

      for (uint i = uint(ubo_particle_emitters.emitters[emitter_index].start_index + ubo_particle_emitters.emitters[emitter_index].num_particles); i < ubo_particles.particles.length(); i++) {
        ubo_particles.particles[i].emitter_index--;
    }
}