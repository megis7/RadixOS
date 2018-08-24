#include <mem_manager_virt.h>
#include <mem_manager_phys.h>
#include <debug.h>
#include <utility.h>
// #include "thread_sched.h"
// #include "spinlock.h"
// #include "print_utility.h"
// #include "file.h"
// #include "error.h"

// private data

pdirectory*	current_directory = 0;		// current page directory
physical_addr current_pdbr = 0;			// current page directory base register

pdirectory* kernel_directory = 0;		// kernel page directory

#pragma region HELPER FUNCTION
// when fault occured the page was present in memory
int page_fault_error_is_page_present(uint32_t error)
{
	return (error & 0x1);
}

// the fault occured due to a write attempt
int page_fault_error_is_write(uint32_t error)
{
	return (error & 0x2);
}

// the fault occured while the cpu was in CPL=3
int page_fault_error_is_user(uint32_t error)
{
	return (error & 0x4);
}

#pragma endregion

// top half page fault handler
// void page_fault(iregisters_t* regs)
// {
// 	uint32_t addr;
// 	_asm
// 	{
// 		mov eax, cr2
// 		mov dword ptr addr, eax
// 	}

// 	// push thread in the exception queue
// 	if (thread_get_current())
// 	{
// 		thread_exception te;
// 		te.exception_number = 14;
// 		te.target_thread = thread_get_current();
// 		te.data[0] = addr;
// 		te.data[1] = regs->err_code;

		
// 		if (!queue_spsc_insert(&thread_get_current()->exceptions, te))
// 			WARNING("queue_lf insertion error");
// 	}
// 	else
// 		PANIC("Page fault occured but threading is not enabled!");

// }

// uint32_t page_fault_calculate_present_flags(uint32_t area_flags)
// {
// 	uint32_t flags = I86_PDE_PRESENT;

// 	if (CHK_BIT(area_flags, MMAP_WRITE))
// 		flags |= I86_PDE_WRITABLE;

// 	if (CHK_BIT(area_flags, MMAP_USER))
// 		flags |= I86_PDE_USER;

// 	return flags;
// }

// void page_fault_alloc_page(uint32_t area_flags, virtual_addr address)
// {
// 	uint32_t flags = page_fault_calculate_present_flags(area_flags);

// 	if (CHK_BIT(area_flags, MMAP_IDENTITY_MAP))
// 		vmmngr_map_page(vmmngr_get_directory(), address, address, flags);
// 	else
// 		vmmngr_alloc_page_f(address, flags);
// }

// void page_fault_bottom(thread_exception te)
// {
// 	thread_exception_print(&te);
// 	uint32_t& addr = te.data[0];
// 	uint32_t& code = te.data[1];

// 	serial_printf("PAGE_FALUT: PROC: %u ADDRESS: %h, THREAD: %u, CODE: %h\n", process_get_current()->id, addr, thread_get_current()->id, code);

// 	if (process_get_current()->contract_spinlock == 1)
// 		PANIC("PAge fault spinlock is already reserved\n");

// 	spinlock_acquire(&process_get_current()->contract_spinlock);
// 	vm_area* p_area = vm_contract_find_area(&thread_get_current()->parent->memory_contract, addr);

// 	if (p_area == 0)
// 	{
// 		serial_printf("could not find address %h in memory contract", addr);

// 		page_fault_alloc_page(MMAP_WRITE | MMAP_ANONYMOUS | MMAP_PRIVATE, addr);
// 		spinlock_release(&process_get_current()->contract_spinlock);
// 		return;

// 		PANIC("");		// terminate thread and process with SIGSEGV
// 	}

// 	vm_area area = *p_area;
// 	spinlock_release(&process_get_current()->contract_spinlock);

// 	// tried to acccess inaccessible page
// 	if ((area.flags & MMAP_PROTECTION) == MMAP_NO_ACCESS)
// 	{
// 		serial_printf("address: %h is inaccessible\n", addr);
// 		PANIC("");
// 	}

// 	// tried to write to read-only or inaccessible page
// 	if (page_fault_error_is_write(code) && (area.flags & MMAP_WRITE) != MMAP_WRITE)
// 	{
// 		serial_printf("cannot write to address: %h\n", addr);
// 		PANIC("");
// 	}

