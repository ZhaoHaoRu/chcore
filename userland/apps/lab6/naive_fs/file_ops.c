#include <string.h>
#include <stdio.h>

#include "file_ops.h"
#include "block_layer.h"

void num2str(int val, char *result) {
	char tmp[10] = {'\0'};
	int i = 0;
	while (val > 0) {
		tmp[i] = val % 10 + '0';
		i += 1;
		val /= 10;
	}
	int len = strlen(tmp);
	i = 0;
	for (int j = len - 1; j >= 0; --j) {
		result[j] = tmp[i];
		++i;
	}
}


int find_file_by_name (const char *name) {
    char buffer[BLOCK_SIZE] = {'\0'};
    int ret = sd_bread(0, buffer);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }
    int i = 0, j = 0;
    char name_buf[BLOCK_SIZE];

    while (i < BLOCK_SIZE) {
        if (buffer[i] == '\0') {
            break;
        }
        if (buffer[i] == '/') {
            if (strncmp(name_buf, name, strlen(name)) == 0) {
                // find the file
                printf("[DEBUG] find the file: %s, the wanted: %s\n", name_buf, name);
                int block_id = 0;
                while (buffer[i] != '/') {
                    block_id = block_id * 10 + buffer[i] - '0';
                    ++i;
                }
                return block_id;
            }
            i += 1;
            while (i < BLOCK_SIZE && buffer[i] != '/') {
                ++i;
            }
            i += 1;
            memset(name_buf, '\0', BLOCK_SIZE);
            j = 0;
        } else {
            name_buf[j] = buffer[i];
            j += 1;
            i += 1;
        }
    }
    return -1;
}


int naive_fs_access(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    // for convience, the root dir content all in the first block
    // the second block is the bitmap
    // the format is: [name]/[block id]/[name]/[block id]/[name]/[block id]/
    printf("[DEBUG] navie_fs_access: name: %s\n", name);
    int ret = find_file_by_name(name);
    if (ret >= 0) {
        return 0;
    }
    return -1;
    /* BLANK END */
    /* LAB 6 TODO END */
    return -2;
}

int naive_fs_creat(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    printf("[DEBUG] naive_fs_creat: name: %s\n", name);
    int ret = find_file_by_name(name);
    if (ret >= 0) {
        return -1;
    }
    // check the bitmap
    char bitmap[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(1, bitmap);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }
    int block_id = 2;
    while (block_id < BLOCK_SIZE) {
        if (bitmap[block_id] != '1') {
            bitmap[block_id] = '1';
            break;
        }
        ++block_id;
    }
    printf("the choose block id is %d\n", block_id);
    if (block_id == BLOCK_SIZE) {
        printf("no enough space\n");
        return -1;
    }

    int i = 0;
    // write the bitmap
    ret = sd_bwrite(1, bitmap);
    if (ret == -1) {
        printf("write failed\n");
        return -1;
    }

    // write the directory
    char dir[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(0, dir);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }
    i = 0;
    printf("the dir before create is %s\n", dir);
    while (i < BLOCK_SIZE && dir[i] != '\0') {
        ++i;
    }
    // copy the name and directory
    int j = 0;
    while (i < BLOCK_SIZE && j < strlen(name)) {
        dir[i] = name[j];
        ++i;
        ++j;
    }
    if (i == BLOCK_SIZE) {
        printf("no enough space\n");
        return -1;
    }

    dir[i] = '/';
    i += 1;
    if (i == BLOCK_SIZE) {
        printf("no enough space\n");
        return -1;
    }
    // write the block num
    char block_num[10] = {'\0'};

    // sprintf(block_num, "%d", i);
    num2str(block_id, block_num);
    j = 0;

    while (i < BLOCK_SIZE && j < strlen(block_num)) {
        dir[i] = block_num[j];
        ++i;
        ++j;
    }
    if (i == BLOCK_SIZE) {
        printf("no enough space\n");
        return -1;
    }

    dir[i] = '/';
    i += 1;

    if (i == BLOCK_SIZE) {
        printf("no enough space\n");
        return -1;
    }

    printf("[DEBUG] after create, the dir is %s\n", dir);
    ret = sd_bwrite(0, dir);
    if (ret == -1) {
        printf("write failed\n");
        return -1;
    }

    return 0;
    /* BLANK END */
    /* LAB 6 TODO END */
    return -2;
}

int naive_fs_pread(const char *name, int offset, int size, char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    printf("[DEBUG] naive_fs_pread: name: %s\n", name);
    int ret = find_file_by_name(name);
    if (ret == -1) {
        return -1;
    }
    char buf[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(ret, buf);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }
    int read_size = BLOCK_SIZE - offset <  size ? BLOCK_SIZE - offset : size;
    memcpy(buffer, buf + offset, read_size);
    return read_size;
    /* BLANK END */
    /* LAB 6 TODO END */
    return -2;
}

int naive_fs_pwrite(const char *name, int offset, int size, const char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    printf("[DEBUG] naive_fs_pwrite: name: %s\n", name);
    int ret = find_file_by_name(name);
    if (ret == -1) {
        return -1;
    }
    char buf[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(ret, buf);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }
    int write_size = BLOCK_SIZE - offset <  size ? BLOCK_SIZE - offset : size;
    memcpy(buf + offset, buffer, write_size);
    ret = sd_bwrite(ret, buf);
    if (ret == -1) {
        printf("write failed\n");
        return -1;
    }
    return write_size;
    /* BLANK END */
    /* LAB 6 TODO END */
    return -2;
}

int naive_fs_unlink(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    printf("[DEBUG] naive_fs_unlink: name: %s\n", name);
    int ret = find_file_by_name(name);
    if (ret == -1) {
        return -1;
    }

    // delete the file in bitmap 
    char bitmap[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(1, bitmap);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }
    bitmap[ret] = '0';
    ret = sd_bwrite(1, bitmap);
    if (ret == -1) {
        printf("write failed\n");
        return -1;
    }

    // delete the file in directory
    char dir[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(0, dir);
    if (ret == -1) {
        printf("read failed\n");
        return -1;
    }

    int i = 0, j = 0, offset = 0;
    char name_buf[BLOCK_SIZE] = {'\0'};
    while (i < BLOCK_SIZE) {
        if (dir[i] == '\0') {
            break;
        }
        if (dir[i] == '/') {
            // /name/block_id/
            if (strncmp(dir + j, name, strlen(name)) == 0) {
                int times = 0;
                while (times < 3 && i < BLOCK_SIZE) {
                    if (dir[i] == '/') {
                        times += 1;
                    }
                    i += 1;
                    offset += 1;
                }
            } else {
                j = i + 1;
            }
        } 
        dir[i - offset] = dir[i];
    }

    ret = sd_bwrite(0, dir);
    if (ret == -1) {
        printf("write failed\n");
        return -1;
    }

    return 0;
    /* BLANK END */
    /* LAB 6 TODO END */
    return -2;
}
