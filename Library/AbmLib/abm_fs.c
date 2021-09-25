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
#include <Guid/EventGroup.h>
#include <ext4.h>
#include <ext4_config.h>
#include <ext4_blockdev.h>
#include <ext4_errno.h>
#include <Protocol/DevicePath.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <ext4_mkfs.h>

static struct ext4_mkfs_info info = {
	.block_size = 4096,
	.journal = false,
};
static int fs_type = F_SET_EXT2;
static struct ext4_fs fs;


int uefi_dev_open (struct ext4_blockdev *bdev);
EFI_BLOCK_IO_PROTOCOL *BlockIo = NULL;

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
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
static char *entry_to_str(uint8_t type)
{
	switch (type) {
	case EXT4_DE_UNKNOWN:
		return "[unk] ";
	case EXT4_DE_REG_FILE:
		return "[fil] ";
	case EXT4_DE_DIR:
		return "[dir] ";
	case EXT4_DE_CHRDEV:
		return "[cha] ";
	case EXT4_DE_BLKDEV:
		return "[blk] ";
	case EXT4_DE_FIFO:
		return "[fif] ";
	case EXT4_DE_SOCK:
		return "[soc] ";
	case EXT4_DE_SYMLINK:
		return "[sym] ";
	default:
		break;
	}
	return "[???]";
}

void uefi_dev_set(EFI_BLOCK_IO_PROTOCOL *BlockIo1)
{
    BlockIo=BlockIo1;
}

int uefi_dev_close (struct ext4_blockdev *bdev){
    return EOK;
}

static int uefi_dev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
			 uint32_t blk_cnt){
  DEBUG ((EFI_D_INFO, "Reading %d blocks from UFS at block: %d \n",blk_cnt, blk_id));
  BlockIo->ReadBlocks(BlockIo, BlockIo->Media->MediaId,blk_id, blk_cnt*4096, buf);
  //DumpHex(buf, 4096);
  return EOK;
}
// Debugging is here too
int uefi_dev_bwrite(struct ext4_blockdev *bdev, const void *buf,
			  uint64_t blk_id, uint32_t blk_cnt){		  
  EFI_STATUS Status;
  DEBUG ((EFI_D_INFO, "Writing %d blocks to UFS at block: %d \n",blk_cnt, blk_id));
  Status=BlockIo->WriteBlocks(BlockIo, BlockIo->Media->MediaId,blk_id, blk_cnt*4096, buf);
  BlockIo->FlushBlocks(BlockIo);
  if(Status!=EFI_SUCCESS){
    DEBUG ((EFI_D_INFO, "Writing %d blocks to UFS at block: %d failed, error: %d, block size: %d\n",blk_cnt, blk_id, Status, BlockIo->Media->BlockSize));
  }
  return EOK;
}

EXT4_BLOCKDEV_STATIC_INSTANCE(ufs_dev, 4096, 65536, uefi_dev_open,
		uefi_dev_bread, uefi_dev_bwrite, uefi_dev_close, 0, 0);
int uefi_dev_open (struct ext4_blockdev *bdev){
    ufs_dev.part_offset = 0;
	ufs_dev.part_size = 65536;
	ufs_dev.bdif->ph_bcnt = ufs_dev.part_size / 4096;
    return EOK;
}

		
struct ext4_blockdev *uefi_dev_get(void)
{
	return &ufs_dev;
}

void mount_meta(struct ext4_blockdev *bd){

  int r, s;
  r=ext4_device_register(bd, "ext4_fs");
  if (!bd){
  DEBUG ((EFI_D_INFO, "BD is broken :(\n"));
  }
  if (r != EOK) {
  DebugAssert("abm_fs.c", 96, "ext4_device_register: fail\n");
	}
	
	
  r=ext4_mount("ext4_fs", "/meta/", false);
  if (r != EOK) {
  DEBUG ((EFI_D_INFO, "ext4_mount: %d, status: %d\n", r,s));

	}
	
  r=ext4_cache_write_back("/meta/", 1);
  if (r != EOK) {
  DebugAssert("abm_fs.c", 115, "ext4_cache_write_back: fail\n");
	}
	
    char sss[255];
	ext4_dir d;
	const ext4_direntry *de;

	DEBUG ((EFI_D_INFO, "ls %s", "/meta/"));

	ext4_dir_open(&d, "/meta/db/entries");
	de = ext4_dir_entry_next(&d);

	while (de) {
		memcpy(sss, de->name, de->name_length);
		sss[de->name_length] = 0;
		if(de->inode_type==EXT4_DE_REG_FILE){
		DEBUG ((EFI_D_INFO, "File file file\n"));
		}
		DEBUG ((EFI_D_INFO, "Found file or dir:  %a\n", sss));
		de = ext4_dir_entry_next(&d);
	}
	ext4_dir_close(&d);
}


