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

EXT4_BLOCKDEV_STATIC_INSTANCE(ufs_dev, 4096, 268435456, uefi_dev_open,
		uefi_dev_bread, uefi_dev_bwrite, uefi_dev_close, 0, 0);
int uefi_dev_open (struct ext4_blockdev *bdev){
    ufs_dev.part_offset = 0;
	ufs_dev.part_size = 268435456;
	ufs_dev.bdif->ph_bcnt = ufs_dev.part_size / 4096;
    return EOK;
}

		
struct ext4_blockdev *uefi_dev_get(void)
{
	return &ufs_dev;
}

void mount_meta(struct ext4_blockdev *bd){
  DEBUG ((EFI_D_INFO, "mount meta: start\n"));

  int r, s;

  r=ext4_device_register(bd, "ext4_fs");
  if (!bd){
    DEBUG ((EFI_D_INFO, "BD is broken :(\n"));
  }
  if (r != EOK) {
    DebugAssert("abm_fs.c", 96, "ext4_device_register: fail\n");
  }
	
  DEBUG ((EFI_D_INFO, "parse boot etry: mount begin\n"));	
  r=ext4_mount("ext4_fs", "/meta/", false);
  DEBUG ((EFI_D_INFO, "parse boot etry: mount done\n"));
  if (r != EOK) {
    DEBUG ((EFI_D_INFO, "ext4_mount: %d, status: %d, formatting partition\n", r,s));
    r = ext4_mkfs(&fs, bd, &info, fs_type);
	if (r != EOK) {
		DEBUG ((EFI_D_INFO,"ext4_mkfs error: %d\n", r));
		return;
	}

	memset(&info, 0, sizeof(struct ext4_mkfs_info));
	r = ext4_mkfs_read_info(bd, &info);
	if (r != EOK) {
		DEBUG ((EFI_D_INFO,"ext4_mkfs_read_info error: %d\n", r));
		return;
	}

	DEBUG ((EFI_D_INFO,"Created filesystem with parameters:\n"));
	DEBUG ((EFI_D_INFO,"Size: %"PRIu64"\n", info.len));
	DEBUG ((EFI_D_INFO,"Block size: %"PRIu32"\n", info.block_size));
	DEBUG ((EFI_D_INFO,"Blocks per group: %"PRIu32"\n", info.blocks_per_group));
	DEBUG ((EFI_D_INFO,"Inodes per group: %"PRIu32"\n",	info.inodes_per_group));
	DEBUG ((EFI_D_INFO,"Inode size: %"PRIu32"\n", info.inode_size));
	DEBUG ((EFI_D_INFO,"Inodes: %"PRIu32"\n", info.inodes));
	DEBUG ((EFI_D_INFO,"Journal blocks: %"PRIu32"\n", info.journal_blocks));
	DEBUG ((EFI_D_INFO,"Features ro_compat: 0x%x\n", info.feat_ro_compat));
	DEBUG ((EFI_D_INFO,"Features compat: 0x%x\n", info.feat_compat));
	DEBUG ((EFI_D_INFO,"Features incompat: 0x%x\n", info.feat_incompat));
	DEBUG ((EFI_D_INFO,"BG desc reserve: %"PRIu32"\n", info.bg_desc_reserve_blocks));
	DEBUG ((EFI_D_INFO,"Descriptor size: %"PRIu32"\n",info.dsc_size));
	DEBUG ((EFI_D_INFO,"Label: %s\n", info.label));
	r=ext4_mount("ext4_fs", "/meta/", false);
    DEBUG ((EFI_D_INFO, "parse boot etry: mount done\n"));
	if(r != EOK){
	  DebugAssert("abm_fs.c", 179, "ext4_mount: fail\n");
	}
  }
	
  r=ext4_cache_write_back("/meta/", 1);
  if (r != EOK) {
  DebugAssert("abm_fs.c", 115, "ext4_cache_write_back: fail\n");
	}
	
    char sss[255];
	ext4_dir d;
	const ext4_direntry *de;

	DEBUG ((EFI_D_INFO, "ls %a\n", "/meta/db/entries"));
	ext4_dir_open(&d, "/meta/db/entries");
	DEBUG ((EFI_D_INFO, "dir open: done\n"));
	de = ext4_dir_entry_next(&d);
    DEBUG ((EFI_D_INFO, "enter loop"));
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


