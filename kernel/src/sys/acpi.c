#include "include/sys/acpi.h" 
#include "include/drivers/serial.h"
#include "include/drivers/io.h"
#include "include/memory/paging.h" 
#include "libc/string.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h> 

extern volatile struct limine_rsdp_request rsdp_request; 
struct acpi_state acpi_state = {0};

static void* acpi_map_physical_to_virtual(uint64_t physical_addr) {
    if (physical_addr == 0) {
        return NULL;
    }
    void* virtual_addr = paging_physical_to_virtual(physical_addr);

    uint64_t page_virtual = (uint64_t)virtual_addr & ~0xFFF;
    uint64_t page_physical = physical_addr & ~0xFFF;

    if (!paging_is_mapped(page_virtual)) {
        uint64_t flags = PAGING_PRESENT | PAGING_WRITABLE | PAGING_NO_EXECUTE;
        if (!paging_map_page(page_virtual, page_physical, flags)) {
            serial_puts("[ACPI] ERROR: Failed to map page for physical address 0x");
            char hex_buf[17];
            serial_puts(itoa(physical_addr, hex_buf, 16));
            serial_putc('\n');
            return NULL;
        }
    } 

    return virtual_addr;
}

void acpi_init(volatile struct limine_hhdm_response *hhdm_response) {
    serial_puts("[ACPI] Initializing ACPI...");

    if (!hhdm_response) {
        serial_puts("[ACPI] ERROR: HHDM response is NULL!");
        return;
    }

    if (!rsdp_request.response) {
        serial_puts("[ACPI] ERROR: No RSDP response from bootloader!");
        return;
    }

    if (!rsdp_request.response->address) {
        serial_puts("[ACPI] ERROR: No RSDP address provided!");
        return;
    }

    serial_puts("[ACPI] RSDP address reported by Limine");

    uint64_t rsdp_phys_addr = rsdp_request.response->address;
    serial_puts("[ACPI] RSDP physical address: ");
    char hex_buf[17];
    serial_puts(itoa(rsdp_phys_addr, hex_buf, 16));
    serial_putc('\n');

    void* rsdp_virt_ptr = acpi_map_physical_to_virtual(rsdp_phys_addr);

    serial_puts("[ACPI] Calculated RSDP virtual address: ");
    serial_puts(itoa((uint64_t)rsdp_virt_ptr, hex_buf, 16));
    serial_putc('\n');

    if (rsdp_virt_ptr == NULL) {
        serial_puts("[ACPI] ERROR: acpi_map_physical_to_virtual failed for RSDP address!");
        return;
    }

    acpi_state.rsdp = (struct acpi_rsdp*)rsdp_virt_ptr;

    serial_puts("[ACPI] RSDP virtual address assigned");

    volatile uint8_t first_byte = acpi_state.rsdp->signature[0];
    serial_puts("[ACPI] First byte of signature read: ");
    serial_putc(first_byte);
    serial_putc(' ');
    serial_puts(itoa(first_byte, hex_buf, 16));
    serial_putc('\n');

    if (strncmp(acpi_state.rsdp->signature, ACPI_RSDP_SIGNATURE, 8) != 0) {
        serial_puts("[ACPI] ERROR: Invalid RSDP signature!");
        return;
    }

    serial_puts("[ACPI] RSDP signature valid");

    size_t rsdp_length = (acpi_state.rsdp->revision == 0) ? 20 : acpi_state.rsdp->length;
    if (!acpi_checksum_valid(acpi_state.rsdp, rsdp_length)) {
        serial_puts("[ACPI] ERROR: Invalid RSDP checksum!");
        return;
    }

    serial_puts("[ACPI] RSDP checksum valid");

    if (acpi_state.rsdp->revision >= 2 && acpi_state.rsdp->xsdt_address != 0) {
        acpi_state.xsdt = (struct acpi_xsdt*)acpi_map_physical_to_virtual(acpi_state.rsdp->xsdt_address);
        if (acpi_state.xsdt) {
            serial_puts("[ACPI] XSDT found and mapped");
            acpi_state.use_xsdt = 1;
        } else {
            serial_puts("[ACPI] WARNING: XSDT address invalid, falling back to RSDT");
        }
    } else {
        acpi_state.rsdt = (struct acpi_rsdt*)acpi_map_physical_to_virtual(acpi_state.rsdp->rsdt_address);
        if (acpi_state.rsdt) {
            serial_puts("[ACPI] RSDT found and mapped");
            acpi_state.use_xsdt = 0;
        } else {
            serial_puts("[ACPI] ERROR: Both XSDT and RSDT are invalid!");
            return;
        }
    }

    acpi_state.fadt = (struct acpi_fadt*)acpi_find_table(ACPI_FADT_SIGNATURE, hhdm_response);
    if (acpi_state.fadt) {
        serial_puts("[ACPI] FADT found");
        // serial_puts("[ACPI] FADT Length: ");
        // serial_put_hex64(acpi_state.fadt->header.length); 
        // serial_putc('\n');
    } else {
        serial_puts("[ACPI] WARNING: FADT not found (ACPI shutdown/reboot unavailable)"); 
    }

    acpi_state.madt = (struct acpi_madt*)acpi_find_table(ACPI_MADT_SIGNATURE, hhdm_response);
    if (acpi_state.madt) {
        serial_puts("[ACPI] MADT found\n");
        // serial_puts("[ACPI] MADT Length: ");
        // serial_put_hex64(acpi_state.madt->header.length); 
        // serial_putc('\n');
    } else {
        serial_puts("[ACPI] WARNING: MADT not found (APIC might not be available)\n"); 
    }

    acpi_state.acpi_available = 1; 

    if (acpi_state.acpi_available) {
        acpi_state.madt = (struct acpi_madt*)acpi_find_table(ACPI_MADT_SIGNATURE, hhdm_response);
        if (acpi_state.madt) {
            serial_puts("[ACPI] MADT found\n");
        }

        acpi_state.fadt = (struct acpi_fadt*)acpi_find_table(ACPI_FADT_SIGNATURE, hhdm_response);
        if (acpi_state.fadt) {
            serial_puts("[ACPI] FADT found\n");
        } else {
            serial_puts("[ACPI] FADT NOT FOUND - ACPI shutdown/reboot unavailable\n");
        }
    } else {
        serial_puts("[ACPI] ACPI not available\n");
    }

    serial_puts("[ACPI] ACPI initialized successfully\n");
}

