#pragma once

#include <vector>

namespace minfi {

using Frame = std::vector<float>;

// Linearly interpolate element-wise between two frames.
// t is clamped to [0, 1]. Throws std::invalid_argument on size mismatch.
Frame interpolate(const Frame& a, const Frame& b, float t);

}  // namespace minfi
