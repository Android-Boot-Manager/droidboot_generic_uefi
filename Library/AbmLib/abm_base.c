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
bool abm_running=true;
static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        abm_running=false;
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
EFI_STATUS
EFIAPI
test_lvgl (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  gST1 = SystemTable; 
  gBS1 = gST1->BootServices; 
  gBS1->LocateProtocol(
      &gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);


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
  list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Example 1");
  lv_obj_set_event_cb(list_btn, event_handler);
  list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Example 2");
  lv_obj_set_event_cb(list_btn, event_handler);
  list_btn = lv_list_add_btn(list1,  LV_SYMBOL_FILE, "Extras");
  lv_list_set_anim_time(list1, 500);

  while (abm_running) {
    lv_tick_inc(1);
    lv_task_handler();
    gBS1->Stall(EFI_TIMER_PERIOD_MILLISECONDS(1));
  }
} 
