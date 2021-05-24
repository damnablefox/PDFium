// Copyright 2021 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_pathdata.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(CFX_PathData, BasicTest) {
  CFX_PathData path;
  path.AppendRect(/*left=*/1, /*bottom=*/2, /*right=*/3, /*top=*/5);
  EXPECT_EQ(5u, path.GetPoints().size());
  EXPECT_TRUE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(1, 2, 3, 5));

  const CFX_Matrix kScaleMatrix(1, 0, 0, 2, 60, 70);
  rect = path.GetRect(&kScaleMatrix);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(61, 74, 63, 80));

  path.Clear();
  EXPECT_EQ(0u, path.GetPoints().size());
  EXPECT_FALSE(path.IsRect());

  // As is, 4 points do not make a rect without a closed path.
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({1, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({1, 0}, FXPT_TYPE::LineTo);
  EXPECT_EQ(4u, path.GetPoints().size());
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  // As is, 4 points with a closed path makes a rect.
  path.ClosePath();
  EXPECT_EQ(4u, path.GetPoints().size());
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 1, 1));

  path.Transform(kScaleMatrix);
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(60, 70, 61, 72));

  path.Clear();
  path.AppendFloatRect({1, 2, 3, 5});
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(1, 2, 3, 5));
}

TEST(CFX_PathData, ShearTransform) {
  CFX_PathData path;
  path.AppendRect(/*left=*/1, /*bottom=*/2, /*right=*/3, /*top=*/5);

  const CFX_Matrix kShearMatrix(1, 2, 0, 1, 0, 0);
  EXPECT_TRUE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(&kShearMatrix);
  EXPECT_FALSE(rect.has_value());

  path.Transform(kShearMatrix);
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  const CFX_Matrix shear_inverse_matrix = kShearMatrix.GetInverse();
  rect = path.GetRect(&shear_inverse_matrix);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(1, 2, 3, 5));

  path.Transform(shear_inverse_matrix);
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(1, 2, 3, 5));
}

TEST(CFX_PathData, Hexagon) {
  CFX_PathData path;
  path.AppendPoint({1, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({3, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 2}, FXPT_TYPE::LineTo);
  path.AppendPoint({1, 2}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  ASSERT_EQ(6u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(5));
  EXPECT_FALSE(path.IsClosingFigure(5));
  EXPECT_FALSE(path.IsRect());
  EXPECT_FALSE(path.GetRect(nullptr).has_value());

  path.ClosePath();
  ASSERT_EQ(6u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(5));
  EXPECT_TRUE(path.IsClosingFigure(5));
  EXPECT_FALSE(path.IsRect());
  EXPECT_FALSE(path.GetRect(nullptr).has_value());

  // Calling ClosePath() repeatedly makes no difference.
  path.ClosePath();
  ASSERT_EQ(6u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(5));
  EXPECT_TRUE(path.IsClosingFigure(5));
  EXPECT_FALSE(path.IsRect());
  EXPECT_FALSE(path.GetRect(nullptr).has_value());

  // A hexagon with the same start/end point is still not a rectangle.
  path.Clear();
  path.AppendPoint({1, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({3, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 2}, FXPT_TYPE::LineTo);
  path.AppendPoint({1, 2}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({1, 0}, FXPT_TYPE::LineTo);
  EXPECT_FALSE(path.IsRect());
  EXPECT_FALSE(path.GetRect(nullptr).has_value());
}

TEST(CFX_PathData, ClosePath) {
  CFX_PathData path;
  path.AppendLine({0, 0}, {0, 1});
  path.AppendLine({0, 1}, {1, 1});
  path.AppendLine({1, 1}, {1, 0});
  ASSERT_EQ(4u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(3));
  EXPECT_FALSE(path.IsClosingFigure(3));

  // TODO(crbug.com/pdfium/1683): Resolve disagreement between these 2 calls,
  // and the call with `kIdentityMatrix` below.
  EXPECT_FALSE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  const CFX_Matrix kIdentityMatrix;
  ASSERT_TRUE(kIdentityMatrix.IsIdentity());
  rect = path.GetRect(&kIdentityMatrix);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 1, 1));

  path.ClosePath();
  ASSERT_EQ(4u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(3));
  EXPECT_TRUE(path.IsClosingFigure(3));
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 1, 1));

  // Calling ClosePath() repeatedly makes no difference.
  path.ClosePath();
  ASSERT_EQ(4u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(3));
  EXPECT_TRUE(path.IsClosingFigure(3));
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 1, 1));

  path.AppendPointAndClose({0, 0}, FXPT_TYPE::LineTo);
  ASSERT_EQ(5u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(3));
  EXPECT_TRUE(path.IsClosingFigure(3));
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(4));
  EXPECT_TRUE(path.IsClosingFigure(4));
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 1, 1));
}

TEST(CFX_PathData, FivePointRect) {
  CFX_PathData path;
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  ASSERT_EQ(5u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(4));
  EXPECT_FALSE(path.IsClosingFigure(4));
  EXPECT_TRUE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 2, 1));

  path.ClosePath();
  ASSERT_EQ(5u, path.GetPoints().size());
  EXPECT_EQ(FXPT_TYPE::LineTo, path.GetType(4));
  EXPECT_TRUE(path.IsClosingFigure(4));
  EXPECT_TRUE(path.IsRect());
  rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 2, 1));
}

TEST(CFX_PathData, SixPlusPointRect) {
  CFX_PathData path;
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  // TODO(crbug.com/pdfium/1683): Treat this as a rect.
  EXPECT_FALSE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.Clear();
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  // TODO(crbug.com/pdfium/1683): Treat this as a rect.
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());
}

TEST(CFX_PathData, NotRect) {
  CFX_PathData path;
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0.1f}, FXPT_TYPE::LineTo);
  EXPECT_FALSE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.ClosePath();
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.Clear();
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({3, 1}, FXPT_TYPE::LineTo);
  path.AppendPointAndClose({0, 1}, FXPT_TYPE::LineTo);
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.Clear();
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 1}, FXPT_TYPE::LineTo);
  path.AppendPointAndClose({0, 1}, FXPT_TYPE::MoveTo);
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.Clear();
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({3, 0}, FXPT_TYPE::LineTo);
  path.AppendPointAndClose({0, 1}, FXPT_TYPE::LineTo);
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.Clear();
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());

  path.Clear();
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({2, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  EXPECT_FALSE(path.IsRect());
  rect = path.GetRect(nullptr);
  EXPECT_FALSE(rect.has_value());
}

TEST(CFX_PathData, EmptyRect) {
  // Document existing behavior where an empty rect is still considered a rect.
  CFX_PathData path;
  path.AppendPoint({0, 0}, FXPT_TYPE::MoveTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 1}, FXPT_TYPE::LineTo);
  path.AppendPoint({0, 0}, FXPT_TYPE::LineTo);
  EXPECT_TRUE(path.IsRect());
  Optional<CFX_FloatRect> rect = path.GetRect(nullptr);
  ASSERT_TRUE(rect.has_value());
  EXPECT_EQ(rect.value(), CFX_FloatRect(0, 0, 0, 1));
}