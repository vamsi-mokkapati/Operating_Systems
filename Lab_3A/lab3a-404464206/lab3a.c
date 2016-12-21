#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

struct super_block *sup;
struct group_block *grp;
struct inode_block *inodes;
struct dir_block *dirs;
char *disk_img_file;
int* inodes_array;
int fd_disk_image, fd_super, fd_group, fd_bitmap, fd_inode, fd_directory, fd_indirect, inode_cnt = 0, dir_cnt = 0;
uint8_t buf_1byte;
uint16_t buf_2bytes;
uint32_t buf_4bytes;
uint64_t buf_8bytes;

#define GROUP_DES_SIZE 32 // group descriptor table size
#define MAX_VALUE 2000
#define REGULAR 0x8000
#define DIRECTORY 0x4000
#define SYMBOLIC 0xA000

struct super_block {
  uint16_t s_magic;
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_block_size;
  uint32_t s_frag_size;
  uint32_t s_blocks_per_group;
  uint32_t s_inodes_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_first_data_block;
  uint32_t s_groups_count;
};

struct group_block {
  uint16_t bg_contained_blocks_count;
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;
  uint32_t bg_inode_bitmap;
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_table;
};

struct inode_block {
  uint32_t inode_number;
  char file_type;
  uint16_t mode;
  uint16_t owner;
  uint16_t group;
  uint16_t link_count;
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t access_time;
  uint64_t file_size;
  uint32_t number_of_blocks;
  uint32_t block_pointers[15];
};

struct dir_block {
  uint32_t i_offset;
  uint32_t i_number;
};

int isPowerOfTwo (int x) {
  while (((x % 2) == 0) && x > 1)
    x /= 2;
  return (x == 1);
}

void super_block() {
  // Magic number
  pread(fd_disk_image, &buf_2bytes, 2, 1024 + 56);
  sup->s_magic = buf_2bytes;
  
  // Total number of Inodes
  pread(fd_disk_image, &buf_4bytes, 4, 1024);
  sup->s_inodes_count = buf_4bytes;
  
  // Total number of Blocks
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 4);
  sup->s_blocks_count = buf_4bytes;
  
  // Block Size
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 24);
  int s_log_block_size = buf_4bytes;
  sup->s_block_size = 1024 << s_log_block_size;
  
  // Fragment Size
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 28);
  int s_log_frag_size = buf_4bytes;
  if (s_log_frag_size > 0)
    sup->s_frag_size = 1024 << s_log_frag_size;
  else
    sup->s_frag_size = 1024 >> -s_log_frag_size;
  
  // Blocks per Group
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 32);
  sup->s_blocks_per_group = buf_4bytes;
  
  // Inodes per Group
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 40);
  sup->s_inodes_per_group = buf_4bytes;
  
  // Fragments per Group
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 36);
  sup->s_frags_per_group = buf_4bytes;
  
  // First Data Block
  pread(fd_disk_image, &buf_4bytes, 4, 1024 + 20);
  sup->s_first_data_block = buf_4bytes;

  // Sanity Checking
  if (sup->s_magic != 0xef53) {
    fprintf(stderr, "Superblock - invalid magic: 0xdead\n");
    exit(EXIT_FAILURE);
  }
  if (sup->s_block_size < 512 || sup->s_block_size > 65536 ||
      isPowerOfTwo(sup->s_block_size) == 0) {
    fprintf(stderr, "Superblock - invalid block size: %d\n", sup->s_block_size);
    exit(EXIT_FAILURE);
  }
  if (sup->s_blocks_count % sup->s_blocks_per_group != 0) {
    fprintf(stderr, "Superblock - %d blocks, %d blocks/group\n", sup->s_blocks_count, sup->s_blocks_per_group);
    exit(EXIT_FAILURE);
  }
  if (sup->s_inodes_count % sup->s_inodes_per_group != 0) {
    fprintf(stderr, "Superblock - %d Inodes, %d Inodes/group\n", sup->s_inodes_count, sup->s_inodes_per_group);
    exit(EXIT_FAILURE);
  }
  if (sup->s_blocks_count > 204800) {
    fprintf(stderr, "Superblock - invalid block count %d > image size 204800\n", sup->s_blocks_count);
    exit(EXIT_FAILURE);
  }
  if (sup->s_first_data_block > 204800) {
    fprintf(stderr, "Superblock - invalid first block %d > image size 204800\n", sup->s_first_data_block);
    exit(EXIT_FAILURE);
  }

  // Number of Groups
  sup->s_groups_count = sup->s_blocks_count / sup->s_blocks_per_group;
  grp = malloc(sizeof(struct group_block) * sup->s_groups_count);

  // Print to super.csv
  dprintf(fd_super, "%x,%d,%d,%d,%d,%d,%d,%d,%d\n",
	  sup->s_magic,
	  sup->s_inodes_count,
	  sup->s_blocks_count,
	  sup->s_block_size,
	  sup->s_frag_size,
	  sup->s_blocks_per_group,
	  sup->s_inodes_per_group,
	  sup->s_frags_per_group,
	  sup->s_first_data_block);
  
  close(fd_super);
}

