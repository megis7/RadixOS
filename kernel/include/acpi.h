#ifndef ACPI_H_13022018
#define ACPI_H_13022018

#include <types.h>
#include <stddef.h>

#define ACPI_MADT	"APIC"		// Multiple APIC descriptor table		(number of processors)
#define ACPI_FADT	"FACP"		// Fixed ACPI descriptor table			(power management)
#define ACPI_SRAT	"SRAT"		// System ressource affinity			(NUMA)
#define ACPI_SLIT	"SLIT"		// System locality information table	(NUMA)

#define ACPI_HPET 	"HPET"		// High Performace Event Timer

enum MADT_TYPE
{
	LAPIC = 0,
	IOAPIC,
	INTERRUPT_OVERRIDE
};

// defines the layout of the acpi root system descriptor pointer
typedef struct rsdp_descriptor_struct_t 
{
    uint8_t signature[8];       // the signature of the descriptor table (used to locate it in RAM)
    uint8_t csum;               // the checksum to ensure correctness
    uint8_t oem_id[6];          
    uint8_t revision;           // the acpi revision (1, 2 ...)
    physical_addr_t rsdt_addr;    // a pointer to the root system descriptor table

} rsdp_descriptor_t;

// defines the layout of a acpi descriptor table header, that is common to all acpi tables
typedef struct acpi_dt_hdr_struct_t
{
	uint8_t signature[4];			// the unique signature of each table (used to locate the table we want to work with)
	uint32_t length;				// the total size of the table, inclusive of the header. 
	uint8_t revision;
	uint8_t csum;					// the checksum to ensure correctness
	uint8_t oem_id[6];
	uint8_t oem_table_id[8];
	uint32_t oem_revision;
	uint32_t create_id;
	uint32_t creator_revision;

} acpi_dt_header_t;

typedef struct rsdt_descriptor_struct_t 
{
	acpi_dt_header_t acpi_header;		// the common acpi header
	uint32_t sdt_ptrs[];				// pointer to the other SDTs (ex. FADT MADT ...)

} rsdt_descriptor_t;

typedef struct madt_descriptor_struct_t
{
	acpi_dt_header_t acpi_header;
	physical_addr_t lapic_addr;			// local APIC MMIO address
	uint32_t flags;						// if bit 1 is set => legacy PIC is installed so mask all of its interrupts

} madt_descriptor_t;

typedef struct madt_entry_header_struct_t
{
	uint8_t entry_type;			// the type of the entry that follows this header
	uint8_t entry_length;		// the length of the entry

} madt_entry_header_t;

// ENTRY TYPE: 0, describes a local APIC entry (expect one lapic per processor)
typedef struct madt_lapic_descriptor_struct_t
{
	madt_entry_header_t header;

	uint8_t processor_id;			// the id of the processor associated with the lapic
	uint8_t apic_id;				// the id of the lapic
	uint32_t flags;					// if set then processor is enabled, otherwise disabled and we should not access it

} madt_lapic_descriptor_t;

// ENTRY TPYE: 1, describes an I/O APIC entry (expect one per system for our kernel)
typedef struct madt_ioapic_descriptor_struct_t
{
	madt_entry_header_t header;

	uint8_t ioapic_id;					// the id of the ioapic
	uint8_t resv0;
	physical_addr_t ioapic_addr;			// the physical address of the ioapic
	uint32_t global_interrupt_base;

} madt_ioapic_descriptor_t;

// ENTRY TYPE: 2, describes interrupt source override
typedef struct madt_interrupt_override_descriptor_struct_t
{
	madt_entry_header_t header;

	uint8_t bus_source;
	uint8_t irq_source;
	uint32_t global_system_int;
	uint16_t flags;
} madt_interrupt_override_descriptor_t;

typedef struct hpet_descriptor_struct_t
{
	acpi_dt_header_t acpi_header;

	uint8_t hardware_rev_id;
    uint8_t comparator_count:5;
    uint8_t counter_size:1;
    uint8_t reserved:1;
    uint8_t legacy_replacement:1;
    uint16_t pci_vendor_id;

    uint8_t address_space_id;    // 0 - system memory, 1 - system I/O
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved1;
    uint64_t address;

    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;

} hpet_descriptor_t;

// searches the possible locations of RAM for a valid RDSP struct
rsdp_descriptor_t* rsdp_find();

// parses a root system descriptor pointer, examining its contained tables (ex. MADT)
int rsdp_parse(rsdp_descriptor_t* rsdp);

// perform first parse of the root system descriptor pointer, enumerating system core resources
int rsdp_first_parse(rsdp_descriptor_t* rsdp);

void rsdp_print(rsdp_descriptor_t* rsdp);

#endif