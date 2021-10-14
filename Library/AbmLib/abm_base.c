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

//TODO: fix this include
#include <../Include/lvgl.h>
#include <../Include/lv_misc/lv_log.h>
#include <string.h>
#include <Protocol/BlockIo.h>
EFI_SYSTEM_TABLE                    *gST1;
EFI_BOOT_SERVICES             *gBS1;
EFI_GRAPHICS_OUTPUT_PROTOCOL *mGop;
lv_disp_drv_t                 mDispDrv;
lv_indev_drv_t                mFakeInputDrv;
lv_disp_t * disp;
bool abm_running=true;
#include <ext4.h>
#include "abm_fs.h"
#include <ext4_mkfs.h>
#include "config.h"

void * buf_k1;
void * buf_rd;
int rd_size;
int k_size;
int num_of_boot_entries;
struct boot_entry *entry_list;

struct boot_entry_now {
    bool boot;
    int sleep_time;
    char *title;
    bool internal;
	char *linux_kernel;
    char *initrd;
    char *dtb;
    char *cmdline;
    char *logopath;
};
struct DualbootInfo {
  BOOLEAN CustomSlot;
  VOID *linux_kernel;
  UINT32 linux_size;
};
struct boot_entry_now boot_now;
struct DualbootInfo *db;
UINT64 BaseMemory;
static struct ext4_blockdev *bd;

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
	    if(((unsigned char*)data)[i] !=0)
		    DEBUG ((EFI_D_INFO,"%02X ", ((unsigned char*)data)[i]));
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			DEBUG ((EFI_D_INFO," "));
			if ((i+1) % 16 == 0) {
				DEBUG ((EFI_D_INFO,"|  %s \n", ascii));
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					DEBUG ((EFI_D_INFO," "));
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					DEBUG ((EFI_D_INFO,"   "));
				}
				DEBUG ((EFI_D_INFO,"|  %s \n", ascii));
			}
		}
	}
}

void *load_file_to_memory(const char *filename) 
{ 
    unsigned char *buf;
	int size = 0;
	size_t rb;
	ext4_file f;
	ext4_fopen(&f, filename, "r");
	ext4_fseek(&f, 0, SEEK_END);
	size = ext4_ftell(&f);
	buf = malloc(size + 1);
	ext4_fseek(&f, 0, SEEK_SET);
	ext4_fread(&f, buf, size, &rb);
	ext4_fclose(&f);
	buf[size] = 0;
	return buf;
}

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        int index = lv_list_get_btn_index(NULL, obj);     
        if(index==0){
            abm_running=false;
            boot_now.title=malloc(strlen(entry_list->title));
            strcpy(boot_now.title, entry_list->title);
            boot_now.title = entry_list->title;
            db->CustomSlot=false;
            boot_now.internal=true;
            //draw_booting();
        //boot_linux_from_storage();
        return;
        }
        if(index==num_of_boot_entries)
        {
            abm_running=false;
            //db->CustomSlot=false;
            buf_k1=NULL;
            k_size=0;
            buf_rd=NULL;
            rd_size=0;
        }
      
        else
        {
            char *linux_kernel = malloc(strlen("/meta/") + strlen((entry_list + index)->linux_kernel) + 1);
		    char *initrd = malloc(strlen("/meta/") + strlen((entry_list + index)->initrd) + 1);
            char *dtb = malloc(strlen("/meta/") + strlen((entry_list + index)->dtb) + 1);
            strcpy(linux_kernel, "/meta/");
		    strcat(linux_kernel, (entry_list + index)->linux_kernel);
		    strcpy(initrd, "/meta/");
		    strcat(initrd, (entry_list + index)->initrd);
            strcpy(dtb, "/meta/");
		    strcat(dtb, (entry_list + index)->dtb);
            boot_now.boot=true;
            boot_now.title=malloc(strlen((entry_list+index)->title));
            boot_now.title = (entry_list+index)->title;
            boot_now.linux_kernel=malloc(strlen(linux_kernel));
            boot_now.linux_kernel = linux_kernel;
            boot_now.initrd=malloc(strlen(initrd));
            boot_now.initrd = initrd;
            boot_now.dtb=malloc(strlen(dtb));
            boot_now.dtb = dtb;
            boot_now.cmdline=malloc(strlen((entry_list + index)->options));
            boot_now.cmdline = (entry_list + index)->options;
            boot_now.internal=false;
            DEBUG ((EFI_D_INFO,"Loading kernel: %a\n",	boot_now.linux_kernel));
            int size= 0;
	        size_t rb;
	        ext4_file f;
	        ext4_fopen(&f, boot_now.linux_kernel, "r");
	        ext4_fseek(&f, 0, SEEK_END);
	        size = ext4_ftell(&f);
	        ext4_fclose(&f);
            buf_k1 = malloc(size);
            buf_k1=load_file_to_memory(boot_now.linux_kernel);
            k_size=size;
            
	        ext4_fopen(&f, boot_now.initrd, "r");
	        ext4_fseek(&f, 0, SEEK_END);
	        size = ext4_ftell(&f);
	        ext4_fclose(&f);
            buf_rd = malloc(size);
            buf_rd=load_file_to_memory(boot_now.initrd);
            rd_size=size;      
           
            abm_running=false;
        }
    }
}
static void EfiGopBltFlush(
    lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
  mGop->Blt(
      mGop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)color_p, EfiBltBufferToVideo, 0, 0,
      area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, 0);

  lv_disp_flush_ready(disp_drv);
}


