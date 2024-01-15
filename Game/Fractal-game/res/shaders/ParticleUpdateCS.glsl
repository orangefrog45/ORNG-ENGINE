#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define PARTICLES_DETACHED
ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"



void InitializeParticle(uint index) {
    // for now just arrange in a line as a visual test
    ssbo_particles_detached.particles[index].pos = vec4(index, 0, 0, 0);
    ssbo_particles_detached.particles[index].scale = vec4(1.0);
    ssbo_particles_detached.particles[index].quat = vec4(0, 0, 0, 1);
    ssbo_particles_detached.particles[index].flags = 0;

    ssbo_particles_detached.particles[index].velocity_life.w = rnd(vec2(index, index)) * 10000.0;
}

#define K float k = max(1.5 / (r2 ), 0.68);
vec3 CSize = vec3(0.5, 0.8, 0.9) * 1.35;

float map(vec3 p) {
	//p.y += 40.0 + 20.0 * sin(ubo_common.time_elapsed * 0.0001 * p.x * 0.01);
	p.y += 40;

	vec3 o = p;
	p = p.xzy;
	float scale = 1.;
	// Move point towards limit set
	for (int i = 0; i < 12; i++)
	{
		// box fold
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + p); //Alternate fractal
		K
		p *= k;
		scale *= k;
	}

	float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z;
	rxy = max(rxy, -(n) / 4.);
	return (rxy) / abs(scale);

}

vec3 Colour(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 24; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
			K

		p *= k;
		scale *= k;
	}
	return  clamp(mix(abs(p.xzy) * vec3(0.2, 0.001, 0.005), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1));
}

vec3 calcNormal(in vec3 pos, float t)
{
	float precis = 0.001 * t;

	vec2 e = vec2(1.0, -1.0) * precis;
	return normalize(e.xyy * map(pos + e.xyy) +
		e.yyx * map(pos + e.yyx) +
		e.yxy * map(pos + e.yxy) +
		e.xxx * map(pos + e.xxx));
}


// Permuted congruential generator (only top 16 bits are well shuffled).
// References: 1. Mark Jarzynski and Marc Olano, "Hash Functions for GPU Rendering".
//             2. UnrealEngine/Random.ush. https://github.com/EpicGames/UnrealEngine
uvec2 _pcg3d16(uvec3 p)
{
	uvec3 v = p * 1664525u + 1013904223u;
	v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
	v.x += v.y*v.z; v.y += v.z*v.x;
	return v.xy;
}
uvec2 _pcg4d16(uvec4 p)
{
	uvec4 v = p * 1664525u + 1013904223u;
	v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
	v.x += v.y*v.w; v.y += v.z*v.x;
	return v.xy;
}

// Get random gradient from hash value.
vec3 _gradient3d(uint hash)
{
	vec3 g = vec3(uvec3(hash) & uvec3(0x80000, 0x40000, 0x20000));
	return g * (1.0 / vec3(0x40000, 0x20000, 0x10000)) - 1.0;
}
vec4 _gradient4d(uint hash)
{
	vec4 g = vec4(uvec4(hash) & uvec4(0x80000, 0x40000, 0x20000, 0x10000));
	return g * (1.0 / vec4(0x40000, 0x20000, 0x10000, 0x8000)) - 1.0;
}

// Optimized 3D Bitangent Noise. Approximately 113 instruction slots used.
// Assume p is in the range [-32768, 32767].
vec3 BitangentNoise3D(vec3 p)
{
	const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
	const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

	// First corner
	vec3 i = floor(p + dot(p, C.yyy));
	vec3 x0 = p - i + dot(i, C.xxx);

	// Other corners
	vec3 g = step(x0.yzx, x0.xyz);
	vec3 l = 1.0 - g;
	vec3 i1 = min(g.xyz, l.zxy);
	vec3 i2 = max(g.xyz, l.zxy);

	// x0 = x0 - 0.0 + 0.0 * C.xxx;
	// x1 = x0 - i1  + 1.0 * C.xxx;
	// x2 = x0 - i2  + 2.0 * C.xxx;
	// x3 = x0 - 1.0 + 3.0 * C.xxx;
	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
	vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

	i = i + 32768.5;
	uvec2 hash0 = _pcg3d16(uvec3(i));
	uvec2 hash1 = _pcg3d16(uvec3(i + i1));
	uvec2 hash2 = _pcg3d16(uvec3(i + i2));
	uvec2 hash3 = _pcg3d16(uvec3(i + 1.0 ));

	vec3 p00 = _gradient3d(hash0.x); vec3 p01 = _gradient3d(hash0.y);
	vec3 p10 = _gradient3d(hash1.x); vec3 p11 = _gradient3d(hash1.y);
	vec3 p20 = _gradient3d(hash2.x); vec3 p21 = _gradient3d(hash2.y);
	vec3 p30 = _gradient3d(hash3.x); vec3 p31 = _gradient3d(hash3.y);

	// Calculate noise gradients.
	vec4 m = clamp(0.5 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0, 1.0);
	vec4 mt = m * m;
	vec4 m4 = mt * mt;

	mt = mt * m;
	vec4 pdotx = vec4(dot(p00, x0), dot(p10, x1), dot(p20, x2), dot(p30, x3));
	vec4 temp = mt * pdotx;
	vec3 gradient0 = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
	gradient0 += m4.x * p00 + m4.y * p10 + m4.z * p20 + m4.w * p30;

	pdotx = vec4(dot(p01, x0), dot(p11, x1), dot(p21, x2), dot(p31, x3));
	temp = mt * pdotx;
	vec3 gradient1 = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
	gradient1 += m4.x * p01 + m4.y * p11 + m4.z * p21 + m4.w * p31;

	// The cross products of two gradients is divergence free.
	return cross(gradient0, gradient1) * 3918.76;
}


