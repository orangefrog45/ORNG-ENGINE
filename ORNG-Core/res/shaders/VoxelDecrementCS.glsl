#version 460 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(binding = 0, r32ui) uniform uimage3D voxel_image;

#if defined ON_CAM_POS_UPDATE || defined BLIT
layout(binding = 1, r32ui) uniform uimage3D voxel_image_copy_dest;
#endif

#ifdef ON_CAM_POS_UPDATE
uniform ivec3 u_delta_tex_coords;
#endif



ORNG_INCLUDE "VoxelCommonINCL.glsl"

#ifdef ON_CAM_POS_UPDATE
void OnCamPosUpdate() {
    ivec3 tex_coord = ivec3(gl_GlobalInvocationID);

    uint tex_size = imageSize(voxel_image).x;

    ivec3 new_coords = tex_coord - u_delta_tex_coords/4;

    if (any(greaterThan(new_coords, ivec3(tex_size))) || any(lessThan(new_coords, ivec3(0)))) {
        // New coords out of bounds
        return;
    }

    uint data = imageLoad(voxel_image, tex_coord).r;

    // Move data into a new texture which will then be copied back into the original
    imageStore(voxel_image_copy_dest, new_coords, uvec4(data));
}
#endif

#ifdef DECREMENT_LUMINANCE
void DecrementLuminance() {
    ivec3 tex_coord = ivec3(gl_GlobalInvocationID);

    vec4 res = convRGBA8ToVec4(imageLoad(voxel_image, tex_coord).r);
    res *= 0.95;
    
    imageStore(voxel_image, tex_coord, uvec4(convVec4ToRGBA8(res)));
}
#endif

void main() {
#ifdef ON_CAM_POS_UPDATE
    // If camera position (which is aligned to the voxel grid of the highest mip) changes, all accumulated luminance data in the voxel texture needs to be moved so lighting remains in 
    // the correct place relative to the camera
    OnCamPosUpdate();
#elif defined(DECREMENT_LUMINANCE)
    // This shader is responsible for decreasing the stored luminance in the scene voxel 3D texture
    // Done to blend colour over frames to prevent harsh voxel snapping artifacts, causes slower lighting updates overall though
    // Decremented result used in SceneVoxelizationFS.glsl
    DecrementLuminance();

#elif defined(BLIT)
    // Just copy the texture
    imageStore(voxel_image_copy_dest, ivec3(gl_GlobalInvocationID), uvec4(imageLoad(voxel_image, ivec3(gl_GlobalInvocationID)).r));

#endif
}