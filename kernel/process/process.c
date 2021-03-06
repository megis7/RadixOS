#include <process/process.h>
#include <mem_alloc.h>
#include <string.h>
#include <gst.h>
#include <sync/spinlock.h>

// private data and functions

// process table.
PCB* process_slots;
spinlock_t ps_lock;			// process slots lock

// thread table
TCB* thread_slots;
spinlock_t ts_lock;			// thread slots lock

#define for_all_process_slots(x) \
PCB* x = &process_slots[0];	\
for(uint32_t i = 0; i < MAX_PROCESS_SLOTS; i++, x = &process_slots[i])

// public functions

void process_init()
{
    uint32_t process_pages = ceil_division(MAX_PROCESS_SLOTS * sizeof(PCB), virt_mem_get_page_size());
	uint32_t thread_pages = ceil_division(MAX_THREAD_SLOTS * sizeof(TCB), virt_mem_get_page_size());

	ps_lock = ts_lock = 0;

	// allocate enough virtual space right after the kernel for the process and the thread slots
	process_slots = alloc_perm();
	for(uint32_t i = 0; i < process_pages - 1; i++)
		alloc_perm();

	thread_slots = alloc_perm();
	for(uint32_t i = 0; i < thread_pages - 1; i++)
		alloc_perm();

	// mark all processes and threads as empty and ready for use
	for(uint32_t i = 0; i < MAX_PROCESS_SLOTS; i++)
	{
		process_slots[i].flags = PROCESS_SLOT_EMPTY;
		process_slots[i].pid = -1;
	}
			
	for(uint32_t i = 0; i < MAX_THREAD_SLOTS; i++)
	{
		thread_slots[i].flags = THREAD_SLOT_EMPTY;
		thread_slots[i].tid = -1;
	}
}

PCB* process_create_static(PCB* parent, physical_addr_t pdir, uint8_t name[16], pid_t pid)
{
	acquire_spinlock(&ps_lock);
	// fail when the slot is already occupied
	if(!(process_slots[pid].flags & PROCESS_SLOT_EMPTY))
	{
		release_spinlock(&ps_lock);
		return 0;
	}

	PCB* new_pcb = &process_slots[pid];
	new_pcb->flags = PROCESS_NEW;

	release_spinlock(&ps_lock);

	new_pcb->pid = pid;
	new_pcb->parent = parent;

	new_pcb->address_space.p_page_directory = pdir;
	new_pcb->address_space.lock = 0;
	
	strcpy_s(&new_pcb->name, 16, name);
	vm_contract_init(&new_pcb->memory_contract);

	return new_pcb;
}

PCB* process_create(PCB* parent, physical_addr_t pdbr, uint8_t name[16])
{
	// find an empty slot in the process slot table
	PCB* new_pcb = 0;
	for(uint32_t i = 0; i < MAX_PROCESS_SLOTS; i++)
	{
		new_pcb = process_create_static(parent, pdbr, name, i);

		if(new_pcb)
			return new_pcb;
	}

	return 0;
}

PCB* get_process(pid_t pid)
{
    if(process_slots[pid].flags & PROCESS_SLOT_EMPTY)
        return 0;

    return &process_slots[pid];
}

TCB* thread_create_static(PCB* parent, virtual_addr_t entry_point, virtual_addr_t stack_top, uint32_t priority, tid_t tid, uint8_t is_kernel, uint8_t exec_cpu)
{
	acquire_spinlock(&ts_lock);

    // fail when the slot is already occupied
    if(!(thread_slots[tid].flags & THREAD_SLOT_EMPTY))
	{
		release_spinlock(&ts_lock);
        return 0;
	}

    TCB* new_tcb = &thread_slots[tid];
    new_tcb->flags = THREAD_NEW;

	release_spinlock(&ts_lock);

    new_tcb->tid = tid;
    new_tcb->parent = parent;
    new_tcb->priotity = priority;
    new_tcb->is_kernel = is_kernel;
    new_tcb->exec_cpu = exec_cpu;
	new_tcb->mailbox = 0;
    new_tcb->next = new_tcb->prev = 0;
    
    if(is_kernel)
        trap_frame_init_kernel(&new_tcb->kframe, entry_point, stack_top, exec_cpu);
    else
        trap_frame_init_user(&new_tcb->frame, entry_point, get_cpu_storage(exec_cpu)->common_stack_top, stack_top);

    return new_tcb;
}

TCB* thread_create(PCB* parent, virtual_addr_t entry_point, virtual_addr_t stack_top, uint32_t priority, uint8_t is_kernel, uint8_t exec_cpu)
{
    // find an empty slot in the thread slot table
	TCB* new_tcb = 0;

	for(uint32_t i = 0; i < MAX_THREAD_SLOTS; i++)
	{
		new_tcb = thread_create_static(parent, entry_point, stack_top, priority, i, is_kernel, exec_cpu);

		if(new_tcb)
			return new_tcb;
	}

	return 0;
}

TCB* get_thread(tid_t tid)
{
	if(thread_slots[tid].flags & THREAD_SLOT_EMPTY)
        return 0;

    return &thread_slots[tid];
}

mailbox_t* thread_alloc_mailbox(TCB* thread)
{
	mailbox_t* mbox = mailbox_create(MAILBOX_THREAD, thread);
	thread->mailbox = mbox;
	return mbox;
}

mailbox_t* thread_alloc_mailbox_static(TCB* thread, mid_t mid)
{
	mailbox_t* mbox = mailbox_create_static(mid, MAILBOX_THREAD, thread);
	thread->mailbox = mbox;
	return mbox;
}