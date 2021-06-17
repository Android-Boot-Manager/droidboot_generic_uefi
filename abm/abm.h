#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiLib.h>
//TODO: fix this include
#include <../Include/lvgl.h>
#include <../Include/lv_misc/lv_log.h>
#include <string.h>

static void event_handler(lv_obj_t * obj, lv_event_t event);
static void EfiGopBltFlush(
    lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

bool key_read(lv_indev_drv_t * drv, lv_indev_data_t*data);

//
// main entry point
//
EFI_STATUS
EFIAPI
efi_main_init (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