struct acpi_sdt_header *acpi_find_table(const char* signature, volatile struct limine_hhdm_response *hhdm_response_unused) {
    (void)hhdm_response_unused;
    if (!acpi_state.acpi_available) {
        return NULL;
    }

    struct acpi_sdt_header* table_header = NULL;

    if (acpi_state.use_xsdt && acpi_state.xsdt) {
        uint64_t entry_count = (acpi_state.xsdt->header.length - sizeof(struct acpi_sdt_header)) / 8;

        for (uint64_t i = 0; i < entry_count; i++) {
            table_header = (struct acpi_sdt_header*)acpi_map_physical_to_virtual(acpi_state.xsdt->tables[i]);
            if (!table_header) continue;

            if (strncmp(table_header->signature, signature, 4) == 0 && acpi_checksum_valid(table_header, table_header->length)) {
                serial_puts("[ACPI] Found table: ");
                serial_puts(table_header->signature);
                serial_putc('\n');
                return table_header;
            }
        }
    } else if (acpi_state.rsdt) {
        uint32_t entry_count = (acpi_state.rsdt->header.length - sizeof(struct acpi_sdt_header)) / 4;

        for (uint32_t i = 0; i < entry_count; i++) {
            table_header = (struct acpi_sdt_header*)acpi_map_physical_to_virtual(acpi_state.rsdt->tables[i]);
            if (!table_header) continue;

            if (strncmp(table_header->signature, signature, 4) == 0 && acpi_checksum_valid(table_header, table_header->length)) {
                serial_puts("[ACPI] Found table: ");
                serial_puts(table_header->signature);
                serial_putc('\n');
                return table_header;
            }
        }
    }

    return NULL;
}

bool acpi_checksum_valid(void *data, size_t length) {
    uint8_t sum = 0;
    uint8_t *bytes = (uint8_t*)data;

    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }

    return sum == 0;
}

void acpi_shutdown(void) {
    if (!acpi_state.fadt || !acpi_state.fadt->smi_cmd || !acpi_state.fadt->acpi_enable) {
        serial_puts("[ACPI] ERROR: Cannot shutdown - FADT not available\n");
        return;
    }
    
    outb(acpi_state.fadt->smi_cmd, acpi_state.fadt->acpi_enable);
    
    uint16_t pm1a_cnt = acpi_state.fadt->pm1a_cnt_blk;
    uint16_t slp_typa = (acpi_state.fadt->pm1a_cnt_blk >> 10) & 0x7; // SLP_TYPa
    uint16_t slp_en = 1 << 13; 
    
    outw(pm1a_cnt, slp_typa | slp_en);
    
    serial_puts("[ACPI] Shutdown command sent\n");
    
    serial_puts("[ACPI] ERROR: Shutdown failed!\n");
}

void acpi_reboot(void) {
    uint8_t temp;
    
    do {
        temp = inb(0x64);
        if ((temp & 0x02) == 0) break;
    } while (1);
    
    outb(0x64, 0xFE); 
    
    serial_puts("[ACPI] Reboot command sent\n");
    
    serial_puts("[ACPI] ERROR: Reboot failed!\n");
}