// ---------- bindings ----------
@group(0) @binding(0) var tex : texture_2d<f32>;
@group(0) @binding(1) var texSampler : sampler;

// ---------- fragment ----------
@fragment
fn fs_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {
  // sRGBテクスチャを使う場合は書き込みターゲット側をsRGBにする（通常のWebGPU設定）
  return textureSample(tex, texSampler, uv);
}
