vec4 convRGBA8ToVec4(uint val) {
  return vec4(float((val & 0x000000FF)),
      float((val & 0x0000FF00) >> 8U),
      float((val & 0x00FF0000) >> 16U),
      float((val & 0xFF000000) >> 24U));
}

ivec3 ApplyMipFaceTexCoordOffset(ivec3 coord, uint face, uint mip_dim) {
    return coord + ivec3(0, 0, face * mip_dim);
}