#include <gtest/gtest.h>

#include "br_logger/fixed_vector.hpp"

using br_logger::FixedVector;

TEST(FixedVector, EmptyOnConstruction)
{
  FixedVector<int, 4> v;
  EXPECT_TRUE(v.Empty());
  EXPECT_EQ(v.Size(), 0u);
  EXPECT_FALSE(v.Full());
}

TEST(FixedVector, PushBackIncreasesSize)
{
  FixedVector<int, 4> v;
  EXPECT_TRUE(v.PushBack(10));
  EXPECT_EQ(v.Size(), 1u);
  EXPECT_EQ(v[0], 10);

  EXPECT_TRUE(v.PushBack(20));
  EXPECT_EQ(v.Size(), 2u);
  EXPECT_EQ(v[1], 20);
}

TEST(FixedVector, PushBackReturnsFalseWhenFull)
{
  FixedVector<int, 2> v;
  EXPECT_TRUE(v.PushBack(1));
  EXPECT_TRUE(v.PushBack(2));
  EXPECT_TRUE(v.Full());
  EXPECT_FALSE(v.PushBack(3));
  EXPECT_EQ(v.Size(), 2u);
}

TEST(FixedVector, PopBackDecreasesSize)
{
  FixedVector<int, 4> v;
  v.PushBack(10);
  v.PushBack(20);
  EXPECT_TRUE(v.PopBack());
  EXPECT_EQ(v.Size(), 1u);
  EXPECT_EQ(v[0], 10);
}

TEST(FixedVector, PopBackReturnsFalseWhenEmpty)
{
  FixedVector<int, 4> v;
  EXPECT_FALSE(v.PopBack());
}

TEST(FixedVector, SubscriptAccess)
{
  FixedVector<int, 4> v;
  v.PushBack(100);
  v.PushBack(200);
  v.PushBack(300);
  EXPECT_EQ(v[0], 100);
  EXPECT_EQ(v[1], 200);
  EXPECT_EQ(v[2], 300);

  v[1] = 999;
  EXPECT_EQ(v[1], 999);
}

TEST(FixedVector, RangeForIteration)
{
  FixedVector<int, 4> v;
  v.PushBack(1);
  v.PushBack(2);
  v.PushBack(3);

  int sum = 0;
  for (const auto& val : v)
  {
    sum += val;
  }
  EXPECT_EQ(sum, 6);
}

TEST(FixedVector, ClearResetsSize)
{
  FixedVector<int, 4> v;
  v.PushBack(1);
  v.PushBack(2);
  v.Clear();
  EXPECT_TRUE(v.Empty());
  EXPECT_EQ(v.Size(), 0u);
}

TEST(FixedVector, FullAndEmptyPredicates)
{
  FixedVector<int, 1> v;
  EXPECT_TRUE(v.Empty());
  EXPECT_FALSE(v.Full());

  v.PushBack(42);
  EXPECT_FALSE(v.Empty());
  EXPECT_TRUE(v.Full());

  v.PopBack();
  EXPECT_TRUE(v.Empty());
  EXPECT_FALSE(v.Full());
}

TEST(FixedVector, CapacityIsCorrect)
{
  FixedVector<int, 16> v;
  EXPECT_EQ(v.GetCapacity(), 16u);
}
