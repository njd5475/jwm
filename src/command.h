/**
 * @file command.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Handle running startup, shutdown, and restart commands.
 *
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <sys/types.h>

class Commands {
public:
	/*@{*/
	static void InitializeCommands();
	static void StartupCommands(void);
	static void ShutdownCommands(void);
	static void DestroyCommands(void);
	/*@}*/

	/** Add a command to be executed at startup.
	 * @param command The command to execute.
	 */
	static void AddStartupCommand(const char *command);

	/** Add a command to be executed at shutdown.
	 * @param command The command to execute.
	 */
	static void AddShutdownCommand(const char *command);

	/** Add a command to be executed after a restart.
	 * @param command The command to execute.
	 */
	static void AddRestartCommand(const char *command);

	/** Run a command.
	 * @param command The command to run (run in sh).
	 */
	static void RunCommand(const char *command);

	/** Read output from a process.
	 * @param command The command to run (run in sh).
	 * @param timeout_ms The timeout in milliseconds.
	 * @return The output (must be freed, NULL on timeout).
	 */
	static char *ReadFromProcess(const char *command, unsigned timeout_ms);

private:
	static std::vector<char*> startupCommands;
	static std::vector<char*> shutdownCommands;
	static std::vector<char*> restartCommands;
	static std::vector<pid_t> pids;

	static void RunCommands(std::vector<char*> commands);
	static void ReleaseCommands(std::vector<char*> commands);
	static void AddCommand(std::vector<char*> commands, const char *command);
};

#endif /* COMMAND_H */

