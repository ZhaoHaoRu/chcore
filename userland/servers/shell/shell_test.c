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

#include "shell.h"

#include <libc/stdio.h>
#include <string.h>
#include <malloc.h>
#include <chcore/ipc.h>
#include <chcore/assert.h>
#include <chcore/procm.h>
#include <chcore/fsm.h>
#include <chcore/thread.h>
#include <chcore/internal/server_caps.h>
#include <chcore/internal/raw_syscall.h>


#define TEST_FUNC(name) \
  do { \
    if (name() == 0) { \
      printf(#name" pass!\n"); \
    } else { \
      printf(#name" fail!\n"); \
    } \
  } while (0)

extern struct ipc_struct *fs_ipc_struct;

char wbuf[256];
char rbuf[32];
char rbuf2[32];

static void test_readline()
{
	char* buf = readline("1>");
	printf("%s\n", buf);
	readline("2>");
}

static void test_echo()
{
	builtin_cmd("echo abc123XYZ");
}

static void test_ls() {
	builtin_cmd("ls /");
}

static void test_cat() {
	builtin_cmd("cat /test.txt");
}

static void test_top() {
	do_top();
}

static void test_run() {
	run_cmd("/helloworld.bin");
}

static int lab5_stdio_file_read_write () {
    memset(wbuf, 0x0, sizeof(wbuf));
    memset(rbuf, 0x0, sizeof(rbuf));
    for(int i = 0; i < sizeof(wbuf); ++i) {
        wbuf[i] = (char) (i % 26 + 'a');
    }
    FILE * pFile;
    pFile = fopen("/myfile.txt", "w");
	printf("[DEBUG] %s %d\n", __func__, 78);
    fwrite(wbuf, sizeof(char) , sizeof(wbuf), pFile);
	printf("[DEBUG] %s %d\n", __func__, 80);
    fclose(pFile);
	printf("[DEBUG] %s %d\n", __func__, 82);
    pFile = fopen("/myfile.txt", "r");
	printf("[DEBUG] %s %d\n", __func__, 84);
    int cnt;
    do {
		printf("[DEBUG] the previous content: %s\n", rbuf);
        cnt = fread(rbuf, sizeof(char), sizeof(rbuf), pFile);
		printf("[DEBUG] fread size: %d, the content: %s\n", cnt, rbuf);
    } while(cnt > 0);
    fclose(pFile);
	printf("the target: %s\n", (char*)wbuf + sizeof(wbuf) - sizeof(rbuf));
	printf("the result: %s\n", rbuf);
	printf("[DEBUG] the compare result: %d\n", memcmp(rbuf, (char*)wbuf + sizeof(wbuf) - sizeof(rbuf), sizeof(rbuf)));
    return memcmp(rbuf, (char*)wbuf + sizeof(wbuf) - sizeof(rbuf), sizeof(rbuf));
}

static int lab5_stdio_file_printf_scanf () {
    memset(rbuf, 0x0, sizeof(rbuf));
    memset(rbuf2, 0x0, sizeof(rbuf2));
    void* ptr = malloc(sizeof(char) * 4);
    int data = *(int*)&ptr;

    FILE * pFile;
    pFile = fopen("/myfile2.txt", "w");
	printf("[DEBUG] the raw date for fprintf: %d\n", data);
    fprintf(pFile, "fprintf %s %d\n", __func__, data);
    fclose(pFile);

    int outdata;
    pFile = fopen("/myfile2.txt", "r");
    fscanf(pFile, "%s %s %d", rbuf, rbuf2, &outdata);
	fclose(pFile);
	free(ptr);

    return strncmp(rbuf, "fprintf", 7) != 0 || strcmp(rbuf2, __func__) != 0 || outdata != data;
}

static void test_stdio() {
	TEST_FUNC(lab5_stdio_file_read_write);
	TEST_FUNC(lab5_stdio_file_printf_scanf);
}

void shell_test() {
	test_stdio();
	test_echo();
	printf("\nSHELL ");
	test_ls();
	printf("\nSHELL ");
	test_cat();
	printf("\n");
	test_readline();
	test_run();
	for (int i = 0; i < 1000; i++) {
        __chcore_sys_yield(); 
	}		
}

void fsm_test() {

	int fsm_cap = __chcore_get_fsm_cap();
	chcore_assert(fsm_cap >= 0);
	fs_ipc_struct = ipc_register_client(fsm_cap);
	chcore_assert(fs_ipc_struct);
	mount_fs("/fakefs.srv", "/fakefs");	
	printf("\nFSM ");
	builtin_cmd("ls /");
	printf("\nFSM ");
	builtin_cmd("ls /fakefs");
	printf("\nFSM ");
	builtin_cmd("cat /fakefs/test.txt");
	printf("\nFSM ");
	builtin_cmd("cat /test.txt");
	printf("\n");

}