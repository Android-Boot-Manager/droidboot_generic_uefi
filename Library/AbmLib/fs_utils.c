#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <stdio.h>
#include <dirent.h>

#include "fs_utils.h"
#include <ext4.h>

int dir_count_entries(const char *path) {
    const ext4_direntry *de;
    ext4_dir d;

    // Ensure we can open directory.

    ext4_dir_open (&d, path);

    // Process each entry.
    int res=0;
    de = ext4_dir_entry_next(&d);
    DEBUG ((EFI_D_INFO, "dir_count_entries, path: %a\n", path));
	while (de) {
	DEBUG ((EFI_D_INFO, "Looping in dir_count_entries\n"));
		if(de->inode_type==EXT4_DE_REG_FILE){
		  res+=1;
		}
		de = ext4_dir_entry_next(&d);
	}
	ext4_dir_close(&d);
    return res;
}
