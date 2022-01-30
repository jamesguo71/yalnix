#ifndef __TRAP_H
#define __TRAP_H

int trap_kernel(UserContext *context);
int trap_clock(UserContext *context);
int trap_illegal(UserContext *context);
int trap_memory(UserContext *context);
int trap_math(UserContext *context);
int trap_tty_receive(UserContext *context);
int trap_tty_transmit(UserContext *context);
int trap_disk(UserContext *context);
#endif