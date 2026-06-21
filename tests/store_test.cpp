#include "wisp/store.h"

#include <gtest/gtest.h>

TEST(StoreTest, SetThenGetReturnsValue) {
    wisp::Store store;

    store.set("foo", "bar");

    const auto value = store.get("foo");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "bar");
}

TEST(StoreTest, GetMissingKeyReturnsEmptyOptional) {
    wisp::Store store;

    EXPECT_FALSE(store.get("missing").has_value());
}

TEST(StoreTest, SetExistingKeyOverwritesValue) {
    wisp::Store store;

    store.set("foo", "bar");
    store.set("foo", "baz");

    const auto value = store.get("foo");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, "baz");
}

TEST(StoreTest, DelExistingKeyReturnsOne) {
    wisp::Store store;
    store.set("foo", "bar");

    EXPECT_EQ(store.del("foo"), 1U);
    EXPECT_FALSE(store.get("foo").has_value());
}

TEST(StoreTest, DelMissingKeyReturnsZero) {
    wisp::Store store;

    EXPECT_EQ(store.del("missing"), 0U);
}

TEST(StoreTest, ExistsReturnsExpectedState) {
    wisp::Store store;

    EXPECT_FALSE(store.exists("foo"));

    store.set("foo", "bar");
    EXPECT_TRUE(store.exists("foo"));

    store.del("foo");
    EXPECT_FALSE(store.exists("foo"));
}
