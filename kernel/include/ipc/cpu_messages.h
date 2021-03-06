#ifndef CPU_MESSAGES_H_19112018
#define CPU_MESSAGES_H_19112018

#include <types.h>
#include <sync/semaphore.h>
#include <trap_frame.h>

// describes all the messages that can be sent to a cpu mailbox

typedef enum {
    CM_NONE,
    CM_RESCHEDULE,                  // triggers a reschedule on the target core
    CM_SEM_WAIT,                    // blocks the current thread at the given semaphore queue
    CM_AWAKEN_THREAD,               // wakes up the given thread and reschedules
} cpu_message_function_t;

// handles a message received by the cpu in the form of an ipi
PUBLIC void cpu_message_handler(trap_frame_t* regs);

#endif