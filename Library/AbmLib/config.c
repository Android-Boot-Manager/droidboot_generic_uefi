// SPDX-License-Identifier: GPL-2.0+
// © 2019 Mis012
// © 2020-2021 luka177

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
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "config.h"
#include "fs_utils.h"
#include <dirent.h>
#include <ext4.h>

#define ENTRIES_DIR "/meta/db/entries"
#define GLOBAL_CONFIG_FILE "/meta/db/db.conf"

static int config_parse_option(char **_dest, const char *option, const char *buffer) {
	char *temp = strstr(buffer, option);
	if(!temp)
		return -1;
	
	temp += strlen(option);
	while (*temp == ' ')
		temp++;
	char *newline = strchr(temp, '\n');
	if(newline)
		*newline = '\0';
	char *dest = malloc(strlen(temp) + 1);
	if(!dest)
		return 1;
	strcpy(dest, temp);
	*_dest = dest;

	//restore the buffer
	*newline = '\n';

	return 0;
}

static int parse_boot_entry_file(struct boot_entry *entry, char *file) {
	int ret;
    ext4_file fp;
	unsigned char *buf;
	size_t rb;
	char *path = malloc(strlen(file) + strlen(ENTRIES_DIR) + strlen("/") + 1);
	strcpy(path, ENTRIES_DIR);
	strcat(path, "/");
	strcat(path, file);
	
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: open file: %a\n", path));
	ret=ext4_fopen (&fp, path, "r");
	if(ret!=EOK){
	DEBUG ((EFI_D_INFO, "parse_boot_entry_file: open file: %a failed\n", path));
	}
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: open file: %a OK\n", path));
    
    ext4_fseek(&fp, 0, SEEK_END);
     DEBUG ((EFI_D_INFO, "parse_boot_entry_file: fseek end OK\n"));
    long fsize = ext4_ftell(&fp);
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: ftell OK\n"));
    ext4_fseek(&fp, 0, SEEK_SET);  /* same as rewind(f); */
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: fseek set OK\n"));
    
    buf = malloc(fsize + 1);
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: malloc OK\n"));
    
    ext4_fread(&fp, buf, fsize, &rb);
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: fread OK\n"));
	ext4_fclose(&fp);
    DEBUG ((EFI_D_INFO, "parse_boot_entry_file: file operations done\n"));
	buf[fsize] = '\0';
	
	ret = config_parse_option(&entry->title, "title", (const char *)buf);
	if(ret < 0) {
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->linux_kernel, "linux", (const char *)buf);
	if(ret < 0) {
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->initrd, "initrd", (const char *)buf);
	if(ret < 0) {
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->dtb, "dtb", (const char *)buf);
	if(ret < 0) {
		free(buf);
		return ret;
	}

	ret = config_parse_option(&entry->options, "options", (const char *)buf);
	if(ret < 0) {
		free(buf);
		return ret;
	}

	free(buf);

	entry->error = false;

	return 0;
}

int entry_count;

int get_entry_count(void) {
	return entry_count;
}

struct boot_entry *parse_boot_entries(struct boot_entry **_entry_list) {
	int ret;

    struct boot_entry *entry_list=malloc(sizeof(struct boot_entry)*dir_count_entries(ENTRIES_DIR));    

	ret = entry_count = dir_count_entries(ENTRIES_DIR);
	if (ret < 0) {
		entry_count = 0;
	}

	const ext4_direntry *de;
    ext4_dir d;

    ret=ext4_dir_open (&d, ENTRIES_DIR);
  
    de = ext4_dir_entry_next(&d);
	int i = 0;
	while(de) {
	DEBUG ((EFI_D_INFO, "Looping in parse_boot_entries, iteration: %d, inode_type: %d, name of file: %a", i, de->inode_type, de->name));
         if(de->inode_type==1){
            struct boot_entry *entry = (entry_list+i);
            DEBUG ((EFI_D_INFO, "Call parse_boot_entry_file\n"));
            ret = parse_boot_entry_file(entry, de->name);
            DEBUG ((EFI_D_INFO, "Call parse_boot_entry_file done\n"));
            if(ret < 0) {
                entry->error = true;
                entry->title = "SYNTAX ERROR";
            }
            DEBUG ((EFI_D_INFO, "New entry found, id %d, title: %a\n", i, entry->title));
            i++;
        }
        de = ext4_dir_entry_next(&d);
	}
	
	ext4_dir_close(&d);
    
   //*_entry_list = entry_list;
    //printf("First entry is: %s\n", entry_list->title);
	return entry_list;
}

int parse_global_config(struct global_config *global_config) {
	int ret;
	ext4_file fp;
	unsigned char *buf;

    ext4_fopen (&fp, GLOBAL_CONFIG_FILE, "r");

    ext4_fseek(&fp, 0, SEEK_END);
    long fsize = ext4_ftell(&fp);
    ext4_fseek(&fp, 0, SEEK_SET);  /* same as rewind(f); */
    
    buf = malloc(fsize + 1);
    
	ext4_fread(&fp, buf, 1, fsize);

	ext4_fclose(&fp);

	ret = config_parse_option(&global_config->default_entry_title, "default", (const char *)buf);
	if(ret < 0) {


		global_config->default_entry_title = NULL;
		global_config->timeout = 0;

		return 0;
	}

	char *timeout = NULL;
	ret = config_parse_option(&timeout, "timeout", (const char *)buf);

	global_config->timeout = atoi(timeout);

	return 0;
} 
