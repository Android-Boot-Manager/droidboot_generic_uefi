#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <stdbool.h>

#ifndef CONFIG_H
#define CONFIG_H
struct global_config {
	char *default_entry_title;
	struct boot_entry *default_entry;
	int timeout;
};


struct boot_entry {
	char *title;
	char *linux_kernel;
	char *initrd;
    char *dtb;
	char *options;
    bool is_android;
	bool error;
};

int get_entry_count(void);

struct boot_entry *parse_boot_entries(struct boot_entry **_entry_list);

int parse_global_config(struct global_config *global_config);

#endif 
