#include "pebble_process_info.h"
#include "src/resource_ids.auto.h"

const PebbleProcessInfo __pbl_app_info __attribute__ ((section (".pbl_header"))) = {
  .header = "PBLAPP",
  .struct_version = { PROCESS_INFO_CURRENT_STRUCT_VERSION_MAJOR, PROCESS_INFO_CURRENT_STRUCT_VERSION_MINOR },
  .sdk_version = { PROCESS_INFO_CURRENT_SDK_VERSION_MAJOR, PROCESS_INFO_CURRENT_SDK_VERSION_MINOR },
  .process_version = { 1, 3 },
  .load_size = 0xb6b6,
  .offset = 0xb6b6b6b6,
  .crc = 0xb6b6b6b6,
  .name = "Conway",
  .company = "Flynn Duniho",
  .icon_resource_id = RESOURCE_ID_MENU_ICON,
  .sym_table_addr = 0xA7A7A7A7,
  .flags = PROCESS_INFO_PLATFORM_EMERY,
  .num_reloc_entries = 0xdeadcafe,
  .uuid = { 0x26, 0xD3, 0x96, 0xDE, 0x37, 0xEE, 0x42, 0x1C, 0xA4, 0x83, 0x59, 0x66, 0xC5, 0xF1, 0x24, 0x7D },
  .virtual_size = 0xb6b6
};
