#!/usr/bin/python

import locale, sys, string

#******************* Global variables, arrays, and dictionaries *******************#

# Necessary info from super.csv

fd_spr = open('super.csv', 'r')
spr_lines = fd_spr.readlines()
fd_spr.close()
arr_line_spr = spr_lines[0].rstrip('\n').split(',')

inodes_per_group = int(arr_line_spr[6])

# Arrays containing inode bitmap block and
# block bitmap block values from group.csv

inode_bitmap = []
block_bitmap = []

fd_grp = open('group.csv', 'r')
num_lines_grp = sum(1 for line in open('group.csv'))
grp_lines = fd_grp.readlines()
fd_grp.close()

for i in range(0, num_lines_grp):
    arr_line_grp = grp_lines[i].rstrip('\n').split(',')
    inode_bitmap.append(arr_line_grp[4])
    block_bitmap.append(arr_line_grp[5])

# Arrays containing free inodes and free blocks by checking if
# each block number in bitmap.csv is in the inode or block
# bitmap arrays above

free_inode_bitmap = []
free_block_bitmap = []

fd_btmp = open('bitmap.csv', 'r')
num_lines_btmp = sum(1 for line in open('bitmap.csv'))
btmp_lines = fd_btmp.readlines()
fd_btmp.close()

for i in range(0, num_lines_btmp):
    arr_line_btmp = btmp_lines[i].rstrip('\n').split(',')
    map_block_num = arr_line_btmp[0]
    free_block_or_inode_num = int(arr_line_btmp[1])
    if (map_block_num in inode_bitmap):
        free_inode_bitmap.append(free_block_or_inode_num)
    if (map_block_num in block_bitmap):
        free_block_bitmap.append(free_block_or_inode_num)

# Open and collect lines data from the remaining CSV files,
# so it can be used in the error checking and reporting functions.

fd_ind = open('inode.csv', 'r')
num_lines_ind = sum(1 for line in open('inode.csv'))
ind_lines = fd_ind.readlines()
fd_ind.close()

fd_dir = open('directory.csv', 'r')
num_lines_dir = sum(1 for line in open('directory.csv'))
dir_lines = fd_dir.readlines()
fd_dir.close()

# Initialize directories

inode_nums_free_bitmap = {}
entry_nums_free_bitmap = {}
inode_nums_all = {}
entry_nums_all = {}

#*********************** Error Checking and Reporting Functions ***********************#

def unallocated_block(ofd):
    for i in range(0, num_lines_ind):
        arr_line_ind = ind_lines[i].rstrip('\n').split(',')
        arr_block_ptrs = arr_line_ind[11:]
        block_num = 0
        for elem in arr_block_ptrs:
            block_num_decimal = int(elem, 16)
            str_block_num_decimal = str(block_num_decimal)
            if (block_num < 12):
                if (block_num_decimal in free_block_bitmap):
                    if (str_block_num_decimal not in inode_nums_free_bitmap and
                        str_block_num_decimal not in entry_nums_free_bitmap):
                        inode_nums_free_bitmap[str_block_num_decimal] = str(arr_line_ind[0])
                        entry_nums_free_bitmap[str_block_num_decimal] = str(block_num)
                if (str_block_num_decimal not in inode_nums_all and
                    str_block_num_decimal not in entry_nums_all):
                    inode_nums_all[str_block_num_decimal] = []
                    entry_nums_all[str_block_num_decimal] = []
                inode_nums_all[str_block_num_decimal].append(str(arr_line_ind[0]))
                entry_nums_all[str_block_num_decimal].append(str(block_num))
                block_num += 1
    for str_blck_dec in inode_nums_free_bitmap:
        ofd.write("UNALLOCATED BLOCK < " + str_blck_dec + " > REFERENCED BY INODE < " +
                  inode_nums_free_bitmap[str_blck_dec] + " > ENTRY < " + entry_nums_free_bitmap[str_blck_dec] + " >\n")

def duplicately_allocated_block(ofd):
    for str_blck_dec in inode_nums_all:
        if (str_blck_dec != '0' and len(inode_nums_all[str_blck_dec]) >= 2):
            inds = []
            entrs = []
            ofd.write("MULTIPLY REFERENCED BLOCK < " + str_blck_dec + " > BY")
            for ind in inode_nums_all[str_blck_dec]:
                inds.append(ind)
            for entr in entry_nums_all[str_blck_dec]:
                entrs.append(entr)
            for i in range(0, len(inds)):
                ofd.write(" INODE < " + inds[i] + " > ENTRY < " + entrs[i] + " >")
            ofd.write("\n")

