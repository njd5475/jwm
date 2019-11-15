/**
 * @file battery.h
 * @author Nick
 * @date 2017
 *
 * @brief Battery tray component.
 *
 */

#ifndef BATTERY_H
#define BATTERY_H

#include "tray.h"

class Battery : public TrayComponentType {
public:

	/*@{*/
	static void InitializeBattery(void);
	static void StartupBattery(void);
	static void ShutdownBattery() {}
	static void DestroyBattery(void);
	/*@}*/

public:
	Battery(int width, int height);
	virtual ~Battery();

	/**
	 * Add an action to a battery.
	 *
	 * @param cp The battery.
	 * @param action The action to take.
	 * @param mask The mouse button mask.
	 */
	void AddBatteryAction(const char *action, int mask);

	void Create();
	void Resize();
	void Draw();

private:
	float lastLevel; /**< Currently displayed level */

};

#endif /* BATTERY_H */
