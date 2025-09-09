#include <gtest/gtest.h>

#include "minfi/interpolate.hpp"

using minfi::Frame;
using minfi::interpolate;

TEST(Interpolate, Endpoints) {
  Frame a{0.f, 1.f, 2.f};
  Frame b{2.f, 1.f, 0.f};
  EXPECT_EQ(interpolate(a, b, 0.0f), a);
  EXPECT_EQ(interpolate(a, b, 1.0f), b);
}

TEST(Interpolate, Midpoint) {
  Frame a{0.f, 1.f, 2.f};
  Frame b{2.f, 1.f, 0.f};
  Frame m = interpolate(a, b, 0.5f);
  EXPECT_FLOAT_EQ(m[0], 1.f);
  EXPECT_FLOAT_EQ(m[1], 1.f);
  EXPECT_FLOAT_EQ(m[2], 1.f);
}

TEST(Interpolate, ClampT) {
  Frame a{0.f, 0.f};
  Frame b{1.f, 1.f};
  auto below = interpolate(a, b, -1.0f);
  auto above = interpolate(a, b, 2.0f);
  EXPECT_FLOAT_EQ(below[0], 0.f);
  EXPECT_FLOAT_EQ(below[1], 0.f);
  EXPECT_FLOAT_EQ(above[0], 1.f);
  EXPECT_FLOAT_EQ(above[1], 1.f);
}

TEST(Interpolate, MismatchThrows) {
  Frame a{0.f};
  Frame b{0.f, 1.f};
  EXPECT_THROW(interpolate(a, b, 0.5f), std::invalid_argument);
}
