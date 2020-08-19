#include "gtest/gtest.h" // we will add the path to C preprocessor later

#include "../src/DesktopEnvironment.h"
#include "../src/DesktopSystemComponent.h"
#include "../src/HashMap.h"


TEST(HashMap, SetValue) {
  HashMap<const char*> map;
  map["first"] = "hello world";
  ASSERT_EQ(map["first"].value(), "hello world");
  ASSERT_TRUE(true);
}

TEST(HashMap, hasKey) {
  HashMap<const char*> map;
  ASSERT_FALSE(map.has("first"));
  map["first_key"] = "Value";
  ASSERT_FALSE(map.has("first"));
  ASSERT_TRUE(map.has("first_key"));
}

int main(int argc, char **argv) {
  assert(environment->OpenConnection());
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
