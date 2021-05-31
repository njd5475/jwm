#!/usr/bin/env ruby

pidMake = spawn 'make'

Process.wait pidMake
retNo = $?
print "Build process returned #{retNo}\n"

if retNo != 0 then
  print "Build failed!"
  exit retNo.to_i
end

desktop = "Xephyr :1 -ac -screen 1440x768"
app = "./src/nicwm"

pidDesktop = spawn desktop
begin
  sleep 1

  puts "Spawning desktop"
  env = {'DISPLAY' => ':1', 'LANG' => 'en_US.UTF-8', 'PATH' => ''}
  pidApp = spawn env, app

  Process.wait pidApp

  puts "App finished, Killing desktop"
  Process.kill "SIGTERM", pidDesktop
raise => e
  print e
  Process.kill 'SIGTERM', pidDesktop
end
# when event app_starts do
#   print Starting build...
#   monitor as @build exec make

# when @build is done do
#   print Virtual Desktop starting...
#   monitor as @desktop exec Xephyr :1 -ac -screen 1440x768

# when @desktop is done do
#   print @desktop.buffers.out @desktop.buffers.err
#   print Starting nicwm
#   monitor as @app exec DISPLAY=:1 ./src/nicwm

# when @app.buffers.out is set do
#   print @app.buffers.out