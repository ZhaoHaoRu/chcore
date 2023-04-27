/*
 * Copyright (c) 2022 Institute of Parallel And Distributed Systems (IPADS)
 * ChCore-Lab is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#include <libc/stdio.h>
#include <string.h>
#include <chcore/types.h>
#include <chcore/fsm.h>
#include <chcore/tmpfs.h>
#include <chcore/ipc.h>
#include <chcore/internal/raw_syscall.h>
#include <chcore/internal/server_caps.h>
#include <chcore/procm.h>
#include <chcore/fs/defs.h>

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#define va_copy(d, s)  __builtin_va_copy(d, s)
#define BUF_LEN 1024

extern struct ipc_struct *fs_ipc_struct;

/* You could add new functions or include headers here.*/
/* LAB 5 TODO BEGIN */
int alloc_file_descripter() {
	static int fd = 0;
	int ret = fd;
	fd += 1;
	return ret;
}

int str2num(const char *str, int *pos) {
	int base = 0;
	while (*(str + *pos) >= '0' && *(str + *pos) <= '9') {
		base = base * 10 + (*(str + *pos) - '0');
		*pos += 1;
	}
	return base;
}

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

int get_mode(const char *mode) {
	if (*mode == 'r') {
		return O_RDONLY;
	} else if (*mode == 'w' && *(mode + 1) == '\0') {
		return O_WRONLY;
	} else {
		return O_RDWR;
	}
}

// return the str length
int getstr(const char *str, int pos) {
	int split_pos = pos;
	while (*(str + split_pos) != '\0' && *(str + split_pos) != ' ') {
		++split_pos;
	}
	return split_pos;
}
/* LAB 5 TODO END */

// abstract the fs_open_file function to reduce duplicate code
int fs_open_file(const char *filename, int fd, const char *mode) {
	printf("[DEBUG] begin fs_open_file\n");
	struct ipc_msg *new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	printf("[DEBUG] create new msg\n");
	if (new_msg == NULL) {
		printf("[fs_open_file] create new msg fail\n");
	}
	struct fs_request *req = (struct fs_request*)ipc_get_msg_data(new_msg);
	printf("[DEBUG] get msg data\n");
	req->req = FS_REQ_OPEN;
	req->open.new_fd = fd;
	memcpy(req->open.pathname, filename, FS_REQ_PATH_BUF_LEN);
	req->open.flags = get_mode(mode);
	printf("[DEBUG] the file open flags: %d\n", req->open.flags);
	int ret = ipc_call(fs_ipc_struct, new_msg);
	printf("[DEBUG] the file open ret: %d\n", ret);
	ipc_destroy_msg(fs_ipc_struct, new_msg);
	return ret;
}
FILE *fopen(const char * filename, const char * mode) {

	/* LAB 5 TODO BEGIN */
	// create necessary data structure
	// struct ipc_msg *new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	// if (new_msg == NULL) {
	// 	printf("[fopen] create new msg fail\n");
	// }
	// struct fs_request *req = (struct fs_request*)ipc_get_msg_data(new_msg);
	// req->req = FS_REQ_OPEN;
	// memcpy(req->open.pathname, filename, FS_REQ_PATH_BUF_LEN);
	struct FILE *file = (struct FILE*)malloc(sizeof(int) * 2);
	// if (*mode == 'r') {
	// 	req->open.flags = O_RDONLY;
	// 	req->open.mode = 0;
	// 	file->mode = 0; 
	// } else {
	// 	req->open.flags = O_WRONLY;
	// 	req->open.mode = 1;
	// 	file->mode = 1;
	// }

	int fd = alloc_file_descripter();
	printf("[DEBUG] the file fd: %d\n", fd);
	// req->open.new_fd = fd;
	
	// int ret = ipc_call(fs_ipc_struct, new_msg);
	// ipc_destroy_msg(fs_ipc_struct, new_msg);
	int ret = fs_open_file(filename, fd, mode);
	printf("[DEBUG] the file open ret: %d\n", ret);
	if (ret >= 0) {	// the file exist, just return
		printf("[DEBUG] the file exist\n");
		file->fd = fd;
		file->mode = get_mode(mode);
		printf("[DEBUG] the file mode: %d\n", file->mode);
		return file;
	} else {	// the file not exist, create it
		struct ipc_msg *new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
		if (new_msg == NULL) {
			printf("[fopen] create [create] msg fail\n");
		}
		struct fs_request *req = (struct fs_request*)ipc_get_msg_data(new_msg);
		req->req = FS_REQ_CREAT;
		memcpy(req->creat.pathname, filename, strlen(filename));
		req->creat.mode = 1;

		ret = ipc_call(fs_ipc_struct, new_msg);
		ipc_destroy_msg(fs_ipc_struct, new_msg);
		if (ret >= 0) {
			file->fd = fd;
			file->mode = get_mode(mode);
			fs_open_file(filename, fd, mode);
			return file;
		} else {
			printf("[fopen] create file fail\n");
		}
	}
	/* LAB 5 TODO END */
    return NULL;
}