// 	// tried to read a write-only or inaccesible page ???what???
// 	/*if (!page_fault_error_is_write(code) && CHK_BIT(area.flags, MMAP_READ))
// 	{
// 		serial_printf("cannot read from address: %h", addr);
// 		PANIC("");
// 	}*/

// 	// if the page is present then a violation happened (we do not implement swap out/shared anonymous yet)
// 	if (page_fault_error_is_page_present(code) == true)
// 	{
// 		serial_printf("memory violation at address: %h with code: %h\n", addr, code);
// 		serial_printf("area flags: %h\n", area.flags);
// 		PANIC("");
// 	}

// 	// here we found out that the page is not present, so we need to allocate it properly
// 	if (CHK_BIT(area.flags, MMAP_PRIVATE))
// 	{
// 		if (CHK_BIT(area.flags, MMAP_ALLOC_IMMEDIATE))
// 		{
// 			// loop through all addresses and map them
// 			for (virtual_addr address = area.start_addr; address < area.end_addr; address += 4096)
// 				//if (CHK_BIT(area.flags, MMAP_ANONYMOUS))	ALLOC_IMMEDIATE works only for anonymous (imposed in mmap)
// 				page_fault_alloc_page(area.flags, address);
// 		}
// 		else
// 		{
// 			if (CHK_BIT(area.flags, MMAP_ANONYMOUS))
// 				page_fault_alloc_page(area.flags, addr & (~0xFFF));
// 			else
// 			{
// 				uint32_t flags = page_fault_calculate_present_flags(area.flags);
// 				vmmngr_alloc_page_f(addr & (~0xFFF), flags);

// 				uint32_t read_start = area.offset + ((addr - area.start_addr) / PAGE_SIZE) * PAGE_SIZE;		// file read start
// 				uint32_t read_size = PAGE_SIZE;		// we read one page at a time (not the whole area as this may not be necessary)

// 				//if (read_start < area.start_addr + PAGE_SIZE)	// we are reading the first page so subtract offset from read_size
// 				//	read_size -= area.offset;

// 				serial_printf("gfd: %u, reading at mem: %h, phys: %h file: %h, size: %u\n", area.fd, addr & (~0xfff), vmmngr_get_phys_addr(addr & (~0xfff)),
// 					read_start, read_size);

// 				gfe* entry = gft_get(area.fd);
// 				if (entry == 0)
// 				{
// 					serial_printf("area.fd = %u", area.fd);
// 					PANIC("page fault gfd entry = 0");
// 				}

// 				// read one page from the file offset given at the 4KB-aligned fault address 
// 				if (read_file_global(area.fd, read_start, read_size, addr & (~0xFFF), VFS_CAP_READ | VFS_CAP_CACHE) != read_size)
// 				{
// 					serial_printf("read fd: %u\n", area.fd);
// 					PANIC("mmap anonymous file read less bytes than expected");
// 				}
// 			}
// 		}
// 	}
// 	else		// MMAP_SHARED
// 	{
// 		if (CHK_BIT(area.flags, MMAP_ANONYMOUS))
// 			PANIC("A shared area cannot be marked as anonymous yet.");
// 		else
// 		{
// 			// in the shared file mapping the address to read is ignored as data are read only to page cache. 
// 			uint32_t read_start = area.offset + ((addr & (~0xfff)) - area.start_addr);
// 			gfe* entry = gft_get(area.fd);

// 			if (read_file_global(area.fd, read_start, PAGE_SIZE, -1, VFS_CAP_READ | VFS_CAP_CACHE) != PAGE_SIZE)
// 				PANIC("mmap shared file failed");

// 			virtual_addr used_cache = page_cache_get_buffer(area.fd, read_start / PAGE_SIZE);
// 			//serial_printf("m%h\n", used_cache);

// 			uint32_t flags = page_fault_calculate_present_flags(area.flags);
// 			vmmngr_map_page(vmmngr_get_directory(), vmmngr_get_phys_addr(used_cache), addr & (~0xfff), flags/*DEFAULT_FLAGS*/);
// 			//serial_printf("shared mapping fd: %u, cache: %h, phys cache: %h, read: %u, addr: %h\n", area.fd, used_cache, used_cache, read_start, addr);
// 		}
// 	}
// }

