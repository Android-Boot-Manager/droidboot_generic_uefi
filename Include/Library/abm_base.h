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

typedef VOID (*LINUX_KERNEL) (UINT64 ParametersBase,
                              UINT64 Reserved0,
                              UINT64 Reserved1,
                              UINT64 Reserved2);
typedef VOID (*LINUX_KERNEL32) (UINT32 Zero, UINT32 Arch, UINTN ParametersBase);
typedef struct DualbootInfo {
  BOOLEAN CustomSlot;
  VOID *linux_kernel;
  UINT32 linux_size;
};
struct DualbootInfo
*test_lvgl (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable, EFI_BLOCK_IO_PROTOCOL *BlockIo, UINT64 base_mem);

void
*get_dualboot_kernel();

int get_dualboot_kernel_size();

void
*get_dualboot_initrd();

int get_dualboot_initrd_size();

void
*get_dualboot_cmdline();

int get_dualboot_cmdline_size();
