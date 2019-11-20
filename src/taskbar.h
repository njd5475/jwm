/**
 * @file taskbar.h
 * @author Joe Wingbermuehle
 * @date 2005-2006
 *
 * @brief Task list tray component.
 *
 */

#ifndef TASKBAR_H
#define TASKBAR_H

#include "tray.h"
#include "TrayComponent.h"

struct MenuAction;
struct ClientNode;
struct TimeType;

typedef struct ClientEntry {
	ClientNode *client;
	struct ClientEntry *next;
	struct ClientEntry *prev;
} ClientEntry;

typedef struct TaskEntry {
	ClientEntry *clients;
	struct TaskEntry *next;
	struct TaskEntry *prev;
} TaskEntry;

class TaskBar : public TrayComponent {
public:
	TaskBar(Tray *tray, TrayComponent *parent);
	virtual ~TaskBar() {}

private:
	TrayComponent *cp;
	struct TaskBar *next;

	int maxItemWidth;
	int userHeight;
	int itemHeight;
	int itemWidth;
	LayoutType layout;
	char labeled;

	Pixmap buffer;

	TimeType mouseTime;
	int mousex, mousey;

	static TaskBar *bars;
	static TaskEntry *taskEntries;
	static TaskEntry *taskEntriesTail;
public:
	void ComputeItemSize();
	char ShouldShowEntry(const TaskEntry *tp);
	static char ShouldFocusEntry(const TaskEntry *tp);
	TaskEntry *GetEntry(int x, int y);
	void Render();
	void Draw(Graphics *g);
	void ShowClientList(TaskBar *bar, TaskEntry *tp);
	static void RunTaskBarCommand(MenuAction *action, unsigned button);

	void SetSize(int width, int height);
	virtual void Create();
	void Resize();
	void ProcessTaskButtonEvent(int x, int y, int mask);
	static void MinimizeGroup(const TaskEntry *tp);
	static void FocusGroup(const TaskEntry *tp);
	static char IsGroupOnTop(const TaskEntry *entry);
	void ProcessTaskMotionEvent(int x, int y, int mask);
	static void SignalTaskbar(const TimeType *now, int x, int y, Window w, void *data);

	/*@{*/
	static void InitializeTaskBar(void);
	static void StartupTaskBar() {}
	static void ShutdownTaskBar(void);
	static void DestroyTaskBar(void);
	/*@}*/

	/** Create a new task bar tray component. */
	struct TrayComponent *CreateTaskBar();

	/** Add a client to the task bar(s).
	 * @param np The client to add.
	 */
	static void AddClientToTaskBar(struct ClientNode *np);

	/** Remove a client from the task bar(s).
	 * @param np The client to remove.
	 */
	static void RemoveClientFromTaskBar(struct ClientNode *np);

	/** Update all task bars. */
	static void UpdateTaskBar(void);

	/** Focus the next client in the task bar. */
	static void FocusNext(void);

	/** Focus the previous client in the task bar. */
	static void FocusPrevious(void);

	/** Set the maximum width of task bar items.
	 * @param cp The task bar component.
	 * @param value The maximum width.
	 */
	void SetMaxTaskBarItemWidth(const char *value);

	/** Set the preferred height of task bar items.
	 * @param cp The task bar component.
	 * @param value The height.
	 */
	static void SetTaskBarHeight(struct TrayComponent *cp, const char *value);

	/** Set whether labels should be displayed (or icon only).
	 * @param cp The task bar component.
	 * @param value 1 if labeled, 0 if no label is to be shown.
	 */
	static void SetTaskBarLabeled(struct TrayComponent *cp, char value);

	/** Update the _NET_CLIENT_LIST property. */
	static void UpdateNetClientList(void);

};

#endif /* TASKBAR_H */