//	Classic Perlin 3D Noise 
//	by Stefan Gustavson
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
vec3 fade(vec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise(vec3 P){
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 / 7.0;
  vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 / 7.0;
  vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

void main() {

    #ifdef INITIALIZE
        InitializeParticle( gl_GlobalInvocationID.x);
        ssbo_particles_detached.data_int[0] = 0;
    #elif defined STATE_UPDATE
     ssbo_particles_detached.data_int[0] -= int(ubo_common.delta_time);

    #else

    #define PTCL ssbo_particles_detached.particles[gl_GlobalInvocationID.x]


	//vec3 pos = vec3(gl_GlobalInvocationID.x % 100, (gl_GlobalInvocationID.x / 100) %  100, gl_GlobalInvocationID.x / float(100 * 100)) * 0.55 - vec3(50, 50, 50) * 0.55 ;
	float ring_size_f = 100.f;
	int ring_size_i = 100;
	int ring_num = int(gl_GlobalInvocationID.x / ring_size_i);
	int group = ring_num / 400;


	vec3 pos = vec3(100 * cos(2 * PI * (gl_GlobalInvocationID.x % ring_size_i) / ring_size_f),  150 * sin(2 * PI * (gl_GlobalInvocationID.x % ring_size_i) / ring_size_f) ,0) ;

		//ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w = 000.0;
	
	float d = map(pos);
	float r = rnd(vec2(d, gl_GlobalInvocationID.x)) * 2.0 * PI;
	if ( ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w < 0) {
		vec3 col = Colour(pos);
		if (group % 2 == 0) {
			vec3 pc = normalize(abs(ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz)) * 300;
			ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale.w =pc.x * 0.01;
			ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.w = pc.y * 0.01;
			ssbo_particles_detached.particles[gl_GlobalInvocationID.x].quat.w =pc.z * 0.01;
		} else {
			ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale.w = col.r +  vec3(1.0, 0.2, 0.05).r * (1.0 + abs(sin(r)) * 5.0) * 0.25;
			ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.w = col.g +  vec3(1.0, 0.2, 0.05).g * 0.1;
			ssbo_particles_detached.particles[gl_GlobalInvocationID.x].quat.w = col.b +  vec3(1.0, 0.2, 0.05).b * (1.0 + abs(cos(r)) * 5.0) * 10.0;

		}
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].quat.xyz = normalize(vec3(cos(r) * 2.0 * PI, sin(r) * 2.0 * PI, cos(r) * sin(r) * 2.0 * PI));
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz = calcNormal(pos, d) * 0.1;
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w = 60000.0;
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz = pos;
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].flags = 0;
		//EmitParticleDirect(p);
	}

	vec3 p = ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz;
	vec3 col = Colour(p);

	ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w -= ubo_common.delta_time;
	if (ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.z > 20000) {
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.z  = -p.z ;
		vec3 pos =max(cos(2 * 2 * PI * (gl_GlobalInvocationID.x % ring_size_i) / ring_size_f),max(abs(sin(ubo_common.time_elapsed * 0.001)) * 0.25, 0.05)) * 3000 * vec3( cos(2 * PI * (gl_GlobalInvocationID.x % ring_size_i) / ring_size_f),  sin(2 * PI * (gl_GlobalInvocationID.x % ring_size_i) / ring_size_f) ,0) ;
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xy = pos.xy;
	}
	

	if (group == 50) {
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz += int(ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w) / 1000 % 2 == 0 ? (BitangentNoise3D(p  * 0.0095) + BitangentNoise3D(p  * 0.001)) * 200 : (vec3(0, 1, 0.5) * (10 - exp(-ring_num)) * 10 + 1) + BitangentNoise3D(p * 0.001 )* 100 ;
	 	ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale.xyz = vec3(3.0);
	}
	else {
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz = (BitangentNoise3D(p * 0.01) + BitangentNoise3D(p * 0.05) * 0.1) * 15  + vec3(0, 0, log(ring_num * 0.1) * 750  )  ;
	ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz += cross(normalize(ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz), vec3(0, 0, 1))   * ring_num * max(exp(ring_num * 0.0001 ), 1.0) ;
	 ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale.xyz = vec3(2.0) * (1.0 + abs(cnoise(pos * 0.001)));
	}
	 ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz += ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz * 0.001 * ubo_common.delta_time;


	// No updates, particles remain stationary
	// ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos = vec4(gl_GlobalInvocationID.x, sin(gl_GlobalInvocationID.x), 0, 0);
	// ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale = vec4(1);
		//ssbo_particles_detached.particles[gl_GlobalInvocationID.x].quat = vec4(0, 0, 0, 1);

    #endif


}

#undef PTCL