void group_descr() {
  int k = 0;
  while (k < sup->s_groups_count) {
    int group_offset = 2048 + (GROUP_DES_SIZE * k);
    
    // Number of Contained Blocks (should be 8192)
    grp[k].bg_contained_blocks_count = sup->s_blocks_per_group;
    
    // Number of Free Blocks
    pread(fd_disk_image, &buf_2bytes, 2, group_offset + 12);
    grp[k].bg_free_blocks_count = buf_2bytes;
    
    // Number of Free Inodes
    pread(fd_disk_image, &buf_2bytes, 2, group_offset + 14);
    grp[k].bg_free_inodes_count = buf_2bytes;
    
    // Number of Directories
    pread(fd_disk_image, &buf_2bytes, 2, group_offset + 16);
    grp[k].bg_used_dirs_count = buf_2bytes;
    
    // (Free) Inode Bitmap Block
    pread(fd_disk_image, &buf_4bytes, 4, group_offset + 4);
    grp[k].bg_inode_bitmap = buf_4bytes;
    
    // (Free) Block Bitmap Block
    pread(fd_disk_image, &buf_4bytes, 4, group_offset + 0);
    grp[k].bg_block_bitmap = buf_4bytes;
    
    // Inode Table (Start) Block
    pread(fd_disk_image, &buf_4bytes, 4, group_offset + 8);
    grp[k].bg_inode_table = buf_4bytes;

    k++;
  }

  // Sanity Checking
  int l = 0;
  while (l < sup->s_groups_count) {
    if (grp[l].bg_contained_blocks_count != sup->s_blocks_per_group) {
      fprintf(stderr, "Group %d: %d blocks, superblock says %d\n", l, grp[l].bg_contained_blocks_count, sup->s_blocks_per_group);
      exit(EXIT_FAILURE);
    }
    // Check bitmap and inode starting blocks to see if within group
    if (grp[l].bg_inode_bitmap < grp[l].bg_contained_blocks_count * l) {
      fprintf(stderr, "Group %d: blocks %d-%d, free Inode map starts at %d\n", l, grp[l].bg_contained_blocks_count * l,
	      grp[l].bg_contained_blocks_count * (l+1), grp[l].bg_inode_bitmap);
      exit(EXIT_FAILURE);
    }
    if (grp[l].bg_block_bitmap < grp[l].bg_contained_blocks_count * l) {
      fprintf(stderr, "Group %d: blocks %d-%d, free block map starts at %d\n", l, grp[l].bg_contained_blocks_count * l,
              grp[l].bg_contained_blocks_count * (l+1),	grp[l].bg_block_bitmap);
      exit(EXIT_FAILURE);
    }
    if (grp[l].bg_inode_table <	grp[l].bg_contained_blocks_count * l) {
      fprintf(stderr, "Group %d: blocks %d-%d, Inode table starts at %d\n", l, grp[l].bg_contained_blocks_count * l,
              grp[l].bg_contained_blocks_count * (l+1), grp[l].bg_inode_table);
      exit(EXIT_FAILURE);
    }
    l++;
  }

  // Print to group.csv
  int m = 0;
  while (m < sup->s_groups_count) {
    dprintf(fd_group, "%d,%d,%d,%d,%x,%x,%x\n",
	    grp[m].bg_contained_blocks_count,
	    grp[m].bg_free_blocks_count,
	    grp[m].bg_free_inodes_count,
	    grp[m].bg_used_dirs_count,
	    grp[m].bg_inode_bitmap,
	    grp[m].bg_block_bitmap,
	    grp[m].bg_inode_table);
    m++;
  }

  close(fd_group);
}

