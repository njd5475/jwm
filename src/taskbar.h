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

#include <vector>
#include <map>
#include "menu.h"
#include "tray.h"
#include "TrayComponent.h"

struct MenuAction;
class ClientNode;
class Icon;
struct TimeType;

class TaskBar : public TrayComponent {
private:
	TaskBar(Tray *tray, TrayComponent *parent);
public:
	virtual ~TaskBar();
	static TaskBar *Create(Tray *tray, TrayComponent *parent);

private:
	int maxItemWidth;
	int userHeight;
	int itemHeight;
	int itemWidth;
	LayoutType layout;
	char labeled;

	Pixmap buffer;

	TimeType mouseTime;
	int mousex, mousey;

	class BarItem {
	public:
	  BarItem(ClientNode* atLeastOne);
	  virtual ~BarItem();
	  bool ShouldFocusEntry();
	  bool hasActiveClient();
	  bool IsGroupOnTop();
	  bool empty();
	  bool RemoveClient(ClientNode *np);
	  unsigned int activeCount();
	  unsigned int focusCount();
	  void MinimizeGroup();
	  void focusGroup();
	  std::vector<Window> getClientWindows();
	  std::vector<ClientNode*> shouldFocus();
	  const char* getClassName();
	  const char* getName();
	  Icon *getIcon();
	  void ShowClientList(TaskBar *bar);
	  void RunTaskBarCommand(MenuItem::MenuAction* action, unsigned button);
	private:
	  std::vector<ClientNode*> clients;
	};


	static std::vector<TaskBar*> bars;
	static std::map<const char*, BarItem*> taskEntries;
public:
	void ComputeItemSize();
	//static char ShouldFocusEntry(const BarItem *tp);
	BarItem *GetEntry(int x, int y);
	void Draw(Graphics *g);
	virtual void Draw();
	void ShowClientList(TaskBar *bar, BarItem *tp);
	static void RunTaskBarCommand(MenuItem::MenuAction *action, unsigned button);

	void SetSize(int width, int height);
	virtual void Create();
	void Resize();
	void ProcessButtonPress(int x, int y, int mask);
	void ProcessButtonRelease(int x, int y, int mask) {}
	void ProcessMotionEvent(int x, int y, int mask);

//	static void MinimizeGroup(const BarItem *tp);
//	static void FocusGroup(const BarItem *tp);
//	static char IsGroupOnTop(const BarItem *entry);
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
	void SetMaxTaskBarItemWidth(unsigned int itemWidth);

	/** Set the preferred height of task bar items.
	 * @param cp The task bar component.
	 * @param value The height.
	 */
	void SetTaskBarHeight(unsigned int itemHeight);

	/** Set whether labels should be displayed (or icon only).
	 * @param cp The task bar component.
	 * @param value 1 if labeled, 0 if no label is to be shown.
	 */
	static void SetTaskBarLabeled(struct TrayComponent *cp, char value);

	/** Update the _NET_CLIENT_LIST property. */
	static void UpdateNetClientList(void);

};

#endif /* TASKBAR_H */
