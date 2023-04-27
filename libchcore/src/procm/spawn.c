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

#include <chcore/procm.h>
#include <chcore/assert.h>
#include <string.h>
#include <stdio.h>

#include "server.h"

int chcore_procm_spawn(const char *path, int *mt_cap_out)
{
        // printf("[DEBUG] chcore_procm_spawn: get line 22\n");
        struct ipc_struct *procm_ipc_struct = get_procm_server();
        // printf("[DEBUG] chcore_procm_spawn: procm_ipc_struct %lx\n", procm_ipc_struct);
        struct ipc_msg *ipc_msg = ipc_create_msg(
                procm_ipc_struct, sizeof(struct procm_ipc_data), 0);
        // printf("[DEBUG] chcore_procm_spawn: get line 27\n");
        chcore_assert(ipc_msg);
        // printf("[DEBUG] chcore_procm_spawn: get line 26\n");
        struct procm_ipc_data *procm_ipc_data =
                (struct procm_ipc_data *)ipc_get_msg_data(ipc_msg);
        // printf("[DEBUG] chcore_procm_spawn: get line 29\n");
        procm_ipc_data->request = PROCM_IPC_REQ_SPAWN;
        strncpy(procm_ipc_data->spawn.args.path, path, PROCM_PATH_LEN - 1);
        // printf("[DEBUG] procm_ipc_data->spawn.args.path: %s\n", procm_ipc_data->spawn.args.path);
        procm_ipc_data->spawn.args.path[PROCM_PATH_LEN - 1] = '\0';
        // printf("[DEBUG] chcore_procm_spawn: get line 33\n");
        int ret = ipc_call(procm_ipc_struct, ipc_msg);
        // printf("[DEBUG] chcore_procm_spawn: get line 35\n");
        if (ret == 0) {
                ret = procm_ipc_data->spawn.returns.pid;
                if (mt_cap_out) {
                        *mt_cap_out = ipc_get_msg_cap(ipc_msg, 0);
                }
        }
        ipc_destroy_msg(procm_ipc_struct, ipc_msg);
        return ret;
}