void free_bitmap() {
  for (int group_num = 0; group_num < sup->s_groups_count; group_num++) {
    for (int byte_in_block = 0; byte_in_block < sup->s_block_size; byte_in_block++) {
      
      // Reads a single byte at an offset that considers the block size,
      // free block bitmap, and the current byte in the block we're on
      int bit_in_block = 8 * byte_in_block;
      int block_num = grp[group_num].bg_block_bitmap;
      int offset_block = byte_in_block + (sup->s_block_size * block_num);
      pread(fd_disk_image, &buf_1byte, 1, offset_block);

      int isFree;
      int8_t traverse = 1;
      int bit = 0;
      while (bit < 8) {
	// isFree will be set to 1 if the current bit is equal to 0 (free).
	isFree = !((traverse << bit) & buf_1byte);

	// Only print out free blocks
	if (isFree) {
	  dprintf(fd_bitmap, "%x,%d\n", block_num,
		  (sup->s_blocks_per_group * group_num) + bit_in_block + (bit + 1));
	}
	bit++;
      }
    }

    for (int byte_in_inode = 0; byte_in_inode < sup->s_block_size; byte_in_inode++) {
      
      // Reads a single byte at an offset that considers the inode size,
      // free inode bitmap, and the current byte in the inode we're on
      int bit_in_inode = 8 * byte_in_inode;
      int inode_num = grp[group_num].bg_inode_bitmap;
      int offset_inode = byte_in_inode + (sup->s_block_size * inode_num);
      pread(fd_disk_image, &buf_1byte, 1, offset_inode);

      int isFree;
      int8_t traverse = 1;
      int bit = 0;
      while (bit < 8) {
        // isFree will be set to 1 if the current bit is equal to 0 (free).
        isFree = !((traverse << bit) & buf_1byte);

	// Only print out free inodes
        if (isFree) {
	  dprintf(fd_bitmap, "%x,%d\n", inode_num,
		  (sup->s_inodes_per_group * group_num) + bit_in_inode + (bit + 1));
	}
	bit++;
      }
    }
  }
  close(fd_bitmap);
}

