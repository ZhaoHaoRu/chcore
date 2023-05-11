#include <chcore/internal/raw_syscall.h>
#include <chcore/internal/server_caps.h>
#include <chcore/sd.h>
#include <chcore/ipc.h>
#include <chcore/memory.h>
#include <chcore/capability.h>
#include <chcore/assert.h>
#include "emmc.h"

int map_mmio(unsigned long long pa_base, unsigned long long size){
	int mmio_pmo, ret;

	/* Map GPIO registers */
	mmio_pmo = __chcore_sys_create_device_pmo(pa_base, size);
	if (mmio_pmo < 0) {
		printf("create mmio_pmo failed,  r=%d\n", mmio_pmo);
		return -1;
	}

	printf("the address is %llx\n", mmio_pmo);
	ret = __chcore_sys_map_pmo(SELF_CAP, mmio_pmo, pa_base, VM_READ | VM_WRITE, size);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

// read a block from sd card which is at lba
static int sdcard_readblock(int lba, char *buffer)
{
	/* LAB 6 TODO BEGIN */
	/* BLANK BEGIN */
	// printf("the virtual address is %llx\n", vir_address);
	u64 cur_address = Seek(lba * BLOCK_SIZE);
	
	int ret = sd_Read(buffer, BLOCK_SIZE);
	// printf("sdcard_readblock: %s\n", buffer);
	// printf("[DEBUG] the read ret: %d\n", ret);
	if (ret < 0) {
		return -1;
	}

	return 0;
	/* BLANK END */
	/* LAB 6 TODO END */
	return -1;
}

static int sdcard_writeblock(int lba, const char *buffer)
{
	/* LAB 6 TODO BEGIN */
	/* BLANK BEGIN */
	u64 cur_address = Seek(lba * BLOCK_SIZE);
	if (cur_address == -1) {
		printf("seek failed\n");
		return -1;
	}
		
	int ret = sd_Write(buffer, BLOCK_SIZE);
	if (ret == -1) {
		printf("write failed\n");
	}

	return ret;
	/* BLANK END */
	/* LAB 6 TODO END */
	return -1;
}

void sd_dispatch(ipc_msg_t * ipc_msg, u64 client_badge)
{
	struct sd_ipc_data *msg;
	int ret = 0;

	msg = (struct sd_ipc_data *)ipc_get_msg_data(ipc_msg);
	switch (msg->request) {
	case SD_IPC_REQ_READ:
		ret = sdcard_readblock(msg->lba, msg->data);
		break;
	case SD_IPC_REQ_WRITE:
		ret = sdcard_writeblock(msg->lba, msg->data);
		break;
	default:
		printf("strange sd ipc request %d\n", msg->request);
		chcore_assert(0); /* fail stop */
	}
	ipc_return(ipc_msg, ret);
}

int main(void)
{
	int ret;
	printf("[DEBUG] sd_server\n");

	map_mmio((unsigned long long)ARM_EMMC_BASE, 0x100);
	while (!Initialize()) ;

	ret = ipc_register_server(sd_dispatch);
	printf("[SD Driver] register server value = %d\n", ret);

	while(1) {
		__chcore_sys_yield();
	}

	return 0;
}
