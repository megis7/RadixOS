#include <mem_manager_virt.h>
#include <mem_manager_phys.h>
#include <debug.h>
#include <utility.h>

// private data

// These fields should be processor specific
pdirectory*	current_directory = 0;		// current page directory
physical_addr current_pdbr = 0;			// current page directory base register

pdirectory* kernel_directory = 0;		// kernel page directory
// ----------------------------------------------------------------------------------

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

#pragma region comment
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
#pragma endregion

// creates a page table for the dir address space
error_t virt_mem_create_table(pdirectory* dir, virtual_addr addr, uint32_t flags)
{
	pd_entry* entry = virt_mem_pdirectory_lookup_entry(dir, addr);
	if (!entry)
		return ERROR_OCCUR;

	ptable* table = (ptable*)phys_mem_alloc_above(0x120000);		// TODO: Investigate why memory above 1mb (alloced at 0x10A000) does not work
	if (!table)
		return ERROR_OCCUR;		// not enough memory!!

	memset(table, 0, sizeof(ptable));
	pd_entry_set_frame(entry, (physical_addr)table);
	pd_entry_add_attrib(entry, flags);

	return ERROR_OK;
}

// public functions

error_t virt_mem_init(uint32_t kernel_pages)
{
	pdirectory* pdir = (pdirectory*)phys_mem_alloc();
	if (pdir == 0)
		return ERROR_OCCUR;

	kernel_directory = pdir;
	memset(pdir, 0, sizeof(pdirectory));

	physical_addr phys = 0;		// page directory structure is allocated at the beginning (<1MB) (false)
								// so identity map the first 4MB to be sure we can point to them

	for (uint32_t i = 0; i < 1024; i++, phys += 4096)
		if (virt_mem_map_page(pdir, phys, phys, VIRT_MEM_DEFAULT_FLAGS) != ERROR_OK)
			return ERROR_OCCUR;

	phys = 0x100000;
	virtual_addr virt = 0xC0000000;

	for (uint32_t i = 0; i < kernel_pages; i++, virt += 4096, phys += 4096)
		if (virt_mem_map_page(pdir, phys, virt, VIRT_MEM_DEFAULT_FLAGS) != ERROR_OK)
			return ERROR_OCCUR;

	if (virt_mem_switch_directory(pdir, (physical_addr)&pdir->entries) != ERROR_OK)
		return ERROR_OCCUR;

	// register_interrupt_handler(14, page_fault);
	// register_bottom_interrupt_handler(14, page_fault_bottom);

	return ERROR_OK;
}

error_t virt_mem_map_page(pdirectory* dir, physical_addr phys, virtual_addr virt, uint32_t flags)
{
	// our goal is to get the pt_entry indicated by virt and set its frame to phys.

	pd_entry* e = virt_mem_pdirectory_lookup_entry(dir, virt);

	if (!pd_entry_is_present(e))								// table is not present
		if (virt_mem_create_table(dir, virt, flags) != ERROR_OK)
			return ERROR_OCCUR;

	// here we have a guaranteed working table (perhaps empty)

	ptable* table = pd_entry_get_frame(*e);
	pt_entry* page = virt_mem_ptable_lookup_entry(table, virt);	// we have the page
	if (page == 0)
		return ERROR_OCCUR;
	
	*page = 0;												// remove previous flags
	*page |= flags;											// and set the new ones
	pt_entry_set_frame(page, phys);

	// TODO: This must happen here only when dir is the current page directory
	//virt_mem_flush_TLB_entry(virt);							// TODO: inform the other cores of the mapping change

	return ERROR_OK;
}

error_t virt_mem_unmap_page(pdirectory* dir, virtual_addr virt)
{
	pd_entry* e = virt_mem_pdirectory_lookup_entry(dir, virt);
	ptable* table = pd_entry_get_frame(*e);
	pt_entry* page = virt_mem_ptable_lookup_entry(table, virt);

	*page = 0;

	return ERROR_OK;
}

error_t virt_mem_alloc_page(virtual_addr base)
{
	return virt_mem_alloc_page_f(base, VIRT_MEM_DEFAULT_FLAGS);
}

