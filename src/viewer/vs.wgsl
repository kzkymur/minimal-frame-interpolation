// ---------- bindings ----------
@group(0) @binding(0) var tex : texture_2d<f32>;
@group(0) @binding(1) var texSampler : sampler;

// ---------- vertex ----------
struct VSOut {
  @builtin(position) pos : vec4<f32>,
  @location(0) uv : vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vi : u32) -> VSOut {
  // フルスクリーン三角形（NDC）
  var positions = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -3.0),
    vec2<f32>(-1.0,  1.0),
    vec2<f32>( 3.0,  1.0)
  );
  // 対応するUV（そのまま貼る）
  var uvs = array<vec2<f32>, 3>(
    vec2<f32>(0.0, 2.0),
    vec2<f32>(0.0, 0.0),
    vec2<f32>(2.0, 0.0)
  );

  var o : VSOut;
  o.pos = vec4<f32>(positions[vi], 0.0, 1.0);
  o.uv  = uvs[vi];
  return o;
}
