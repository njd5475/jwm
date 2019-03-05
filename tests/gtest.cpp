#include "gtest/gtest.h" // we will add the path to C preprocessor later

#include "../src/DesktopEnvironment.h"
#include "../src/DockComponent.h"
#include "../src/DesktopComponent.h"
#include "../src/parse.h"

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

TEST(DesktopEnvironment, ParseNewDesktop) {
  ParseConfigString(
"<?xml version=\"1.0\"?>"
" <JWM>"
"  <!-- The root menu. -->"
"  <RootMenu onroot=\"12\">"
"      <Program icon=\"terminal.png\" label=\"Terminal\">xterm</Program>"
"      <Menu icon=\"folder.png\" label=\"Applications\">"
"          <Program icon=\"music.png\" label=\"Audacious\">audacious</Program>"
"          <Program icon=\"calculator.png\" label=\"Calculator\">xcalc</Program>"
"          <Program icon=\"gimp.png\" label=\"Gimp\">gimp</Program>"
"         <Program icon=\"chat.png\" label=\"Pidgin\">pidgin</Program>"
"          <Program icon=\"www.png\" label=\"Firefox\">firefox</Program>"
"          <Program icon=\"editor.png\" label=\"XEdit\">xedit</Program>"
"      </Menu>"
"      <Menu icon=\"folder.png\" label=\"Utilities\">"
"          <Program icon=\"font.png\" label=\"Fonts\">xfontsel</Program>"
"          <Program icon=\"window.png\" label=\"Window Properties\">"
"              xprop | xmessage -file -"
"          </Program>"
"          <Program icon=\"window.png\" label=\"Window Information\">"
"              xwininfo | xmessage -file -"
"          </Program>"
"      </Menu>"
"      <Separator/>"
"      <Program icon=\"lock.png\" label=\"Lock\">"
"          xlock -mode blank"
"      </Program>"
"      <Separator/>"
"      <Restart label=\"Restart\" icon=\"restart.png\"/>"
"      <Exit label=\"Exit\" confirm=\"true\" icon=\"quit.png\"/>"
"  </RootMenu>"
" </JWM>"
  );
  ASSERT_TRUE(true);
}

TEST(DesktopEnvironment, ParseTray) {
  ParseConfigString(
"<?xml version=\"1.0\"?>"
" <JWM>"
"      <!-- Tray at the bottom. -->"
"      <Tray x=\"0\" y=\"-1\" autohide=\"off\">"
"          <TrayButton icon=\"jwm-blue\">root:1</TrayButton>"
"          <Spacer width=\"2\"/>"
"          <TrayButton label=\"_\">showdesktop</TrayButton>"
"          <Spacer width=\"2\"/>"
"          <Pager labeled=\"true\"/>"
"          <TaskList maxwidth=\"246\"/>"
"          <Battery></Battery>"
"          <Dock/>"
"          <Clock format=\"%H:%M\"><Button mask=\"123\">exec:xclock</Button></Clock>"
"      </Tray>"
"</JWM>"
  );
  ASSERT_TRUE(true);
}

TEST(DesktopEnvironment, ParseDesktops) {
  ParseConfigString(
"<?xml version=\"1.0\"?>"
"<JWM>"
"      <!-- Virtual Desktops -->"
"      <!-- Desktop tags can be contained within Desktops for desktop names. -->"
"      <Desktops width=\"4\" height=\"1\">"
"          <!-- Default background. Note that a Background tag can be"
"                contained within a Desktop tag to give a specific background"
"                for that desktop."
"           -->"
"          <Background type=\"solid\">#111111</Background>"
"      </Desktops>"
"</JWM>"
  );
  ASSERT_TRUE(true);
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