error_t virt_mem_alloc_page_f(virtual_addr base, uint32_t flags)
{
	// assume that 'base' is not already allocated

	physical_addr addr = (physical_addr)phys_mem_alloc();
	if (addr == 0)
		return ERROR_OCCUR;

	if (virt_mem_map_page(virt_mem_get_directory(), addr, base, flags) != ERROR_OK)
		return ERROR_OCCUR;

	// TODO: TLB shootdown

	return ERROR_OK;
}

error_t virt_mem_free_page(virtual_addr base)
{
	physical_addr addr = virt_mem_get_phys_addr(base);
	if(addr == 0)
		return ERROR_OCCUR;
	
	if(virt_mem_unmap_page(virt_mem_get_directory(), base) != ERROR_OK)
		return ERROR_OCCUR;

	phys_mem_dealloc(addr);		// TODO: Add error checking

	// TODO: TLB shootdown

	return ERROR_OK;
}

error_t virt_mem_switch_directory(pdirectory* dir, physical_addr pdbr)
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

pdirectory* virt_mem_get_directory()
{
	return current_directory;
}

void virt_mem_flush_TLB_entry(virtual_addr addr)
{
	asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

error_t virt_mem_ptable_clear(ptable* table)
{
	memset(table, 0, sizeof(ptable));
	phys_mem_dealloc((physical_addr)table);

	return ERROR_OK;
}

pt_entry* virt_mem_ptable_lookup_entry(ptable* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_TABLE_INDEX(addr)];

	// set_last_error(EINVAL, VMEM_BAD_ARGUMENT, EO_virt_mem);
	return 0;
}

error_t virt_mem_pdirectory_clear(pdirectory* pdir)
{
	for (int i = 0; i < TABLES_PER_DIR; i++)
	{
		if (virt_mem_ptable_clear((ptable*)pd_entry_get_frame(pdir->entries[i])) != ERROR_OK)
			return ERROR_OCCUR;
	}

	memset(pdir, 0, sizeof(pdirectory));
	phys_mem_dealloc((physical_addr)pdir);		// TODO: This should be physical address, not virtual

	return ERROR_OK;
}

pd_entry* virt_mem_pdirectory_lookup_entry(pdirectory* p, virtual_addr addr)
{
	if (p)
		return &p->entries[PAGE_DIR_INDEX(addr)];

	// set_last_error(EINVAL, VMEM_BAD_ARGUMENT, EO_virt_mem);
	return 0;
}

void virt_mem_print(pdirectory* dir)
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

// checked
physical_addr virt_mem_get_phys_addr(virtual_addr addr)
{
	pd_entry* e = virt_mem_pdirectory_lookup_entry(virt_mem_get_directory(), addr);
	if (!e)
		return 0;

	ptable* table = pd_entry_get_frame(*e);
	pt_entry* page = virt_mem_ptable_lookup_entry(table, addr);
	if (!page)
		return 0;

	physical_addr p_addr = pt_entry_get_frame(*page);

	p_addr += (addr & 0xfff);		// add in-page offset
	return p_addr;
}

// checked
int virt_mem_is_page_present(virtual_addr addr)
{
	pd_entry* e = virt_mem_pdirectory_lookup_entry(virt_mem_get_directory(), addr);
	if (e == 0 || !pd_entry_is_present(*e))
		return 0;

	ptable* table = (ptable*)pd_entry_get_frame(*e);
	if (table == 0)
		return 0;

	pt_entry* page = virt_mem_ptable_lookup_entry(table, addr);
	if (page == 0 || !pt_entry_is_present(*page) == 0)
		return 0;

	return 1;
}

// checked
physical_addr virt_mem_create_address_space()
{
	pdirectory* dir = (pdirectory*)phys_mem_alloc_above_1mb();
	if (!dir)
		return 0;

	memset(dir, 0, sizeof(pdirectory));
	return (physical_addr)dir;
}

// performs shallow copy
error_t virt_mem_map_kernel_space(pdirectory* pdir)
{
	if (!pdir)
		return ERROR_OCCUR;

	memcpy(pdir, kernel_directory, sizeof(pdirectory));
	return ERROR_OK;
}

// not necessary
error_t virt_mem_switch_to_kernel_directory()
{
	return virt_mem_switch_directory(kernel_directory, (physical_addr)kernel_directory);
}

uint32_t virt_mem_get_page_size()
{
	return PAGE_SIZE;
}