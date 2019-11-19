/**
 * @file binding.h
 * @author Joe Wingbermuehle
 * @date 2004-2017
 *
 * @brief Header for mouse/key bindings.
 *
 */

#ifndef KEY_H
#define KEY_H

#include "action.h"
#include "settings.h"

struct KeyNode;

class Binding {

public:
  static void InitializeBindings(void);
  static void StartupBindings(void);
  static void ShutdownBindings(void);
  static void DestroyBindings(void);

  /** Mask of 'lock' keys. */
  static unsigned int lockMask;

  /** Get the action to take from a key event. */
  static ActionType GetKey(MouseContextType context, unsigned state, int code);

  /** Parse a modifier string.
   * @param str The modifier string.
   * @return The modifier mask.
   */
  static unsigned int ParseModifierString(const char *str);

  /** Insert a key binding.
   * @param action The action.
   * @param modifiers The modifier mask.
   * @param stroke The key stroke (not needed if code given).
   * @param code The key code (not needed if stroke given).
   * @param command Extra parameter needed for some key binding types.
   */
  static void InsertBinding(ActionType action, const char *modifiers,
      const char *stroke, const char *code, const char *command);

  /** Insert a mouse binding.
   * A mouse binding maps a mouse click in a certain context to an action.
   * @param button The mouse button.
   * @param mask The modifier mask.
   * @param context The mouse context.
   * @param action The action binding.
   * @param command Extra parameter needed for some bindings.
   */
  static void InsertMouseBinding(
      int button,
      const char *mask,
      MouseContextType context,
      ActionType action,
      const char *command);

  /** Run a command caused by a key binding. */
  static void RunKeyCommand(MouseContextType context, unsigned state, int code);

  /** Show a root menu caused by a key binding.
   * @param event The event that caused the menu to be shown.
   */
  static void ShowKeyMenu(MouseContextType context, unsigned state, int code);

  /** Validate key bindings.
   * This will log an error if an invalid key binding is found.
   * This is called after parsing the configuration file.
   */
  static void ValidateKeys(void);

  static void GrabKey(KeyNode *np, Window win);

};

#endif /* KEY_H */
