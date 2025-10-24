// Vertex shader for full-screen triangle
// Outputs UV at @location(0)

// DEBUG VERSION: Output a stable 0..1 UV covering the screen
// Original code kept below for reference.

struct VSOut {
  @builtin(position) pos : vec4<f32>,
  @location(0) uv : vec2<f32>,
};

@vertex
fn vs(@builtin(vertex_index) vid : u32) -> VSOut {
  // Full-screen triangle in NDC
  var ndc = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -3.0),
    vec2<f32>( 3.0,  1.0),
    vec2<f32>(-1.0,  1.0)
  );

  var o : VSOut;
  let p = ndc[vid];
  o.pos = vec4<f32>(p, 0.0, 1.0);
  // Map NDC (-1..1) to UV (0..1) so the fragment shader
  // can render a deterministic gradient independent of textures.
  o.uv = p * 0.5 + vec2<f32>(0.5, 0.5);
  return o;
}

// ----- Original (commented out) -----
// struct VSOut {
//   @builtin(position) pos : vec4<f32>,
//   @location(0) uv : vec2<f32>,
// };
//
// @vertex
// fn vs(@builtin(vertex_index) vid : u32) -> VSOut {
//   var pos = array<vec2<f32>, 3>(
//     vec2<f32>(-1.0, -3.0),
//     vec2<f32>( 3.0,  1.0),
//     vec2<f32>(-1.0,  1.0)
//   );
//   var uv  = array<vec2<f32>, 3>(
//     vec2<f32>(0.0, 2.0),
//     vec2<f32>(2.0, 0.0),
//     vec2<f32>(0.0, 0.0)
//   );
//   var o : VSOut;
//   o.pos = vec4<f32>(pos[vid], 0.0, 1.0);
//   o.uv  = uv[vid];
//   return o;
// }
