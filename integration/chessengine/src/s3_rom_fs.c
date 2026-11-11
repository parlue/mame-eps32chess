// Platform bootstrap for the private FAT/WL ROM partition. Never format on
// mount failure: a missing/corrupt image must not destroy installed ROMs.
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "wear_levelling.h"
#include <stdio.h>
int mame_s3_mount_roms(void)
{
    static wl_handle_t handle=WL_INVALID_HANDLE;
    if(handle!=WL_INVALID_HANDLE)return 1;
    const esp_vfs_fat_mount_config_t config={
        .format_if_mount_failed=false,
        .max_files=16,
        .allocation_unit_size=4096,
    };
    esp_err_t error=esp_vfs_fat_spiflash_mount_rw_wl("/roms","roms",&config,&handle);
    if(error!=ESP_OK){printf("[ROM FS] mount failed: %s; no formatting attempted\n",esp_err_to_name(error));return 0;}
    FILE *rom=fopen("/roms/mm4/710","rb");
    if(!rom){printf("[ROM FS] missing /roms/mm4/710\n");return 0;}
    fseek(rom,0,SEEK_END);long bytes=ftell(rom);fclose(rom);
    printf("[ROM FS] mounted; MM IV ROM: %ld bytes (MAME verifies hashes)\n",bytes);
    return bytes==32768;
}
