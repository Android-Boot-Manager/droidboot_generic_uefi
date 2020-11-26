#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiLib.h>
//TODO: fix this include
#include <../Include/lvgl.h>
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
  Print(L"efi_main: hi this is abm uefi app\n");
    // Prepare LittleVGL
  lv_init();

  lv_disp_drv_init(&mDispDrv);
  mDispDrv.flush_cb = EfiGopBltFlush;
  lv_disp_drv_register(&mDispDrv);
while (TRUE) {
    //lv_tick_inc(1);
    //lv_task_handler();
    gBS1->Stall(EFI_TIMER_PERIOD_MILLISECONDS(1000));
    Print(L"hi");  
}
} 
