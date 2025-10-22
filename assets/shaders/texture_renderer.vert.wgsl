// Vertex shader for full-screen triangle
// Outputs UV at @location(0)

struct VSOut {
  @builtin(position) pos : vec4<f32>,
  @location(0) uv : vec2<f32>,
};

@vertex
fn vs(@builtin(vertex_index) vid : u32) -> VSOut {
  var pos = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -3.0),
    vec2<f32>( 3.0,  1.0),
    vec2<f32>(-1.0,  1.0)
  );
  var uv  = array<vec2<f32>, 3>(
    vec2<f32>(0.0, 2.0),
    vec2<f32>(2.0, 0.0),
    vec2<f32>(0.0, 0.0)
  );
  var o : VSOut;
  o.pos = vec4<f32>(pos[vid], 0.0, 1.0);
  o.uv  = uv[vid];
  return o;
}

