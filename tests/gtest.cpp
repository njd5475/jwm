#include "gtest/gtest.h" // we will add the path to C preprocessor later

#include "../src/DesktopEnvironment.h"
#include "../src/DockComponent.h"
#include "../src/DesktopComponent.h"

TEST(DockComponent, InitializeComponent) {
  DockComponent *dc = new DockComponent();
  dc->initialize();
  ASSERT_TRUE(true);
}

TEST(DesktopEnvironment, StartupComponent) {
  DockComponent *dc = new DockComponent();
  dc->start();
  ASSERT_TRUE(true);
}

TEST(DesktopEnvironment, StopComponent) {

}

TEST(DesktopEnvironment, RegisterComponentTest) {
  DesktopEnvironment *de = DesktopEnvironment::DefaultEnvironment();
  ASSERT_NE(NULL, de);
  de->RegisterComponent(new DockComponent());
  ASSERT_EQ(1, de->ComponentCount());
  de->RegisterComponent(new DesktopComponent());
  ASSERT_EQ(2, de->ComponentCount());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