// creates a page table for the dir address space
error_t vmmngr_create_table(pdirectory* dir, virtual_addr addr, uint32_t flags)
{
	pd_entry* entry = vmmngr_pdirectory_lookup_entry(dir, addr);
	if (!entry)
		return ERROR_OCCUR;

	ptable* table = (ptable*)phys_mem_alloc();
	if (!table)
		return ERROR_OCCUR;		// not enough memory!!

	memset(table, 0, sizeof(ptable));
	pd_entry_set_frame(entry, (physical_addr)table);
	pd_entry_add_attrib(entry, flags);

	return ERROR_OK;
}

error_t vmmngr_map_page(pdirectory* dir, physical_addr phys, virtual_addr virt, uint32_t flags)
{
	// our goal is to get the pt_entry indicated by virt and set its frame to phys.

	pd_entry* e = vmmngr_pdirectory_lookup_entry(dir, virt);

	if (!pd_entry_is_present(e))								// table is not present
		if (vmmngr_create_table(dir, virt, flags) != ERROR_OK)
			return ERROR_OCCUR;
		
	// here we have a guaranteed working table (perhaps empty)

	ptable* table = pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, virt);	// we have the page
	if (page == 0)
		return ERROR_OCCUR;
	
	*page = 0;												// remove previous flags
	*page |= flags;											// and set the new ones
	pt_entry_set_frame(page, phys);

	vmmngr_flush_TLB_entry(virt);							// TODO: inform the other cores of the mapping change

	return ERROR_OK;
}

error_t vmmngr_initialize(uint32_t kernel_pages)
{
	pdirectory* pdir = (pdirectory*)phys_mem_alloc();
	if (pdir == 0)
		return ERROR_OCCUR;

	kernel_directory = pdir;
	memset(pdir, 0, sizeof(pdirectory));

	physical_addr phys = 0;		// page directory structure is allocated at the beginning (<1MB) (false)
								// so identity map the first 4MB to be sure we can point to them

	for (uint32_t i = 0; i < 1024; i++, phys += 4096)
		if (vmmngr_map_page(pdir, phys, phys, DEFAULT_FLAGS) != ERROR_OK)
			return ERROR_OCCUR;

	phys = 0x100000;
	virtual_addr virt = 0xC0000000;

	for (uint32_t i = 0; i < kernel_pages; i++, virt += 4096, phys += 4096)
		if (vmmngr_map_page(pdir, phys, virt, DEFAULT_FLAGS) != ERROR_OK)
			return ERROR_OCCUR;

	if (vmmngr_switch_directory(pdir, (physical_addr)&pdir->entries) != ERROR_OK)
		return ERROR_OCCUR;

	// register_interrupt_handler(14, page_fault);
	// register_bottom_interrupt_handler(14, page_fault_bottom);

	return ERROR_OK;
}

error_t vmmngr_alloc_page(virtual_addr base)
{
	return vmmngr_alloc_page_f(base, DEFAULT_FLAGS);
}

error_t vmmngr_alloc_page_f(virtual_addr base, uint32_t flags)
{
	//TODO: cater for memory mapped IO where (in the most simple case) an identity map must be done.
	//TODO: fix this function
	physical_addr addr = base;

	// assume that 'base' is not already allocated

	if (base < 0xF0000000)		// memory mapped IO above some location...
	{
		addr = (physical_addr)phys_mem_alloc();
		if (addr == 0)
			return ERROR_OCCUR;
	}

	if (!addr)
	{
		// set_last_error(EINVAL, VMEM_BAD_ARGUMENT, EO_VMMNGR);
		return ERROR_OCCUR;
	}

	if (vmmngr_map_page(vmmngr_get_directory(), addr, base, flags) != ERROR_OK)
		return ERROR_OCCUR;

	return ERROR_OK;
}

error_t vmmngr_free_page(pt_entry* entry)
{
	physical_addr addr = pt_entry_get_frame(*entry);
	phys_mem_dealloc(addr);

	// TODO: make unmap function

	pt_entry_del_attrib(entry, I86_PTE_PRESENT);
	pt_entry_del_attrib(entry, I86_PTE_WRITABLE);

	return ERROR_OK;
}

