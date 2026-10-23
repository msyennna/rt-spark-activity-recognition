#ifndef RTSPARK_MQTT_H
#define RTSPARK_MQTT_H

/* Foreground-only; each poll advances one bounded connection operation. */
void rtspark_mqtt_init(void);
void rtspark_mqtt_poll(void);
/* Copies a fresh sample only when online and no QoS 1 delivery is pending.
 * Offline/busy samples are skipped; reconnect never replays stale motion. */
int rtspark_mqtt_offer(const char *json);

#endif
