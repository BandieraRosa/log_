#include <gtest/gtest.h>
#include "br_logger/fixed_vector.hpp"

using br_logger::FixedVector;

TEST(FixedVector, EmptyOnConstruction) {
    FixedVector<int, 4> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_FALSE(v.full());
}

TEST(FixedVector, PushBackIncreasesSize) {
    FixedVector<int, 4> v;
    EXPECT_TRUE(v.push_back(10));
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 10);

    EXPECT_TRUE(v.push_back(20));
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[1], 20);
}

TEST(FixedVector, PushBackReturnsFalseWhenFull) {
    FixedVector<int, 2> v;
    EXPECT_TRUE(v.push_back(1));
    EXPECT_TRUE(v.push_back(2));
    EXPECT_TRUE(v.full());
    EXPECT_FALSE(v.push_back(3));
    EXPECT_EQ(v.size(), 2u);
}

TEST(FixedVector, PopBackDecreasesSize) {
    FixedVector<int, 4> v;
    v.push_back(10);
    v.push_back(20);
    EXPECT_TRUE(v.pop_back());
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], 10);
}

TEST(FixedVector, PopBackReturnsFalseWhenEmpty) {
    FixedVector<int, 4> v;
    EXPECT_FALSE(v.pop_back());
}

TEST(FixedVector, SubscriptAccess) {
    FixedVector<int, 4> v;
    v.push_back(100);
    v.push_back(200);
    v.push_back(300);
    EXPECT_EQ(v[0], 100);
    EXPECT_EQ(v[1], 200);
    EXPECT_EQ(v[2], 300);

    v[1] = 999;
    EXPECT_EQ(v[1], 999);
}

TEST(FixedVector, RangeForIteration) {
    FixedVector<int, 4> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    int sum = 0;
    for (const auto& val : v) {
        sum += val;
    }
    EXPECT_EQ(sum, 6);
}

TEST(FixedVector, ClearResetsSize) {
    FixedVector<int, 4> v;
    v.push_back(1);
    v.push_back(2);
    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
}

TEST(FixedVector, FullAndEmptyPredicates) {
    FixedVector<int, 1> v;
    EXPECT_TRUE(v.empty());
    EXPECT_FALSE(v.full());

    v.push_back(42);
    EXPECT_FALSE(v.empty());
    EXPECT_TRUE(v.full());

    v.pop_back();
    EXPECT_TRUE(v.empty());
    EXPECT_FALSE(v.full());
}

TEST(FixedVector, CapacityIsCorrect) {
    FixedVector<int, 16> v;
    EXPECT_EQ(v.capacity(), 16u);
}
