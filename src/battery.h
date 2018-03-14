/**
 * @file battery.h
 * @author Nick
 * @date 2018
 *
 * @brief Battery tray component.
 *
 */

#ifndef BATTERY_H
#define BATTERY_H

struct TrayComponentType;

/*@{*/
void InitializeBattery(void);
void StartupBattery(void);
#define ShutdownBattery() (void)(0)
void DestroyBattery(void);
/*@}*/

/** Create a battery component for the tray.
 * @param format The format of the battery.
 * @param zone The timezone of the battery (NULL for local time).
 * @param width The width of the battery (0 for auto).
 * @param height The height of the battery (0 for auto).
 */
struct TrayComponentType *CreateBattery(const char *format,
                                      const char *zone,
                                      int width, int height);

/** Add an action to a battery.
 * @param cp The battery.
 * @param action The action to take.
 * @param mask The mouse button mask.
 */
void AddBatteryAction(struct TrayComponentType *cp,
                    const char *action,
                    int mask);

#endif /* BATTERY_H */