void inode() {
  inodes_array = malloc(sizeof(int) * sup->s_inodes_count);
  for (int group_num = 0; group_num < sup->s_groups_count; group_num++) {
    for (int byte_in_inode = 0; byte_in_inode < sup->s_block_size; byte_in_inode++) {
      int bit_in_inode = 8 * byte_in_inode;
      int inode_num = grp[group_num].bg_inode_bitmap;
      int offset_inode = byte_in_inode + (sup->s_block_size * inode_num);
      pread(fd_disk_image, &buf_1byte, 1, offset_inode);

      int isFree;
      int8_t traverse = 1;
      int bit = 0;
      while (bit < 8) {
	// isFree will be set to 1 if the current bit is equal to 0 (free).
	isFree = !((traverse << bit) & buf_1byte);
	if (!isFree && sup->s_inodes_per_group > bit_in_inode + bit) {
	  int inode_offset = (grp[group_num].bg_inode_table * sup->s_block_size) + ((bit_in_inode + bit) * 128);
	  uint64_t temp_size;
	  
	  // Inode Number
	  inodes[inode_cnt].inode_number = (sup->s_inodes_per_group * group_num) + bit_in_inode + (bit + 1);
	  
	  // File Type
	  pread(fd_disk_image, &buf_2bytes, 2, inode_offset + 0);
	  inodes_array[inode_cnt] = inode_offset;
	  if (buf_2bytes & REGULAR)
	    inodes[inode_cnt].file_type = 'f';
	  else if (buf_2bytes & DIRECTORY) {
	    inodes[inode_cnt].file_type = 'd';
	    dirs[dir_cnt].i_offset = inode_offset;
	    dirs[dir_cnt].i_number = inodes[inode_cnt].inode_number;
	    dir_cnt++;
	  }
	  else if (buf_2bytes & SYMBOLIC)
	    inodes[inode_cnt].file_type = 's';
	  else
	    inodes[inode_cnt].file_type = '?';
	  
	  // Mode
	  inodes[inode_cnt].mode = buf_2bytes;
	  
	  // Owner
	  pread(fd_disk_image, &buf_2bytes, 2, inode_offset + 2);
	  inodes[inode_cnt].owner = buf_2bytes;
	  
	  // Group
	  pread(fd_disk_image, &buf_2bytes, 2, inode_offset + 24);
	  inodes[inode_cnt].group = buf_2bytes;
	  
	  // Link Count
	  pread(fd_disk_image, &buf_2bytes, 2, inode_offset + 26);
	  inodes[inode_cnt].link_count = buf_2bytes;
	  
	  // Creation Time
	  pread(fd_disk_image, &buf_4bytes, 4, inode_offset + 12);
          inodes[inode_cnt].creation_time = buf_4bytes;
	  
	  // Modification Time
	  pread(fd_disk_image, &buf_4bytes, 4, inode_offset + 16);
          inodes[inode_cnt].modification_time = buf_4bytes;
	  
	  // Access Time
	  pread(fd_disk_image, &buf_4bytes, 4, inode_offset + 8);
          inodes[inode_cnt].access_time = buf_4bytes;
	  
	  // File Size
	  pread(fd_disk_image, &buf_8bytes, 4, inode_offset + 4);
	  temp_size = buf_8bytes;
	  pread(fd_disk_image, &buf_8bytes, 4, inode_offset + 108);
	  temp_size |= (buf_8bytes << 32);
	  inodes[inode_cnt].file_size = temp_size;
	  
	  // Number of Blocks
	  pread(fd_disk_image, &buf_4bytes, 4, inode_offset + 28);
	  int num_blocks = buf_4bytes/(sup->s_block_size/512);
	  inodes[inode_cnt].number_of_blocks = num_blocks;
	  
	  // Block Pointers * 15
	  int b = 0;
	  while (b < 15) {
	    int block_offset = inode_offset + (4 * b) + 40;
	    pread(fd_disk_image, &buf_4bytes, 4, block_offset);
	    inodes[inode_cnt].block_pointers[b] = buf_4bytes;
	    b++;
	  }
	  buf_4bytes = 0;
	  buf_8bytes = 0;
	  buf_2bytes = 0;
	  inode_cnt++;
	}
	bit++;
      }
    }
  }

  // Sanity Checking
  for (int p = 0; p < inode_cnt; p++) {
    for (int q = 0; q < 15; q++) {
      if (inodes[p].block_pointers[q] > sup->s_blocks_count) {
	fprintf(stderr, "Inode %d - invalid block pointer [%d]: %x\n",
		p, q, inodes[p].block_pointers[q]);
	exit(EXIT_FAILURE);
      }
    }
  }

  // Print to inode.csv
  for (int i = 0; i < inode_cnt; i++) {
    dprintf(fd_inode, "%d,%c,%o,%d,%d,%d,%x,%x,%x,%d,%d,",
	    inodes[i].inode_number,
	    inodes[i].file_type,
	    inodes[i].mode,
	    inodes[i].owner,
	    inodes[i].group,
	    inodes[i].link_count,
	    inodes[i].creation_time,
	    inodes[i].modification_time,
	    inodes[i].access_time,
	    inodes[i].file_size,
	    inodes[i].number_of_blocks);
    for (int j = 0; j < 14; j++) {
      dprintf(fd_inode, "%x,", inodes[i].block_pointers[j]);
    }
    dprintf(fd_inode, "%x\n", inodes[i].block_pointers[14]);
  }

  close(fd_inode);
}

