#ifndef IOAPIC_H_17022018
#define IOAPIC_H_17022018

#include <types.h>

enum IOAPIC_REGS
{
    IOAPIC_INDEX = 0,           // index register used to access other registers
    IOAPIC_DATA = 0x10,         // data register used to communicate data to/from the selected register
};

// Describes the index that must be written to the INDEX register to access the corresponding IOAPIC register 
enum IOAPIC_INDEXED_REGS
{
    IOAPIC_ID = 0,              // bits 24-27 contain the io apic ID
    IOAPIC_VER = 1,             // bits 0-8 contain the version of the apic and bits 16-23 the maximum number of IRQs that can be handled (redirection entries)

    IOAPIC_REDTBL0_LOW = 0x10,       // following this index are the actual redirection entries (2 x 32 bit each)
    IOAPIC_REDTBL0_HIGH = 0x11
};

/* Describes the way that a the interrupt will be sent to the lapic */
enum IOAPIC_DELIVERY_MODE
{
    IOAPIC_DELIVERY_FIXED = 0,                  // Normal interrupt delivery to the designated vector
    IOAPIC_DELIVERY_LOWEST = 1 << 8,            // Delivers the irq to the lowest priority executing cpu
    IOAPIC_DELIVERY_SMI = 2 << 8,
    IOAPIC_DELIVERY_NMI = 4 << 8,
    IOAPIC_DELIVERY_INIT = 5 << 8,
    IOAPIC_DELIVERY_EXT = 7 << 8                // Handle as if local interrupt was generated by external device
};

/* Describes the destination mode to use when sending the interrupt */
enum IOAPIC_DESTINATION_MODE
{
    IOAPIC_DESTINATION_PHYSICAL = 0,             // Uses the processor ID supplied to find the target processor
    IOAPIC_DESTINATION_LOGICAL = 1 << 11         
};

/* Describes the polarity of the interrupt */
enum IOAPIC_POLARITY_MODE 
{
    IOAPIC_POLARITY_HIGH = 0,                    // Interrupt is active high (assume high unless otherwise specified in the override descriptors) 
    IOAPIC_POLARITY_LOW = 1 << 13                // Interrupt is active low
};

/* Describes the trigger mode of the interrupt */
enum IOAPIC_TRIGGER_MODE 
{
    IOAPIC_TRIGGER_EDGE = 0,
    IOAPIC_TRIGGER_LEVEL = 1 << 15
};

#define IOAPIC_INT_MASK (1 << 16)

// Destination field: Physical Destination allows only bits 56-59 for the cpu ID (only 16 processors are addressable)

// read data from register 'reg'
uint32_t ioapic_read(physical_addr ioapic_base, uint32_t reg);

// write data to register 'reg'
void ioapic_write(physical_addr ioapic_base, uint32_t reg, uint32_t val);

// maps the given 'irq' to the given 'vector' number that will be routed to the 'apic_id' processor (int_mode combines trigger and polarity of the interrupt)
void ioapic_map_irq(physical_addr ioapic_base, uint32_t apic_id, uint8_t irq, uint8_t vector, uint32_t delivery_mode, uint32_t destination_mode, uint32_t int_mode);

#endif