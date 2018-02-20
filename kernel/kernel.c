#include <screen.h>
#include <gdt.h>
#include <debug.h>
#include <acpi.h>
#include <pic.h>
#include <multiboot.h>
#include <phys_mem_manager.h>
#include <idt.h>
#include <ioapic.h>
#include <lapic.h>
#include <heap_manager.h>
#include <pit.h>

#include <ap_boot.h>

extern void* lapic_base;
extern void* ioapic_base;
extern uint32_t pit_count;

void kernel_main(multiboot_info_t* mbd, unsigned int magic)
{
	SetForegroundColor(VGA_COLOR_GREEN);
	SetBackgroundColor(VGA_COLOR_BLACK);
	ClearScreen();

	Print("Hello this is RadixOS\n");

	gdt_set_gate(0, 0, 0, 0, 0);
	gdt_set_gate(1, 0, 0xFFFFFFFF, GDT_RW | GDT_EX, GDT_SZ | GDT_GRAN);
	gdt_set_gate(2, 0, 0xFFFFFFFF, GDT_RW, GDT_SZ | GDT_GRAN);

	gdtr_install();

	idt_init();
	idtr_install();

	// disable the PIC so we can use the local apic
	pic_disable();

	// initialize physical memory manager
	if(phys_mem_manager_init(0x100000) != 0)
		PANIC("Could not initialize physical memory manager");

	if((mbd->flags & 1) == 0)
		PANIC("Cannot get memory length");
	if((mbd->flags && (1 << 6) == 0))
		PANIC("Cannot get memory map");

	printfln("GRUB detected %u KB lower and %u MB higher", mbd->mem_lower, mbd->mem_upper / 1024);

	multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(mbd->mmap_addr);
	int error = 0;

	for(; (uint32_t)entry < mbd->mmap_addr + mbd->mmap_length;)
	{
		if(entry->type == 1)		// this is available memory
		{
			// MMB is placed at 0x100000, so fix memory to not interfere
			if(entry->addr == 0x100000)
			{
				entry->addr += 0x1000;
				entry->len -= 0x1000;
			}
			if( (error = phys_mem_insert_region((physical_addr)entry->addr, (uint32_t)entry->len - (uint32_t)entry->len % 4096)) != 0)
			{
				printfln("error: %u", error);
				PANIC("Could not insert region into mmb");
			}
		}
			
		entry = (uint32_t)entry + entry->size + sizeof(entry->size);
	}

	phys_mem_print();

	// parse acpi tables !!!
	rsdp_descriptor_t* rsdp = rsdp_find();

	if(rsdp != 0)
		rsdp_print(rsdp);
	else
		PANIC("RSDP could not be found.. TODO: Parse mp tables");

	if(rsdp_parse(rsdp) != 0)
		PANIC("error occured during rsdp parsing!");

	pit_timer_init(1000);
	lapic_enable(lapic_base);
	ioapic_map_irq(ioapic_base, 0, 2, 64);

	ClearScreen();
	asm("sti");

	memcpy(0x8000, ap_boot_bin, ap_boot_bin_len);

	lapic_send_ipi(lapic_base, 1, 0, LAPIC_DELIVERY_INIT, 0, 0);
	pit_sleep(101);

	printfln("init sent");

	lapic_send_ipi(lapic_base, 1, 8, LAPIC_DELIVERY_SIPI, 0, 0);

	pit_sleep(2);

	printfln("processor ready");

	lapic_send_ipi(lapic_base, 1, 0, LAPIC_DELIVERY_NORMAL, 0, 0);

	heap_t* h = heap_create(0, 1000);
	int* ptr = heap_alloc_a(h, 4, 15);

	printfln("allocated ptr: %h", ptr);

	pit_sleep(2000);

	while(1)
	{
		int tempX = cursorX, tempY = cursorY;
		SetPointer(0, SCREEN_HEIGHT - 2);
		printfln("time: %u", pit_count);

		SetPointer(tempX, tempY);
	}
}