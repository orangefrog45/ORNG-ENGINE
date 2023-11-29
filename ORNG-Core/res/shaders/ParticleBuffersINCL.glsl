ORNG_INCLUDE "UtilINCL.glsl"
ORNG_INCLUDE "CommonINCL.glsl"
#define VELOCITY_SCALE 15.0

struct ParticleEmitter {
    vec4 pos;
    float start_index;
    float num_particles;
    float spawn_cooldown;
};

struct Particle {
    vec4 velocity_life;
    float emitter_index;
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


void InitializeParticle(uint index) {

    vec3 emitter_pos = ubo_particle_emitters.emitters[uint(ubo_particles.particles[index].emitter_index)].pos.xyz;
    vec2 rnd_seed = abs(vec2(ubo_particle_transforms.transforms[index][3][0], ubo_particle_transforms.transforms[index][3][1]));
    
    vec3 vel = (vec3(gold_noise(rnd_seed.xy, index - ubo_common.delta_time), gold_noise(rnd_seed.yx, index + ubo_common.delta_time), gold_noise(rnd_seed.xx, ubo_common.delta_time + sin(index + 0.001))) - vec3(0.5)) * 2.0 * VELOCITY_SCALE;

    #ifdef FIRST_INITIALIZATION
        ubo_particles.particles[index].velocity_life = vec4(vel, (index % 64000) * 10000.0 / 64000.0);
        ubo_particle_transforms.transforms[index] =
        mat4(0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            emitter_pos.x,  emitter_pos.y, emitter_pos.z, 1
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