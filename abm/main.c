#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiLib.h>
//TODO: fix this include
#include <../Include/lvgl.h>
#include <../Include/lv_misc/lv_log.h>
#include <string.h>

EFI_SYSTEM_TABLE                    *gST1;
EFI_BOOT_SERVICES             *gBS1;
EFI_GRAPHICS_OUTPUT_PROTOCOL *mGop;
lv_disp_drv_t                 mDispDrv;
lv_indev_drv_t                mFakeInputDrv;
lv_disp_t * disp;

static void EfiGopBltFlush(
    lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
  mGop->Blt(
      mGop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)color_p, EfiBltBufferToVideo, 0, 0,
      area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, 0);

  lv_disp_flush_ready(disp_drv);
}


//
// main entry point
//
EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  gST1 = SystemTable; 
  gBS1 = gST1->BootServices; 
  gBS1->LocateProtocol(
      &gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);
Print(L"commom init\n");
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
lv_obj_set_size(list1, 1000, 1000);

   lv_obj_t * list_btn;
    lv_obj_set_state(list1, LV_STATE_DEFAULT);
 list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Example 1");
 list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Example 2");
 list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Extras");
Print(L"commom loop");

while (TRUE) {
    lv_tick_inc(1);
    lv_task_handler();
    gBS1->Stall(EFI_TIMER_PERIOD_MILLISECONDS(1));
}
} 