void dir_entry() {
  for (int dir_num = 0; dir_num < dir_cnt; dir_num++) {
    int entry_number = 0;
    uint32_t indirect_blck_num = 0;

    // The first 12 blocks are direct blocks.
    int d_block = 0;
    while (d_block < 12) {
      int blck_offset = dirs[dir_num].i_offset + (4 * d_block) + 40;
      pread(fd_disk_image, &buf_4bytes, 4, blck_offset);
      int dir_offset = buf_4bytes * sup->s_block_size;
      int next_offset = dir_offset + sup->s_block_size;
      if (dir_offset) {
	while (dir_offset < next_offset) {
	  uint32_t parent_inode_num = 0;
	  uint32_t file_entry_inode_num = 0;
	  uint16_t entry_length = 0;
	  uint8_t name_length = 0;

	  // Inode Number of the File Entry
	  pread(fd_disk_image, &buf_4bytes, 4, dir_offset + 0);
	  file_entry_inode_num = buf_4bytes;

	  // Entry Length
	  pread(fd_disk_image, &buf_2bytes, 2, dir_offset + 4);
	  entry_length = buf_2bytes;

	  // Only Print to directory.csv if file entry inode number is nonzero
	  if (file_entry_inode_num) {
	    // Name Length
	    pread(fd_disk_image, &buf_1byte, 1, dir_offset + 6);
	    name_length = buf_1byte;

	    // Parent Inode Number
	    parent_inode_num = dirs[dir_num].i_number;

	    // Sanity Checking
	    if (entry_length < 8 || entry_length > 1024 || name_length > entry_length) {
	      fprintf(stderr, "Inode %d, block %x - bad dirent: len = %d, namelen = %d\n",
		      parent_inode_num, dirs[dir_num].i_offset + (4 * d_block) + 40,
		      entry_length, name_length);
	      exit(EXIT_FAILURE);
	    }
	    if (file_entry_inode_num > sup->s_inodes_count) {
	      fprintf(stderr, "Inode %d, block %x - bad dirent: Inode = %d\n",
		      parent_inode_num, dirs[dir_num].i_offset + (4 * d_block) + 40,
		      file_entry_inode_num);
	      exit(EXIT_FAILURE);
	    }
	    
	    dprintf(fd_directory, "%d,%d,%d,%d,%d,\"",
		    parent_inode_num,
		    entry_number,
		    entry_length,
		    name_length,
		    file_entry_inode_num);

	    char read_nm;
	    int i = 0;
	    while (i < name_length) {
	      pread(fd_disk_image, &read_nm, 1, dir_offset + i + 8);
	      dprintf(fd_directory, "%c", read_nm);
	      i++;
	    }
	    dprintf(fd_directory, "\"\n");
	  }

	  // Entry number and offset values should always be appended
	  // to, regardless of the file entry inode number value
	  entry_number++;
	  dir_offset = dir_offset + entry_length;
	}
      }
      d_block++;
    }
    
    // The 13th entry is the first indirect block
    pread(fd_disk_image, &buf_4bytes, 4, dirs[dir_num].i_offset + (4 * 12) + 40);
    indirect_blck_num = buf_4bytes;
    if (buf_4bytes) {
      int i_block = 0;
      while (i_block < sup->s_block_size/4) {
	int dir_offset = (indirect_blck_num * sup->s_block_size) + (4 * i_block);
	pread(fd_disk_image, &buf_4bytes, 4, dir_offset);
	dir_offset = buf_4bytes * sup->s_block_size;
	int next_offset = dir_offset + sup->s_block_size;
	if (dir_offset) {
	  while (dir_offset < next_offset) {
	    uint32_t parent_inode_num = 0;
	    uint32_t file_entry_inode_num = 0;
	    uint16_t entry_length = 0;
	    uint8_t name_length = 0;

	    // Inode Number of the File Entry
	    pread(fd_disk_image, &buf_4bytes, 4, dir_offset + 0);
	    file_entry_inode_num = buf_4bytes;

	    // Entry Length
	    pread(fd_disk_image, &buf_2bytes, 2, dir_offset + 4);
	    entry_length = buf_2bytes;

	    // Only Print to directory.csv if file entry inode number is nonzero
	    if (file_entry_inode_num) {
	      // Name Length
	      pread(fd_disk_image, &buf_1byte, 1, dir_offset + 6);
	      name_length = buf_1byte;
	      
	      // Parent Inode Number
	      parent_inode_num = dirs[dir_num].i_number;

	      // Sanity Checking
	      if (entry_length < 8 || entry_length > 1024 || name_length > entry_length) {
		fprintf(stderr, "Inode %d, block %x - bad dirent: len = %d, namelen = %d\n",
			parent_inode_num, dirs[dir_num].i_offset + (4 * d_block) + 40,
			entry_length, name_length);
		exit(EXIT_FAILURE);
	      }
	      if (file_entry_inode_num > sup->s_inodes_count) {
		fprintf(stderr, "Inode %d, block %x - bad dirent: Inode = %d\n",
			parent_inode_num, dirs[dir_num].i_offset + (4 * d_block) + 40,
			file_entry_inode_num);
		exit(EXIT_FAILURE);
	      }
	      
	      dprintf(fd_directory, "%d,%d,%d,%d,%d,\"",
		      parent_inode_num,
		      entry_number,
		      entry_length,
		      name_length,
		      file_entry_inode_num);
	      
	      char read_nm;
	      int i = 0;
	      while (i < name_length) {
		pread(fd_disk_image, &read_nm, 1, dir_offset + i + 8);
		dprintf(fd_directory, "%c", read_nm);
		i++;
	      }
	      dprintf(fd_directory, "\"\n");
	    }

	    // Entry number and offset values should be appended to,
	    // regardless of the file entry inode number value
	    entry_number++;
	    dir_offset = dir_offset + entry_length;
	  }
	}
	i_block++;
      }
    }
  }

  close(fd_directory);
}