//Read keys state
bool key_read(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
  EFI_INPUT_KEY  Key;

  //Read keys
  gST1->ConIn->ReadKeyStroke (gST1->ConIn, &Key);

  //Vol up
  data->key = LV_KEY_UP;
  if (Key.ScanCode==SCAN_UP){
      data->state = LV_INDEV_STATE_PR;
      return false;
  } 

  //Vol down 
  data->key = LV_KEY_DOWN;
  if (Key.ScanCode==SCAN_DOWN){
      data->state = LV_INDEV_STATE_PR;
      return false;
  } 

  //Pwr key
  data->key = LV_KEY_ENTER;
  if (Key.ScanCode==SCAN_SUSPEND){
      data->state = LV_INDEV_STATE_PR;
      return false;
  } 
}

//
// main entry point
//
struct DualbootInfo
*test_lvgl (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, EFI_BLOCK_IO_PROTOCOL *BlockIo, UINT64 base_mem)
{
  gST1 = SystemTable; 
  gBS1 = gST1->BootServices; 
  gBS1->LocateProtocol(
      &gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);
  BaseMemory=base_mem;
  // Setup lwext4 lib, based on UEFI BlockIo protocol
  uefi_dev_set(BlockIo);
  bd = uefi_dev_get();
  if (!bd) {
	DebugAssert("abm_base.c", 92, "open_filedev: fail\n");
  }
  
  mount_meta(bd);
  entry_list = parse_boot_entries(&entry_list);

  // Prepare LittleVGL
  lv_init();   
  static lv_disp_buf_t disp_buf;
  static lv_color_t buf[LV_HOR_RES_MAX * 10]; /*Declare a buffer for 10 lines*/
  lv_disp_buf_init( & disp_buf, buf, NULL, LV_HOR_RES_MAX * 10); /*Initialize the display buffer*/
  lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
  lv_disp_drv_init( & disp_drv); /*Basic initialization*/
  disp_drv.flush_cb =  EfiGopBltFlush; /*Set your driver function*/  
  disp_drv.buffer = & disp_buf; /*Assign the buffer to the display*/
  lv_disp_drv_register( & disp_drv); /*Finally register the driver*/

  lv_obj_t * win = lv_win_create(lv_scr_act(), NULL);
  lv_win_set_title(win, "Boot Menu"); 
  lv_obj_t * list1 = lv_list_create(win, NULL);
  lv_obj_set_size(list1, 1000, 1050);

  lv_group_t * g1 = lv_group_create();
  lv_group_add_obj(g1, list1);
  lv_group_focus_obj(list1);


  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
  indev_drv.type = LV_INDEV_TYPE_KEYPAD;
  indev_drv.read_cb = key_read;
  /*Register the driver in LVGL and save the created input device object*/
  lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
  lv_indev_set_group(my_indev, g1);

  lv_obj_t * list_btn;
  lv_obj_set_state(list1, LV_STATE_DEFAULT);
  num_of_boot_entries=get_entry_count();
  for(int i=0; i<get_entry_count(); i++){
        list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, (entry_list+i)->title);
        lv_obj_set_event_cb(list_btn, event_handler);
  }
  list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Extras");
  lv_obj_set_event_cb(list_btn, event_handler);
  lv_list_set_anim_time(list1, 500);

  while (abm_running) {
    lv_tick_inc(1);
    lv_task_handler();
    gBS1->Stall(EFI_TIMER_PERIOD_MILLISECONDS(1));
  }
  return db;
} 

void
*get_dualboot_kernel(){
return buf_k1;
}

int get_dualboot_kernel_size(){
return k_size;
}

void
*get_dualboot_initrd(){
return buf_rd;
}

int get_dualboot_initrd_size(){
return rd_size;
}
