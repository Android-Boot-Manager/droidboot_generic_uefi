#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/BlockIo.h>

#include <ext4_config.h>
#include <ext4_blockdev.h>

#include <stdint.h>
#include <stdbool.h>

struct ext4_blockdev *uefi_dev_get(void);
void uefi_dev_set(EFI_BLOCK_IO_PROTOCOL *BlockIo1);
int uefi_dev_bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt);
