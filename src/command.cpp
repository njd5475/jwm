/**
 * @file command.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Handle running startup, shutdown, and restart commands.
 *
 */

#include "jwm.h"
#include "command.h"
#include "misc.h"
#include "main.h"
#include "error.h"
#include "timing.h"

#include <vector>
#include <fcntl.h>

std::vector<char*> Commands::startupCommands;
std::vector<char*> Commands::shutdownCommands;
std::vector<char*> Commands::restartCommands;
std::vector<pid_t> Commands::pids;

/** Process startup/restart commands. */
void Commands::StartupCommands(void) {
  if (isRestarting) {
    RunCommands(restartCommands);
  } else {
    RunCommands(startupCommands);
  }
}

/** Process shutdown commands. */
void Commands::ShutdownCommands(void) {
  if (!shouldRestart) {

    RunCommands(shutdownCommands);
  }

  for (auto pid : pids) {
    int ret = kill(pid, SIGTERM);
    char buf[80];
    sprintf(buf, "\nKilling pid=%d returned=%d\n", pid, ret);
    Logger::Log(buf);

  }
  pids.clear();
}

/** Destroy the command lists. */
void Commands::DestroyCommands(void) {
  ReleaseCommands(startupCommands);
  ReleaseCommands(shutdownCommands);
  ReleaseCommands(restartCommands);
}

void Commands::InitializeCommands() {

}

/** Run the commands in a command list. */
void Commands::RunCommands(std::vector<char*> commands) {

  std::vector<char*>::iterator it;
  for (it = commands.begin(); it != commands.end(); ++it) {
    Commands::RunCommand((*it));
  }

}

/** Release a command list. */
void Commands::ReleaseCommands(std::vector<char*> commands) {
  for (auto command : commands) {
    Release(command);
  }
  commands.clear();
}

/** Add a command to a command list. */
void Commands::AddCommand(std::vector<char*> commands, const char *command) {
  commands.push_back(strdup(command));
}

/** Add a startup command. */
void Commands::AddStartupCommand(const char *command) {
  AddCommand(startupCommands, command);
}

/** Add a shutdown command. */
void Commands::AddShutdownCommand(const char *command) {
  AddCommand(shutdownCommands, command);
}

/** Add a restart command. */
void Commands::AddRestartCommand(const char *command) {
  AddCommand(restartCommands, command);
}

/** Execute an external program. */
void Commands::RunCommand(const char *command) {

  const char *displayString;

  if (JUNLIKELY(!command)) {
    return;
  }

  displayString = DisplayString(display);
  pid_t pid = fork();
  if (!pid) {
    close(ConnectionNumber(display));
    if (displayString && displayString[0]) {
      const size_t var_len = strlen(displayString) + 9;
      char *str = (char*) malloc(var_len);
      snprintf(str, var_len, "DISPLAY=%s", displayString);
      putenv(str);
    }
    setsid();
    execl(SHELL_NAME, SHELL_NAME, "-c", command, NULL);
    Warning(_("exec failed: (%s) %s"), SHELL_NAME, command);
    exit(EXIT_SUCCESS);
  } else if (pid != -1) {
    //store pid
    char buf[80];
    sprintf(buf, "\nLaunched pid=%d\n", pid);
    Logger::Log(buf);
    pids.push_back(pid);
  } else {
    //error
    Log("Could not fork process");
  }

}

/** Reads the output of an exernal program. */
char* Commands::ReadFromProcess(const char *command, unsigned timeout_ms) {
  const unsigned BLOCK_SIZE = 256;
  pid_t pid;
  int fds[2];

  if (pipe(fds)) {
    Warning(_("could not create pipe"));
    return NULL;
  }
  if (fcntl(fds[0], F_SETFL, O_NONBLOCK) == -1) {
    /* We don't return here since we can still process the output
     * of the command, but the timeout won't work. */
    Warning(_("could not set O_NONBLOCK"));
  }

  pid = fork();
  if (pid == 0) {
    /* The child process. */
    close(ConnectionNumber(display));
    close(fds[0]);
    dup2(fds[1], 1); /* stdout */
    setsid();
    execl("/bin/sh", "/bin/sh", "-c", command, NULL);
    Warning(_("exec failed: (%s) %s"), SHELL_NAME, command);
    exit(EXIT_SUCCESS);
  } else if (pid > 0) {
    char *buffer;
    unsigned buffer_size, max_size;
    TimeType start_time, current_time;

    max_size = BLOCK_SIZE;
    buffer_size = 0;
    buffer = new char[max_size];

    GetCurrentTime(&start_time);
    for (;;) {
      struct timeval tv;
      unsigned long diff_ms;
      fd_set fs;
      int rc;

      /* Make sure we have room to read. */
      if (buffer_size + BLOCK_SIZE > max_size) {
        max_size *= 2;
        buffer = (char*) Reallocate(buffer, max_size);
      }

      FD_ZERO(&fs);
      FD_SET(fds[0], &fs);

      /* Determine the max time to sit in select. */
      GetCurrentTime(&current_time);
      diff_ms = GetTimeDifference(&start_time, &current_time);
      diff_ms = timeout_ms > diff_ms ? (timeout_ms - diff_ms) : 0;
      tv.tv_sec = diff_ms / 1000;
      tv.tv_usec = (diff_ms % 1000) * 1000;

      /* Wait for data (or a timeout). */
      rc = select(fds[0] + 1, &fs, NULL, &fs, &tv);
      if (rc == 0) {
        /* Timeout */
        Warning(_("timeout: %s did not complete in %u milliseconds"), command,
            timeout_ms);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        break;
      }

      rc = read(fds[0], &buffer[buffer_size], BLOCK_SIZE);
      if (rc > 0) {
        buffer_size += rc;
      } else {
        /* Process exited, check for any leftovers and return. */
        do {
          if (buffer_size + BLOCK_SIZE > max_size) {
            max_size *= 2;
            buffer = (char*) Reallocate(buffer, max_size);
          }
          rc = read(fds[0], &buffer[buffer_size], BLOCK_SIZE);
          buffer_size += (rc > 0) ? rc : 0;
        } while (rc > 0);
        break;
      }
    }
    close(fds[1]);
    buffer[buffer_size] = 0;
    return buffer;
  }

  return NULL;
}