size_t fwrite(const void * src, size_t size, size_t nmemb, FILE * f) {

	/* LAB 5 TODO BEGIN */
	// create necessary data structure
	struct ipc_msg *new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request) + nmemb * size + 1, 0);
	printf("[fwrite] the new message size: %d\n", sizeof(struct fs_request) + nmemb * size + 1);
	if (new_msg == NULL) {
		printf("[fwrite] create new msg fail\n");
	}
	struct fs_request* req = (struct fs_request*)ipc_get_msg_data(new_msg);
	req->req = FS_REQ_WRITE;
	req->write.fd = f->fd;
	printf("[DEBUG] the fwrite fd: %d, %d\n", req->write.fd, f->fd);
	req->write.count = nmemb * size;
	int ret = ipc_set_msg_data(new_msg, (void*)src, sizeof(struct fs_request), nmemb * size + 1);
	if (ret != 0) {
		printf("[fwrite] set message data fail\n");
		return ret;
	}
	printf("[DEBUG] the fwrite fd: %d, %d\n", req->write.fd, f->fd);
	printf("[DEBUG] fwrite ipc call\n");
	ret = ipc_call(fs_ipc_struct, new_msg);
	ipc_destroy_msg(fs_ipc_struct, new_msg);
	return ret;
	/* LAB 5 TODO END */
    return 0;

}

size_t fread(void * destv, size_t size, size_t nmemb, FILE * f) {

	/* LAB 5 TODO BEGIN */
	// first check the argument validation
	if (f == NULL) {
		printf("[fread] invalid parameter\n");
		return -1;
	}
	
	struct ipc_msg *new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request) + size * nmemb + 2, 0);
	if (new_msg == NULL) {
		printf("[fread] create new msg fail\n");
		return -1;
	}
	struct fs_request *req = (struct fs_request*)ipc_get_msg_data(new_msg);
	req->req = FS_REQ_READ;
	req->read.fd = f->fd;
	req->read.count = size * nmemb;
	
	int ret = ipc_call(fs_ipc_struct, new_msg);
	printf("[DEBUG] the fread ipc_call ret: %d\n", ret);
	if (ret < 0) {
		printf("[fread] read process wrong, the return value: %d\n", ret);
	}
	if (ret > 0) {
		memcpy(destv, ipc_get_msg_data(new_msg), ret);
	}
	printf("[DEBUG] the destv in fread: %s\n", (char*)destv);
	ipc_destroy_msg(fs_ipc_struct, new_msg);
	/* LAB 5 TODO END */
    return ret;

}

int fclose(FILE *f) {

	/* LAB 5 TODO BEGIN */
	if (f == NULL) {
		printf("[fclose] invalid parameter\n");
		return -1;
	}
	struct ipc_msg *new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	if (new_msg == NULL) {
		printf("[fclose] create new msg fail\n");
		return -1;
	} 
	struct fs_request* req = (struct fs_request*)ipc_get_msg_data(new_msg);
	req->req = FS_REQ_CLOSE;
	req->close.fd = f->fd;
	int ret = ipc_call(fs_ipc_struct, new_msg);
	ipc_destroy_msg(fs_ipc_struct, new_msg);
	return ret;
	/* LAB 5 TODO END */
    return 0;

}

/* Need to support %s and %d. */
int fscanf(FILE * f, const char * fmt, ...) {

	/* LAB 5 TODO BEGIN */
	va_list arg;
	va_start(arg, fmt);
	if (f == NULL) {
		printf("[fscanf] invalid parameter\n");
		return -1;
	}

	int ptr = 0, fmt_ptr = 0, fmt_size = strlen(fmt);
	char buf[BUF_LEN] = {'\0'};
	int size = fread(buf, 1, BUF_LEN, f);

	while (ptr < size && fmt_ptr < fmt_size) {
		if (*(fmt + fmt_ptr) == '%') {
			++fmt_ptr;
			if (*(fmt_ptr + fmt) == 's') {
				int split_pos = getstr(buf, ptr);
				char *current_arg = va_arg(arg, char*);
				memcpy(current_arg, ptr + buf, split_pos - ptr);
				ptr = split_pos + 1;
				
			} else if (*(fmt_ptr + fmt) == 'd') {
				int *current_arg = va_arg(arg, int*);
				*current_arg = str2num(buf, &ptr);
				++ptr;
			}
		} 
		++fmt_ptr;
	}

	// TODO: whether need lseek?
	// new_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
	// if (new_msg == NULL) {
	// 	printf("[fscanf] create lseek new msg fail\n");
	// 	return -1;
	// }
	/* LAB 5 TODO END */
    return 0;
}

/* Need to support %s and %d. */
int fprintf(FILE * f, const char * fmt, ...) {

	/* LAB 5 TODO BEGIN */
	// generate the string
	va_list arg;
	va_start(arg, fmt);
	char buf[BUF_LEN] = {'\0'};
	int ptr = 0, fmt_ptr = 0, fmt_size = strlen(fmt);
	while (fmt_ptr < fmt_size) {
		if (*(fmt_ptr + fmt) == '%') {
			++fmt_ptr;
			if (*(fmt_ptr + fmt) == 'd') {
				int current_arg = va_arg(arg, int);
				printf("[DEBUG] current int arg: %d\n", current_arg);
				char int_str[10] = {'\0'};
				num2str(current_arg, int_str);
				// char* int_str = num2str(current_arg);
				memcpy(buf + ptr, int_str, strlen(int_str));
				ptr += strlen(int_str);
			} else if (*(fmt_ptr + fmt) == 's') {
				char *current_arg = va_arg(arg, char*);
				memcpy(buf + ptr, current_arg, strlen(current_arg));
				ptr += strlen(current_arg);
			}
			++fmt_ptr;
		} else {
			buf[ptr] = *(fmt + fmt_ptr);
			++ptr;
			++fmt_ptr;
		}
	}
	printf("[fprintf] the format string is %s\n", buf);
	int ret = fwrite(buf, 1, strlen(buf), f);
	va_end(arg);
	return ret;
	/* LAB 5 TODO END */
}

