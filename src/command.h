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
};

#endif /* COMMAND_H */

