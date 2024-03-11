// RGBA8/Vec4 conversion functions sourced from paper https://www.icare3d.org/research/OpenGLInsights-SparseVoxelization.pdf
vec4 convRGBA8ToVec4(uint val) {
  return vec4(float((val & 0x000000FF)),
      float((val & 0x0000FF00) >> 8U),
      float((val & 0x00FF0000) >> 16U),
      float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val) {
  return (uint(val.w) & 0x000000FF) << 24U
    | (uint(val.z) & 0x000000FF) << 16U
    | (uint(val.y) & 0x000000FF) << 8U
    | (uint(val.x) & 0x000000FF);
}


ivec3 ApplyMipFaceTexCoordOffset(ivec3 coord, uint face, uint mip_dim) {
    return coord + ivec3(0, 0, face * mip_dim);
}