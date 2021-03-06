#include <processor_startup.h>
#include <gst.h>
#include <lapic.h>
#include <ap_boot.h>
#include <screen.h>
#include <sync/spinlock.h>
#include <mem_manager_phys.h>
#include <mem_manager_virt.h>
#include <kernel_definitions.h>
#include <debug.h>
#include <utility.h>
#include <thread_sched.h>
#include <system.h>
#include <ipc/mailbox.h>

#include <clock/clock.h>

#include <elf.h>
#include <user_test/user_test.h>


extern uint32_t lock;  // lock, used to test spinlock functions when printing

// private functions and data

spinlock_t ready = 0;
spinlock_t process_ready = 0;

// ! this is not required -- delete
uint32_t get_stack();

void idle()
{
	printfln("EXECUTING IDLE THREAD ON CORE: %u", get_cpu_id);

	for(;;);
}

virtual_addr_t setup_processor_common_stack(uint8_t cpu_id)
{
    virtual_addr_t common_stack = alloc_perm() + 4096;

    per_cpu_write(PER_CPU_OFFSET(common_stack_top), common_stack);
    tss_set_kernel_stack(&get_cpu_storage(cpu_id)->tss_entry, common_stack, GDT_SS_ENTRY(cpu_id) * 8);

    return common_stack;
}

// startup the processor connected to 'lapic_id' and set it to execute at address 'exec_base'
void processor_startup(uint32_t lapic_id, physical_addr_t exec_base)
{
	// implement intel protocol for processor boot 
		
	// send the INIT interrupt and wait for 100ms
	lapic_send_ipi(get_gst()->lapic_base, lapic_id, 0, LAPIC_DELIVERY_INIT, LAPIC_DESTINATION_PHYSICAL, LAPIC_DESTINATION_TARGET);
	lapic_sleep(100);

	// send the STARTUP interrupt and wait for 1ms
	lapic_send_ipi(get_gst()->lapic_base, lapic_id, exec_base >> 12, LAPIC_DELIVERY_SIPI, 0, 0);
    lapic_sleep(1);
    // at this line processor has started spinning
}

// this function is called from the assembled 'ap_boot.fasm' after starting the processor
// GS register should be set to the processor's local data area, prior to calling this function
void setup_processor()
{
	// use the BSP page directory
	virt_mem_switch_directory(get_gst()->BSP_dir);

	idtr_install(&get_gst()->idtr);

	uint32_t id = get_cpu_id;
    printfln("processor %u is awake at stack %h", id, get_stack());

	lapic_enable(get_gst()->lapic_base);
	// calibrate the lapic timer of the AP
	lapic_calibrate_timer(get_gst()->lapic_base, 10, 64);

	// give the mark to the BSP to continue waking up processors
	release_spinlock(&ready);
	INT_ON;

    final_processor_setup();
}

// this function sets up higher level services like virtual memory, ipc... for the processor
// it is executed as common code by the APs and the BSP
void final_processor_setup()
{
	acquire_spinlock(&process_ready);   // wait for the processes to be initialized
    release_spinlock(&process_ready);   // release immediately so that other cpus can continue initialization of the scheduler
	
    // setup usermode kernel stack (one per processor)
    setup_processor_common_stack(get_cpu_id);

	get_cpu_storage(get_cpu_id)->mailbox = mailbox_create_static(get_cpu_id, MAILBOX_CPU, get_cpu_storage(get_cpu_id));

    thread_sched_t* scheduler = &get_cpu_storage(get_cpu_id)->scheduler;

    // initialize scheduler
    scheduler_init(scheduler);

    // create clock task
    TCB* clock_task = thread_create(get_process(KERNEL_PROCESS_SLOT), clock_task_entry_point, alloc_perm() + 4096, 0, 1, get_cpu_id);
	TCB* idle_thread = thread_create(get_process(KERNEL_PROCESS_SLOT), idle, alloc_perm() + 4096, 5, 1, get_cpu_id);

	mailbox_t* mbox = thread_alloc_mailbox_static(clock_task, get_gst()->processor_count + get_cpu_id);

	// ! usermode example with elf loading (from memory not from an actual file of course)
	// elf32_ehdr_t* hdr = user_test_bin;
	// TCB* user_task = thread_create(get_process(KERNEL_PROCESS_SLOT), hdr->e_entry >> 8, 0xD00000 + 4096, 0, 0, get_cpu_id);

	// physical_addr_t pa = virt_mem_create_address_space();
	// printfln("creating address space: %h", pa);

	// virt_mem_switch_directory(pa);

	// printfln("pa at: %h", pd_entry_get_frame(*virt_mem_get_page_directory_entry_by_addr(virt_mem_get_self_recursive_table(), virt_mem_get_foreign_recursive_table())));

	// virt_mem_alloc_page_f(1 GB, VIRT_MEM_DEFAULT_PTE_FLAGS | I86_PTE_USER);
	// virt_mem_get_current_directory()->entries[15] = 15;

	// printfln("table 15 value: %u", virt_mem_get_current_directory()->entries[15]);

	// virt_mem_switch_directory(get_gst()->BSP_dir);

	// printfln("table 15 value: %u", virt_mem_get_current_directory()->entries[15]);

	// if(elf_load(hdr) != ERROR_OK)
	// 	PANIC("Could not load elf");
	// PANIC("END");
	
    scheduler_add_ready(clock_task);
    scheduler_add_ready(idle_thread);
	// scheduler_add_ready(user_task);

	printfln("starting processor");

    scheduler_current_start();

    PANIC("SHOULD NOT REACH HERE");
}

// public functions

void startup_all_AP()
{
    // this is a hardcoded memory location required by the assembled 'ap_boot.fasm'
	memcpy(0x8000, ap_boot_bin, ap_boot_bin_len);       // copy the ap_boot code to the predefined memory address
	*(uint32_t*)0x8002 = setup_processor;               // target 'c' function address to be called by the assembler
	*(gdt_ptr_t*)0x8006 = get_gst()->gdtr;              // global descriptor table register address (processors share a common descriptor with different entries)

	// currently the stack for each processor is allocated using the physical memory manager 
    // startup all but the first (BSP) processors

	acquire_spinlock(&ready);
    for(uint32_t i = 1; i < get_gst()->processor_count; i++)
	{
		if(get_gst()->per_cpu_data_base[i].enabled == 0)
			continue;
		
		*(uint16_t*)0x800C = GDT_SS_ENTRY(i);                               // mark the gdt entry so that 'ap_boot.fasm' can set the SS register accordingly
		*(uint32_t*)0x800E = phys_mem_alloc_above_1mb() + 4096;				// reserve 4KB stack for each processor. must be identity mapped region

		processor_startup(get_gst()->per_cpu_data_base[i].id, 0x8000);

		// wait for the processor to gracefully boot 
		acquire_spinlock(&ready);
	}
}