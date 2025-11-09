#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

// ACPI Table Signatures
#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_XSDT_SIGNATURE "XSDT"
#define ACPI_MADT_SIGNATURE "APIC"
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_MCFG_SIGNATURE "MCFG"
#define ACPI_HPET_SIGNATURE "HPET"

// RSDP Structure
struct acpi_rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    
    // ACPI 2.0+ fields
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct acpi_rsdt {
    struct acpi_sdt_header header;
    uint32_t tables[];
} __attribute__((packed));

struct acpi_xsdt {
    struct acpi_sdt_header header;
    uint64_t tables[];
} __attribute__((packed));

struct acpi_madt {
    struct acpi_sdt_header header;
    uint32_t local_apic_addr;
    uint32_t flags;
} __attribute__((packed));

#define MADT_ENTRY_LAPIC 0
#define MADT_ENTRY_IOAPIC 1
#define MADT_ENTRY_ISO 2
#define MADT_ENTRY_NMI 4
#define MADT_ENTRY_LAPIC_ADDR_OVERRIDE 5

struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_lapic {
    struct madt_entry_header header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_ioapic {
    struct madt_entry_header header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_addr;
    uint32_t gsi_base;
} __attribute__((packed));

struct madt_iso {
    struct madt_entry_header header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed));

struct madt_nmi {
    struct madt_entry_header header;
    uint8_t processor_id;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed));

struct acpi_fadt {
    struct acpi_sdt_header header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    
    uint8_t reserved;
    
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_len;
    uint8_t gpe1_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    
    // ACPI 2.0+ 
    uint16_t boot_arch_flags;
    uint8_t reserved2;
    uint32_t flags;
    
    // ACPI 3.0+ 
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } reset_reg;
    
    uint8_t reset_value;
    uint8_t reserved3[3];
    
    // FADT 5.0
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_pm1a_evt_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_pm1b_evt_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_pm1a_cnt_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_pm1b_cnt_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_pm2_cnt_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_pm_tmr_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_gpe0_blk;
    
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } x_gpe1_blk;
} __attribute__((packed));

// MCFG (PCI Express)
struct acpi_mcfg {
    struct acpi_sdt_header header;
    uint64_t reserved;
} __attribute__((packed));

struct mcfg_entry {
    uint64_t base_address;
    uint16_t segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed));

// HPET 
struct acpi_hpet {
    struct acpi_sdt_header header;
    uint32_t block_id;
    struct {
        uint64_t address;
        uint8_t address_space_id;
        uint8_t register_bit_width;
        uint8_t register_bit_offset;
        uint8_t access_size;
        uint64_t address64;
    } base_address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

//ACPI 
struct acpi_state {
    struct acpi_rsdp *rsdp;
    struct acpi_rsdt *rsdt;
    struct acpi_xsdt *xsdt;
    struct acpi_madt *madt;
    struct acpi_fadt *fadt;
    struct acpi_mcfg *mcfg;
    struct acpi_hpet *hpet;
    bool use_xsdt;
    bool acpi_available;
};

void acpi_init(volatile struct limine_hhdm_response *hhdm_resp);
bool acpi_checksum_valid(void *data, size_t length);
struct acpi_sdt_header *acpi_find_table(const char *signature, volatile struct limine_hhdm_response *hhdm_response);
void acpi_shutdown(void);
void acpi_reboot(void);

extern struct acpi_state acpi_state;

#endif // ACPI_H