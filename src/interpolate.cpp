#include "minfi/interpolate.hpp"

#include <algorithm>
#include <stdexcept>

namespace minfi {

namespace {
inline float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}
}  // namespace

Frame interpolate(const Frame& a, const Frame& b, float t) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("interpolate: frame size mismatch");
  }
  const float u = clamp01(t);
  Frame out;
  out.resize(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    out[i] = a[i] * (1.0f - u) + b[i] * u;
  }
  return out;
}

}  // namespace minfi
