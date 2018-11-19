#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "ext2_fs.h"

static int fd = 0;

void analyze_superblock(struct ext2_super_block*);
void analyze_groupdesc(struct ext2_group_desc*);
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

  if(pread(fd, gd, sizeof(struct ext2_group_desc), EXT2_MIN_BLOCK_SIZE*2) == -1)
    print_err(strerror(errno));

  analyze_superblock(sb);
  analyze_groupdesc(gd);
  
  return 0;
}

void analyze_superblock(struct ext2_super_block* sb){
  fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
	  sb->s_blocks_count,
	  sb->s_inodes_count,
	  EXT2_MIN_BLOCK_SIZE << sb->s_log_block_size,
	  sb->s_inode_size,
	  sb->s_blocks_per_group,
	  sb->s_inodes_per_group,
	  sb->s_first_ino);
}

void analyze_groupdesc(struct ext2_group_desc* gd){
  fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
	  0, 0, 0, 0, 0, 0, 0, 0);
}
