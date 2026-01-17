#ifndef _MULTIBOOT2_H_
#define _MULTIBOOT2_H_

#include <stdint.h>

#define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
#define MULTIBOOT2_ARCHITECTURE_I386 0
#define MULTIBOOT2_HEADER_TAG_END 0
#define MULTIBOOT2_HEADER_TAG_INFORMATION_REQUEST 1
#define MULTIBOOT2_HEADER_TAG_ADDRESS 2
#define MULTIBOOT2_HEADER_TAG_ENTRY_ADDRESS 3
#define MULTIBOOT2_HEADER_TAG_CONSOLE_FLAGS 4
#define MULTIBOOT2_HEADER_TAG_FRAMEBUFFER 5
#define MULTIBOOT2_HEADER_TAG_MODULE_ALIGN 6
#define MULTIBOOT2_HEADER_TAG_EFI_BS 7
#define MULTIBOOT2_HEADER_TAG_EFI32 8
#define MULTIBOOT2_HEADER_TAG_EFI64 9
#define MULTIBOOT2_HEADER_TAG_RELOCATABLE 10

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BS 18
#define MULTIBOOT_TAG_TYPE_EFI32_IH 19
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

struct multiboot_header_tag {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
};

struct multiboot_header_tag_information_request {
    struct multiboot_header_tag header;
    uint32_t requests[0];
};

struct multiboot_header_tag_address {
    struct multiboot_header_tag header;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
};

struct multiboot_header_tag_entry_address {
    struct multiboot_header_tag header;
    uint32_t entry_addr;
};

struct multiboot_header_tag_console_flags {
    struct multiboot_header_tag header;
    uint32_t console_flags;
};

struct multiboot_header_tag_framebuffer {
    struct multiboot_header_tag header;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct multiboot_header_tag_module_align {
    struct multiboot_header_tag header;
};

struct multiboot_header_tag_relocatable {
    struct multiboot_header_tag header;
    uint32_t min_addr;
    uint32_t max_addr;
    uint32_t align;
    uint32_t preference;
};

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char string[0];
};

struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[0];
};

struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;
    uint32_t mem_upper;
};

struct multiboot_tag_bootdev {
    uint32_t type;
    uint32_t size;
    uint32_t biosdev;
    uint32_t slice;
    uint32_t part;
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct multiboot_mmap_entry {
        uint64_t addr;
        uint64_t len;
        uint32_t type;
        uint32_t zero;
    } entries[0];
};

struct multiboot_tag_vbe {
    uint32_t type;
    uint32_t size;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
};

struct multiboot_tag_framebuffer_common {
    uint32_t type;
    uint32_t size;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[0];
};

struct multiboot_tag_framebuffer_tag_rgb {
    uint8_t framebuffer_red_field_position;
    uint8_t framebuffer_red_mask_size;
    uint8_t framebuffer_green_field_position;
    uint8_t framebuffer_green_mask_size;
    uint8_t framebuffer_blue_field_position;
    uint8_t framebuffer_blue_mask_size;
};

struct multiboot_tag_elf_sections {
    uint32_t type;
    uint32_t size;
    uint32_t num;
    uint32_t entsize;
    uint32_t shndx;
    char sections[0];
};

struct multiboot_tag_apm {
    uint32_t type;
    uint32_t size;
    uint16_t version;
    uint16_t cseg;
    uint32_t offset;
    uint16_t cseg_16;
    uint16_t dseg;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg_16_len;
    uint16_t dseg_len;
};

struct multiboot_tag_efi32 {
    uint32_t type;
    uint32_t size;
    uint32_t pointer;
};

struct multiboot_tag_efi64 {
    uint32_t type;
    uint32_t size;
    uint64_t pointer;
};

struct multiboot_tag_smbios {
    uint32_t type;
    uint32_t size;
    uint8_t major;
    uint8_t minor;
    uint8_t reserved[6];
    uint8_t tables[0];
};

struct multiboot_tag_old_acpi {
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[0];
};

struct multiboot_tag_new_acpi {
    uint32_t type;
    uint32_t size;
    uint8_t rsdp[0];
};

struct multiboot_tag_network {
    uint32_t type;
    uint32_t size;
    uint8_t dhcpack[0];
};

struct multiboot_tag_efi_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t descr_size;
    uint32_t descr_vers;
    uint8_t efi_mmap[0];
};

struct multiboot_tag_efi_bs {
    uint32_t type;
    uint32_t size;
    uint8_t efi_bs[0];
};

struct multiboot_tag_efi32_ih {
    uint32_t type;
    uint32_t size;
    uint32_t pointer;
};

struct multiboot_tag_efi64_ih {
    uint32_t type;
    uint32_t size;
    uint64_t pointer;
};

struct multiboot_tag_load_base_addr {
    uint32_t type;
    uint32_t size;
    uint32_t load_base_addr;
};

#endif
