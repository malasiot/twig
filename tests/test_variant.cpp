#include <gtest/gtest.h>
#include <variant/variant.hpp>

using namespace twig;

class VariantTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};

// Test Variant construction and type detection
TEST_F(VariantTest, ConstructorUndefined) {
    Variant v;
    EXPECT_TRUE(v.isUndefined());
}

TEST_F(VariantTest, ConstructorInteger) {
    Variant v(42);
    EXPECT_EQ(v.toInteger(), 42);
}

TEST_F(VariantTest, ConstructorString) {
    Variant v("hello");
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.toString(), "hello");
}

TEST_F(VariantTest, ConstructorBoolean) {
    Variant v(true);
    EXPECT_TRUE(v.toBoolean());
}

TEST_F(VariantTest, ConstructorFloat) {
    Variant v(3.14);
    EXPECT_DOUBLE_EQ(v.toFloat(), 3.14);
}

// Test Variant arrays
TEST_F(VariantTest, ConstructorArray) {
    Variant::Array arr{1, 2, 3};
    Variant v(arr);
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.length(), 3);
}

// Test Variant objects
TEST_F(VariantTest, ConstructorObject) {
    Variant::Object obj{{"key", "value"}};
    Variant v(obj);
    EXPECT_TRUE(v.isObject());
    EXPECT_EQ(v.length(), 1);
}

// Test length method
TEST_F(VariantTest, LengthString) {
    Variant v("hello");
    EXPECT_EQ(v.length(), 5);
}

TEST_F(VariantTest, LengthArray) {
    Variant::Array arr{1, 2, 3, 4};
    Variant v(arr);
    EXPECT_EQ(v.length(), 4);
}

// Test null value
TEST_F(VariantTest, NullVariant) {
    Variant v = Variant::null();
    EXPECT_TRUE(v.isNull());
}

// Test copy constructor
TEST_F(VariantTest, CopyConstructor) {
    Variant v1("original");
    Variant v2(v1);
    EXPECT_EQ(v2.toString(), "original");
}

// Test move constructor
TEST_F(VariantTest, MoveConstructor) {
    Variant v1("moved");
    Variant v2(std::move(v1));
    EXPECT_EQ(v2.toString(), "moved");
}