def unallocated_inode(ofd):
    pass

def missing_inode(ofd):
    for j in range(0, num_lines_ind):
        arr_line_ind = ind_lines[j].rstrip('\n').split(',')
        ind_num = int(arr_line_ind[0])
        link_cnt = int(arr_line_ind[5])
        inode_bitmap_len = len(inode_bitmap)
        free_list = 0
        if (str(ind_num) not in free_inode_bitmap and link_cnt == 0):
            if (ind_num > 10):
                ind_elem = ind_num // inodes_per_group
                free_list = int(inode_bitmap[ind_elem])
                ofd.write("MISSING INODE < " + str(ind_num) + " > SHOULD BE IN FREE LIST < " + str(free_list) + " >\n")
                
def incorrect_link_count(ofd):
    i = 0
    while (i < num_lines_ind):
        arr_line_ind = ind_lines[i].rstrip('\n').split(',')
        ind_num = int(arr_line_ind[0])
        link_cnt = int(arr_line_ind[5])
        dir_lnk_cnt_check = 0
        j = 0
        while (j < num_lines_dir):
            arr_line_dir = dir_lines[j].rstrip('\n').split(',')
            file_entry_ind_num = int(arr_line_dir[4])
            if (file_entry_ind_num == ind_num):
                dir_lnk_cnt_check += 1
            j += 1
        if (link_cnt != dir_lnk_cnt_check):
            ofd.write("LINKCOUNT < " + str(ind_num) + " > IS < " + str(link_cnt) +
                      " > SHOULD BE < " + str(dir_lnk_cnt_check) + " >\n")
        i += 1

def incorrect_directory_entry(ofd):
    for i in range(0, num_lines_dir):
        arr_line_dir = dir_lines[i].rstrip('\n').split(',')
        name = str(arr_line_dir[5])
        parent_inode_num = str(arr_line_dir[0])
        file_entry_inode_num = str(arr_line_dir[4])
        
        # NAME < . >
        if (name == '"."'):
            if (parent_inode_num != file_entry_inode_num):
                ofd.write("INCORRECT ENTRY IN < " + parent_inode_num +
                          " > NAME < . > LINK TO < " + file_entry_inode_num +
                          " > SHOULD BE < " + parent_inode_num + " >\n")
        # NAME < .. >
        elif (name == '".."'):
            j = 0
            while (j < num_lines_dir):
                arr_line_dir_cmp = dir_lines[j].rstrip('\n').split(',')
                name_cmp = str(arr_line_dir_cmp[5])
                parent_inode_num_cmp = str(arr_line_dir_cmp[0])
                file_entry_inode_num_cmp = str(arr_line_dir_cmp[4])

                if (name_cmp != '"."' and name_cmp != '".."'):
                    if (parent_inode_num == file_entry_inode_num_cmp and
                        parent_inode_num_cmp != file_entry_inode_num):
                        ofd.write("INCORRECT ENTRY IN < " + parent_inode_num +
                                  " > NAME < .. > LINK TO < " + file_entry_inode_num +
                                  " > SHOULD BE < " + parent_inode_num_cmp + " >\n")
                j += 1

def invalid_block_pointer(ofd):
    for i in range(0, num_lines_ind):
        arr_line_ind = ind_lines[i].rstrip('\n').split(',')
        num_blocks = int(arr_line_ind[10])
        ind_num = str(arr_line_ind[0])
        arr_block_ptrs = arr_line_ind[11:]
        block_num = 0
        for elem in arr_block_ptrs:
            block_num_decimal = int(elem, 16)
            str_block_num_decimal = str(block_num_decimal)
            if (block_num < num_blocks):
                if (block_num_decimal == 0 and num_blocks <= 15):
                    ofd.write("INVALID BLOCK < " + str_block_num_decimal + " > IN INODE < " +
                              ind_num + " > ENTRY < " + str(block_num) + " >\n")
                block_num += 1
    
#********************************** Main Function **********************************#

def main():
    fd_output = open('lab3b_check.txt', 'w+')
    
    unallocated_block(fd_output)
    duplicately_allocated_block(fd_output)
    unallocated_inode(fd_output)
    missing_inode(fd_output)
    incorrect_link_count(fd_output)
    incorrect_directory_entry(fd_output)
    invalid_block_pointer(fd_output)

    fd_output.close()

if __name__ == "__main__":
    main()
