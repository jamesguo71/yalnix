#ifndef __SCHEDULER_H
#define __SCHEDULER_H
#include <hardware.h>
#include "process.h"

#define SCHEDULER_CVAR_START       0
#define SCHEDULER_CVAR_END         1
#define SCHEDULER_DELAY_START      2
#define SCHEDULER_DELAY_END        3
#define SCHEDULER_LOCK_START       4
#define SCHEDULER_LOCK_END         5
#define SCHEDULER_PIPE_READ_START  6
#define SCHEDULER_PIPE_READ_END    7
#define SCHEDULER_PIPE_WRITE_START 8
#define SCHEDULER_PIPE_WRITE_END   9
#define SCHEDULER_PROCESSES_START  10
#define SCHEDULER_PROCESSES_END    11
#define SCHEDULER_READY_START      12
#define SCHEDULER_READY_END        13
#define SCHEDULER_TERMINATED_START 14
#define SCHEDULER_TERMINATED_END   15
#define SCHEDULER_TTY_READ_START   16
#define SCHEDULER_TTY_READ_END     17
#define SCHEDULER_TTY_WRITE_START  18
#define SCHEDULER_TTY_WRITE_END    19
#define SCHEDULER_WAIT_START       20
#define SCHEDULER_WAIT_END         21
#define SCHEDULER_RUNNING          22
#define SCHEDULER_IDLE             23
#define SCHEDULER_NUM_LISTS        24


typedef struct scheduler scheduler_t;


/*!
 * \desc    Initializes memory for a new scheduler_t struct
 *
 * \return  An initialized scheduler_t struct, NULL otherwise.
 */
scheduler_t *SchedulerCreate();


/*!
 * \desc                  Frees the memory associated with a scheduler_t struct
 *
 * \param[in] _scheduler  A scheduler_t struct that the caller wishes to free
 */
int SchedulerDelete(scheduler_t *_scheduler);

int    SchedulerAddCVar(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddDelay(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddIdle(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddLock(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddPipeRead(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddPipeWrite(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddProcess(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddReady(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddRunning(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddTerminated(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddTTYRead(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddTTYWrite(scheduler_t *_scheduler, pcb_t *_process);
int    SchedulerAddWait(scheduler_t *_scheduler, pcb_t *_process);

pcb_t *SchedulerGetIdle(scheduler_t *_scheduler);
pcb_t *SchedulerGetProcess(scheduler_t *_scheduler, int _pid);
pcb_t *SchedulerGetReady(scheduler_t *_scheduler);
pcb_t *SchedulerGetRunning(scheduler_t *_scheduler);
pcb_t *SchedulerGetTerminated(scheduler_t *_scheduler, int _pid);
pcb_t *SchedulerGetTTYWrite(scheduler_t *_scheduler);
pcb_t *SchedulerGetWait(scheduler_t *_scheduler, int _pid);

int    SchedulerPrintCVar(scheduler_t *_scheduler);
int    SchedulerPrintDelay(scheduler_t *_scheduler);
int    SchedulerPrintLock(scheduler_t *_scheduler);
int    SchedulerPrintPipeRead(scheduler_t *_scheduler);
int    SchedulerPrintPipeWrite(scheduler_t *_scheduler);
int    SchedulerPrintProcess(scheduler_t *_scheduler);
int    SchedulerPrintReady(scheduler_t *_scheduler);
int    SchedulerPrintTerminated(scheduler_t *_scheduler);
int    SchedulerPrintTTYRead(scheduler_t *_scheduler);
int    SchedulerPrintTTYWrite(scheduler_t *_scheduler);
int    SchedulerPrintWait(scheduler_t *_scheduler);

int    SchedulerRemoveCVar(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveDelay(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveLock(scheduler_t *_scheduler, int _pid);
int    SchedulerRemovePipeRead(scheduler_t *_scheduler, int _pid);
int    SchedulerRemovePipeWrite(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveProcess(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveReady(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveTerminated(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveTTYRead(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveTTYWrite(scheduler_t *_scheduler, int _pid);
int    SchedulerRemoveWait(scheduler_t *_scheduler, int _pid);

int    SchedulerUpdateCVar(scheduler_t *_scheduler, int _cvar_id);
int    SchedulerUpdateDelay(scheduler_t *_scheduler);
int    SchedulerUpdateLock(scheduler_t *_scheduler, int _lock_id);
int    SchedulerUpdatePipeRead(scheduler_t *_scheduler, int _pipe_id);
int    SchedulerUpdatePipeWrite(scheduler_t *_scheduler, int _pipe_id, int _write_pid);
int    SchedulerUpdateTerminated(scheduler_t *_scheduler, pcb_t *_parent);
int    SchedulerUpdateTTYRead(scheduler_t *_scheduler, int _tty_id);
int    SchedulerUpdateTTYWrite(scheduler_t *_scheduler, int _tty_id, int _write_pid);
int    SchedulerUpdateWait(scheduler_t *_scheduler, int _pid);
#endif // __SCHEDULER_H