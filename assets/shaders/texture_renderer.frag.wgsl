// DEBUG VERSION: ignore texture and render UV gradient + checker pattern

// @group(0) @binding(0) var texImg : texture_2d<f32>;
// @group(0) @binding(1) var texSmp : sampler;

struct FSIn {
  @location(0) uv : vec2<f32>,
};

@fragment
fn fs(in : FSIn) -> @location(0) vec4<f32> {
  // Simple UV gradient
  var color = vec3<f32>(in.uv, 0.0);

  // Add a subtle checkerboard to debug interpolation
  let scale = 40.0; // squares per axis
  let check = (select(0.0, 1.0, (i32(floor(in.uv.x * scale)) + i32(floor(in.uv.y * scale))) % 2 == 0));
  let grid = mix(0.25, 1.0, check);
  color *= grid;

  return vec4<f32>(color, 1.0);
}

// ----- Original (commented out) -----
// @group(0) @binding(0) var texImg : texture_2d<f32>;
// @group(0) @binding(1) var texSmp : sampler;
// struct FSIn { @location(0) uv : vec2<f32>, };
// @fragment
// fn fs(in : FSIn) -> @location(0) vec4<f32> {
//   let c = textureSample(texImg, texSmp, in.uv);
//   return c;
// }
