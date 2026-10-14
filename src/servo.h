
#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>

/*
 * Initialize the servo module.
 *
 * Returns 0 on success, or a negative error code on failure.
 */
int servo_init(void);

/* Request the hatch to open. */
int servo_open(void);

/* Request the hatch to close. */
int servo_close(void);

/* Toggle between open and closed. */
int servo_toggle(void);

/* Return the last commanded hatch state. */
bool servo_is_open(void);

#endif /* SERVO_H */
