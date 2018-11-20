#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "ext2_fs.h"
#include <math.h>
#include <time.h>
#include <stdint.h>

static int fd = 0;
__u32	blocks_count;		/* Blocks count */
__u32	inodes_count;		/* Inodes count */
__u32	block_size;	/* log2(Block size) */
__u32	inode_size;	/* log2(Block size) */
__u32	blocks_per_group;	/* # Blocks per group */
__u32	inodes_per_group;	/* # Inodes per group */
__u32 first_data_block;

void analyze_superblock(struct ext2_super_block*);
void analyze_groupdesc(struct ext2_group_desc*);
void analyze_fbentries(__u32, __u32);
void analyze_fientries(__u32, __u32, __u32);
void analyze_inodesummary(__u32, __u32, __u32);
void analyze_dir(__u32, __u32);
void print_err(char* err){
  fprintf(stderr, "%s\n", err);
  exit(2);
}

int main(int argc, char** argv){
  if(argc != 2){
    fprintf(stderr, "Usage: lab3a [img file]\n");
    exit(EXIT_FAILURE);
  }
  if((fd = open(argv[1], O_RDONLY)) < 0)
    print_err(strerror(errno));

  struct ext2_super_block* sb = malloc(sizeof(struct ext2_super_block));
  struct ext2_group_desc* gd = malloc(sizeof(struct ext2_group_desc));

  if(pread(fd, sb, sizeof(struct ext2_super_block), EXT2_MIN_BLOCK_SIZE) == -1)
    print_err(strerror(errno));

  analyze_superblock(sb);
  analyze_groupdesc(gd);

  free(gd);
  free(sb);
  return 0;
}

void analyze_superblock(struct ext2_super_block* sb){
  blocks_count = sb->s_blocks_count;
  inodes_count = sb->s_inodes_count;
  block_size = EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size;
  inode_size = sb->s_inode_size;
  blocks_per_group = sb->s_blocks_per_group;
  inodes_per_group = sb->s_inodes_per_group;
  first_data_block = sb->s_first_data_block;
  printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
	  blocks_count,
	  inodes_count,
	  block_size,
	  inode_size,
	  blocks_per_group,
	  inodes_per_group,
	  sb->s_first_ino);
}

void analyze_groupdesc(struct ext2_group_desc* gd){
  __u32 nGroups = ceil(((double)blocks_count)/blocks_per_group);
  __u32 i;
  for(i = 0; i < nGroups; i++){
    if(pread(fd, gd, sizeof(struct ext2_group_desc), 1024 + EXT2_MIN_BLOCK_SIZE + i*sizeof(struct ext2_group_desc)) == -1)
      print_err(strerror(errno));

    __u32 totalBlocks = (blocks_count % blocks_per_group) ? blocks_count % blocks_per_group : blocks_per_group; // if expression > 0, then it is the last block (remainder)
    __u32 totalInodes = (inodes_count % inodes_per_group) ? inodes_count % inodes_per_group : inodes_per_group; // if expression > 0, then it is the last block (remainder)
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
	    i,
	    totalBlocks,
	    totalInodes,
	    gd->bg_free_blocks_count,
	    gd->bg_free_inodes_count,
	    gd->bg_block_bitmap,
	    gd->bg_inode_bitmap,
	    gd->bg_inode_table);

    analyze_fbentries(gd->bg_block_bitmap, i);
    analyze_fientries(gd->bg_inode_bitmap, gd->bg_inode_table, i);
  }

}

void analyze_fbentries(__u32 block_bitmap, __u32 group) {
  char buf[block_size];
  if(pread(fd, buf, block_size, block_bitmap * block_size) < 0)
    print_err(strerror(errno));
  __u32 index = first_data_block + (group * blocks_per_group);

  __u32 i, j;
  for (i = 0; i < block_size; i++) {
    char curr = buf[i];
    for (j = 0; j < 8; j++) {
      if (!(curr & 1)) {
        printf("BFREE,%d\n", index);
      }
      curr >>= 1;
      index++;
    }
  }
}

void analyze_fientries(__u32 inode_bitmap, __u32 inode_table, __u32 group){
  __u32 bitmap_size = inodes_per_group/8;
  char bitmap[bitmap_size];
  if(pread(fd, bitmap, bitmap_size, inode_bitmap * block_size) < 0)
    print_err(strerror(errno));

  __u32 inode_block = group * inodes_per_group + 1;
  __u32 i;
  for(i = 0; i < bitmap_size; i++){
    char byte = bitmap[i];
    __u32 j;
    for(j = 0; j < 8; j++){ // scan through entire byte for 0's = free/available blocks
      if(!(byte & 1))
	printf("IFREE,%d\n", inode_block);
      else
	analyze_inodesummary(inode_table, inode_block - (group * inodes_per_group + 1), inode_block);
      byte >>= 1;
      inode_block++;
    }
  }
}

void analyze_inodesummary(__u32 inode_table, __u32 inodeIndex, __u32 inode){
  struct ext2_inode* inodeStruct = malloc(sizeof(struct ext2_inode));
  pread(fd, inodeStruct, sizeof(struct ext2_inode), inode_table*block_size + inodeIndex*sizeof(struct ext2_inode));
  if(!inodeStruct->i_mode || !inodeStruct->i_links_count){
    free(inodeStruct);
    return;
  }
  
  char type;
  if((inodeStruct->i_mode & 0xA000) == 0xA000)
    type = 's';
  else if((inodeStruct->i_mode & 0x8000) == 0x8000)
    type = 'f';
  else if((inodeStruct->i_mode & 0x4000) == 0x4000)
    type = 'd';
  else
    type = '?';

  char inodeChange[25], inodeMod[25], inodeAccess[25];
  time_t ctime = inodeStruct->i_ctime;
  time_t mtime = inodeStruct->i_mtime;
  time_t atime = inodeStruct->i_atime;
  strftime(inodeChange, 25, "%m/%d/%y %H:%M:%S", gmtime(&ctime));
  strftime(inodeMod, 25, "%m/%d/%y %H:%M:%S", gmtime(&mtime));
  strftime(inodeAccess, 25, "%m/%d/%y %H:%M:%S", gmtime(&atime));
  
  printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
	 inode,
	 type,
	 (inodeStruct->i_mode & 0xFFF),
	 inodeStruct->i_uid,
	 inodeStruct->i_gid,
	 inodeStruct->i_links_count,
	 inodeChange,
	 inodeMod,
	 inodeAccess,
	 inodeStruct->i_size,
	 inodeStruct->i_blocks);

  // if file or directory or symbolic link with size > 60
  if(type == 'f' || type == 'd' || (type == 's' && inodeStruct->i_size > 60)){
    __u32 i;
    // scan all the direct blocks
    for(i = 0; i < EXT2_N_BLOCKS; i++)
      printf(",%d", inodeStruct->i_block[i]);
    printf("\n");
    
    for(i = 0; i < EXT2_NDIR_BLOCKS; i++)
      if(type == 'd' && inodeStruct->i_block[i])
	analyze_dir(inode, inodeStruct->i_block[i]);
  }
  free(inodeStruct);
}

void analyze_dir(__u32 inode, __u32 block){
  struct ext2_dir_entry* dir = malloc(sizeof(struct ext2_dir_entry));
  __u32 i;
  for(i = 0; i < block_size; i += dir->rec_len){
    pread(fd, dir, sizeof(struct ext2_dir_entry), block*block_size + i);
    if(dir->inode)
      printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
	     inode,
	     i,
	     dir->inode,
	     dir->rec_len,
	     dir->name_len,
	     dir->name);
  }

  free(dir);
}
