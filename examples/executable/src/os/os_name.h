#ifndef OS_NAME_H
#define OS_NAME_H

/* Implemented once per platform: os_name_unix.c and os_name_win.c.
   idl only builds the file whose suffix matches the current system. */
const char *os_name(void);

#endif
