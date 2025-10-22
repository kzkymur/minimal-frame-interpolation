// Fragment shader sampling a 2D texture with nearest filtering

@group(0) @binding(0) var texImg : texture_2d<f32>;
@group(0) @binding(1) var texSmp : sampler;

struct FSIn {
  @location(0) uv : vec2<f32>,
};

@fragment
fn fs(in : FSIn) -> @location(0) vec4<f32> {
  let c = textureSample(texImg, texSmp, in.uv);
  return c;
}

