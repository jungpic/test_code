#ifndef __WATCHDOG_TRIGGER_H__
#define __WATCHDOG_TRIGGER_H__

int init_trigger_hw_watchdog(void);

int set_watchdog_trigger_none(void);
int set_watchdog_trigger_heartbeat(void);

#endif // __WATCHDOG_TRIGGER_H__