void indir_block_entry() {
  int entry_num_in_block = 0;
  int block_num = 0;
  int block_ptr = 0;
  int i_node = 0;
  while (i_node < inode_cnt) {
    int indir_offset = inodes_array[i_node] + (4 * 12) + 40;
    pread(fd_disk_image, &buf_4bytes, 4, indir_offset);
    block_num = buf_4bytes;
    int indirect = 0;
    while (indirect < sup->s_block_size/4) {
      int quad_indir = 4 * indirect;
      int blck_ptr_offset = (sup->s_block_size * block_num) + quad_indir;
      pread(fd_disk_image, &buf_4bytes, 4, blck_ptr_offset);
      block_ptr = buf_4bytes;
      if(block_ptr) {
	
	// Sanity Checking
	if (block_ptr > sup->s_blocks_count) {
	  fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x\n",
		  block_num, entry_num_in_block, block_ptr);
	  exit(EXIT_FAILURE);
	}
	
	dprintf(fd_indirect, "%x,%d,%x\n",
		block_num, entry_num_in_block, block_ptr);
	entry_num_in_block++;
      }
      indirect++;
    }
    i_node++;
  }
  
  close(fd_indirect);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Only one argument 'disk-image' is allowed!\n");
    exit(EXIT_FAILURE);
  }
  
  sup = malloc(sizeof(struct super_block));
  grp = malloc(sizeof(struct group_block));
  inodes = malloc(sizeof(struct inode_block) * MAX_VALUE);
  dirs = malloc(sizeof(struct dir_block) * MAX_VALUE);
  disk_img_file = malloc(sizeof(char) * strlen(argv[1]+1));
  disk_img_file = argv[1];
  
  fd_disk_image = open(disk_img_file, O_RDONLY);
  fd_super = creat("super.csv", S_IRUSR | S_IWUSR);
  fd_group = creat("group.csv", S_IRUSR | S_IWUSR);
  fd_bitmap = creat("bitmap.csv", S_IRUSR | S_IWUSR);
  fd_inode = creat("inode.csv", S_IRUSR | S_IWUSR);
  fd_directory = creat("directory.csv", S_IRUSR | S_IWUSR);
  fd_indirect = creat("indirect.csv", S_IRUSR | S_IWUSR);

  super_block();
  group_descr();
  free_bitmap();
  inode();
  dir_entry();
  indir_block_entry();

  close(fd_disk_image);
  return 0;
}
