check_LTLIBRARIES = libgtest.la
libgtest_la_SOURCES = ../googletest/googletest/src/gtest-all.cc
libgtest_la_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest
libgtest_la_LDFLAGS = -pthread

check_PROGRAMS = gtest

gtest_SOURCES = gtest.cpp ../src/DesktopEnvironment.cpp ../src/DesktopComponent.cpp ../src/AbstractAction.cpp\
	../src/desktop.cpp ../src/DockComponent.cpp ../src/dock.cpp ../src/BackgroundComponent.cpp ../src/action.cpp\
	../src/button.cpp ../src/confirm.cpp ../src/color.cpp ../src/clientlist.cpp ../src/client.cpp ../src/clock.cpp\
	../src/cursor.cpp ../src/background.cpp ../src/border.cpp ../src/debug.cpp ../src/battery.cpp ../src/binding.cpp\
	../src/command.cpp ../src/error.cpp ../src/event.cpp ../src/font.cpp ../src/grab.cpp ../src/gradient.cpp\
	../src/group.cpp ../src/help.cpp ../src/hint.cpp ../src/icon.cpp ../src/image.cpp ../src/lex.cpp ../src/match.cpp\
	../src/menu.cpp ../src/misc.cpp ../src/move.cpp ../src/outline.cpp ../src/pager.cpp ../src/parse.cpp\
	../src/place.cpp ../src/popup.cpp ../src/render.cpp ../src/resize.cpp ../src/root.cpp ../src/screen.cpp\
	../src/settings.cpp ../src/spacer.cpp ../src/status.cpp ../src/swallow.cpp ../src/taskbar.cpp ../src/timing.cpp\
	../src/tray.cpp ../src/traybutton.cpp ../src/winmenu.cpp ../src/Component.cpp

gtest_LDADD = libgtest.la

gtest_LDFLAGS = -pthread

gtest_CPPFLAGS = @CFLAGS@ -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest -pthread -fpermissive

TESTS = gtest