error_t vmmngr_switch_directory(pdirectory* dir, physical_addr pdbr)
{
	// if the page directory hasn't change do not flush cr3 as such an action is a performance hit
	// if (pmmngr_get_PDBR() == pdbr)
	// 	return ERROR_OK;				TODO: this must be per-processor

	current_directory = dir;		// TODO: make these processor-specific
	current_pdbr = pdbr;

	// TODO: load pdbr

	// pmmngr_load_PDBR(current_pdbr);

	return ERROR_OK;
}

pdirectory* vmmngr_get_directory()
{
	return current_directory;
}

void vmmngr_flush_TLB_entry(virtual_addr addr)
{
	// TODO: How does this work on SMP ???
// 	_asm
// 	{
// 		cli
// 		invlpg addr					
// 		sti
// 	}
}

error_t vmmngr_ptable_clear(ptable* table)
{
	memset(table, 0, sizeof(ptable));
	phys_mem_dealloc((physical_addr)table);

	return ERROR_OK;
}

pt_entry* vmmngr_ptable_lookup_entry(ptable* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_TABLE_INDEX(addr)];

	// set_last_error(EINVAL, VMEM_BAD_ARGUMENT, EO_VMMNGR);
	return 0;
}

error_t vmmngr_pdirectory_clear(pdirectory* pdir)
{
	for (int i = 0; i < TABLES_PER_DIR; i++)
	{
		if (vmmngr_ptable_clear((ptable*)pd_entry_get_frame(pdir->entries[i])) != ERROR_OK)
			return ERROR_OCCUR;
	}

	memset(pdir, 0, sizeof(pdirectory));
	phys_mem_dealloc((physical_addr)pdir);		// TODO: This should be physical address, not virtual

	return ERROR_OK;
}

pd_entry* vmmngr_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_DIR_INDEX(addr)];

	// set_last_error(EINVAL, VMEM_BAD_ARGUMENT, EO_VMMNGR);
	return 0;
}

void vmmngr_print(pdirectory* dir)
{
	for (int i = 0; i < 1024; i++)
	{
		if (!pd_entry_test_attrib(&dir->entries[i], I86_PDE_PRESENT))
			continue;

		ptable* table = (ptable*)PAGE_GET_PHYSICAL_ADDR(&dir->entries[i]);

		printfln("table %i is present at %h", i, table);

		for (int j = 0; j < 1024; j++)
		{
			if (!pt_entry_test_attrib(&table->entries[j], I86_PTE_PRESENT))
				continue;

			printf("page %i is present with frame: %h ", j, pt_entry_get_frame(table->entries[j]));
		}
	}
}

physical_addr vmmngr_get_phys_addr(virtual_addr addr)
{
	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), addr);
	if (!e)
		return 0;

	ptable* table = pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, addr);
	if (!page)
		return 0;

	physical_addr p_addr = pt_entry_get_frame(*page);

	p_addr += (addr & 0xfff);		// add in-page offset
	return p_addr;
}

void vmmngr_free_page_addr(virtual_addr addr)
{
	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), addr);
	ptable* table = (ptable*)pd_entry_get_frame(*e);
	pt_entry* page = vmmngr_ptable_lookup_entry(table, addr);

	vmmngr_free_page(page);
}

int vmmngr_is_page_present(virtual_addr addr)
{
	pd_entry* e = vmmngr_pdirectory_lookup_entry(vmmngr_get_directory(), addr);
	if (e == 0 || !pd_entry_is_present(*e))
		return 0;

	ptable* table = (ptable*)pd_entry_get_frame(*e);
	if (table == 0)
		return 0;

	pt_entry* page = vmmngr_ptable_lookup_entry(table, addr);
	if (page == 0 || !pt_entry_is_present(*page) == 0)
		return 0;

	return 1;
}

pdirectory* vmmngr_create_address_space()
{
	pdirectory* dir = (pdirectory*)phys_mem_alloc();
	if (!dir)
		return 0;

	memset(dir, 0, sizeof(pdirectory));
	return dir;
}

error_t vmmngr_map_kernel_space(pdirectory* pdir)
{
	if (!pdir)
		return ERROR_OCCUR;

	memcpy(pdir, kernel_directory, sizeof(pdirectory));
	return ERROR_OK;
}

error_t vmmngr_switch_to_kernel_directory()
{
	return vmmngr_switch_directory(kernel_directory, (physical_addr)kernel_directory);
}

uint32_t vmmngr_get_page_size()
{
	return PAGE_SIZE;
}