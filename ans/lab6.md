# lab6

### 赵浩如 520021910352
## 思考题1
> circle中还提供了SDHost的代码。SD卡，EMMC和SDHost三者之间的关系是怎么样的？

External Mass Media Controller (EMMC)是闪存存储器控制器，它支持多种存储卡接口标准，包括SD卡接口标准。SD卡是一种常见的闪存存储卡，它有多种不同的规格和接口，其中包括SD卡接口标准。

SDhost是一个控制SD卡和MMC（MultiMediaCard）的主机控制器，它与处理器或其他主机设备连接，控制SD卡和MMC的读写操作。SDhost通常被集成在SoC（System-on-a-Chip）或外部芯片中，作为连接SD卡和主机设备之间的接口。

在使用SD卡时，SDhost与SD卡之间的通信是通过EMMC控制器进行的。EMMC控制器可以根据SD卡的不同规格和接口标准，控制SDhost与SD卡之间的通信。因此，EMMC可以看作是连接SD卡和SDhost之间的桥梁。

> 请详细描述ChCore是如何与SD卡进行交互的？即ChCore发出的指令是如何输送到SD卡上，又是如何得到SD卡的响应的。(提示: IO设备常使用MMIO的方式映射到内存空间当中)

在最开始，`sd server`使用`map_mmio`函数，将SD卡的寄存器映射到内存空间中。然后，使用一个循环调用`Initialize`，初始化SD卡的驱动程序。在初始化过程中，`sd server`会设置SD卡的时钟频率，并调用`TimeoutWait`函数，等待SD卡的初始化完成。初始化完成后，`sd server`会调用`sd_readblock`和`sd_writeblock`函数，读写SD卡的数据。在数据读写时，用户程序是通过IPC给SD卡发送请求来使用SD卡，`sd_dispatch`在解析IPC请求之后，根据请求类型的不同，调用`write`和`read`的处理函数(里面包含了和SD卡交互的逻辑), 将数据写入SD卡或从SD卡中读取数据，然后将处理结果返回给用户程序。

> 请简要介绍一下SD卡驱动的初始化流程。

在`Initialize`函数中，首先分配了一个指向TSCR结构体的指针，TSCR结构体是用于存储SD卡寄存器的结构体；接下来指定了SD卡中数据区的偏移量，然后调用了`CardInit`函数。

在`CardInit`函数中，调用了`CardReset`函数，对于SD卡重置成初始状态，以便下一步对于SD卡进行操作，这里如果失败的话，会进行一定次数的重试。

在`CardReset`函数中，首先判断`USE_SDHOST`。如果未定义 USE_SDHOST，则直接与硬件交互（使用寄存器），而如果定义了 USE_SDHOST，则使用其他接口（可能是驱动程序）与硬件交互。

接下来做了如下工作：

1. 禁用时钟，等待超时时间，并检查是否有有效的sd卡。

2. 清除控制寄存器，将时钟设置为低频，并启用 SD 时钟。

3. 屏蔽中断并重置。

4. 设置设备结构，向sd卡发送命令，并初始化变量。

如果过程中有任何失败，返回`-1`。


> 在驱动代码的初始化当中，设置时钟频率的意义是什么？为什么需要调用`TimeoutWait`进行等待?

设置时钟频率的意义是为了使SD卡能够正常工作。SD卡的时钟频率是SD卡和SD卡控制器之间进行数据传输的时钟频率，如果时钟频率设置不正确，SD卡和SD卡控制器之间的数据传输就会出错，导致SD卡无法正常工作。

在设置时钟频率后，需要等待一段时间，以确保时钟稳定。这是因为在设置时钟频率之后，时钟需要一定的时间来稳定到正确的速率。在这段等待时间内，使用TimeoutWait等待系统时钟稳定。TimeoutWait的作用是让代码等待一段时间，以确保系统时钟稳定，以便后续代码正确地执行。

## 思考题2
> 查阅资料了解 SD 卡是如何进行分区，又是如何识别分区对应的文件系统的？尝试设计方案为 ChCore 提供多分区的 SD 卡驱动支持，设计其解析与挂载流程。本题的设计部分请在实验报告中详细描述，如果有代码实现，可以编写对应的测试程序放入仓库中提交。

SD卡是通过分区表来实现分区的，分区表通常存储在SD卡的MBR（Master Boot Record）扇区或GPT（GUID Partition Table）扇区中，用于描述SD卡的分区信息，包括每个分区的起始扇区、结束扇区、分区类型等。在Linux系统中，可以使用fdisk、parted等工具来查看和管理SD卡的分区表信息。

当SD卡被插入计算机后，Linux系统会通过SD卡驱动程序读取SD卡的MBR或GPT扇区，并解析出SD卡的分区信息。对于每个分区，Linux系统会将其视为一个独立的逻辑设备，并尝试识别分区对应的文件系统类型。一旦文件系统类型被确定，Linux系统就可以将文件系统挂载到指定的挂载点，使其可以被访问和使用。

在`chcore`中，可以通过以下步骤来实现多分区的SD卡驱动支持：
1. 实现一个 SD 卡驱动程序：首先，实现一个 SD 卡驱动程序，它可以在 SD 卡中读取和写入数据。此驱动程序应支持与 SD 卡交互所需的标准命令，例如发送读取和写入命令以及处理错误条件。

1. 对 SD 卡进行分区：实现 SD 卡驱动程序后，将 SD 卡分成多个分区。

2. 实现一个分区表读取器：在对 SD 卡进行分区后，实现一个分区表读取器，可以从 SD 卡读取分区表。分区表包含有关磁盘上分区布局的信息，包括每个分区的开始和结束扇区。
分区表的具体设计如下：
    - 每个分区的起始位置和大小都可以通过分区表中的表项（partition entry）来描述。每个分区表项的大小为 16 个字节，其中包括了分区的起始地址、大小和类型等信息。分区类型字段用于表示分区使用的文件系统类型，如 FAT32、NTFS 等。
    - 个 MBR 分区表中的分区类型字段的取值可以是 0x00 到 0xFF 的任意值，我们可以提前规定一些常用的类型：
    ```shell
    0x01: FAT12 分区
    0x04: FAT16 分区（小于 32 MB）
    0x05: 扩展分区（Extended Partition）
    0x06: FAT16 分区（大于等于 32 MB）
    0x07: NTFS 分区
    0x0B: FAT32 分区
    0x0C: FAT32 分区（使用 INT 13h 扩展）
    0x0E: FAT16 分区（使用 INT 13h 扩展）
    0x82: Linux Swap 分区
    0x83: Linux 分区
    0x8E: Linux LVM 分区
    ```

3. 挂载分区：一旦有了分区表信息，就可以在 ChCore 中将每个分区挂载为一个单独的文件系统。这可以通过为每个分区创建一个挂载点然后使用挂载系统调用将分区附加到该挂载点来完成。

    在 ChCore 中，挂载系统调用可以通过实现一个名为“mount”的系统调用来完成。该系统调用可以接受两个参数：源路径和目标路径。其中，源路径指定要挂载的文件系统的位置，可以是设备文件、文件或目录，而目标路径指定挂载点的位置，必须是一个目录。当 mount 系统调用成功返回时，指定的文件系统就会被挂载到指定的挂载点上。
    
    在具体实现时，可以通过读取分区表信息，获取每个分区的起始扇区号和扇区数，并将其作为设备文件传递给 mount 系统调用。在 mount 系统调用成功返回后，可以使用文件系统相关的系统调用，如 open、read、write 等来访问该分区中的文件。

4. 实现文件系统驱动程序：最后，需要为已安装的每个分区实施文件系统驱动程序，以允许 ChCore 使用适当的文件系统（例如 FAT32 或 ext4）将数据读取和写入各个分区。

    在具体实现时，可以将文件系统驱动程序编写为动态链接库（shared library），并使用动态链接库加载器（dynamic linker）将其加载到 ChCore 中，以便在运行时动态加载和卸载文件系统驱动程序
## 实现思路
### part1

#### emmc.c
`emmc.c`中主要实现了SD卡的控制器逻辑，实现时思路参考了`circle`中的`sdhost.c`。`sd.c`中主要实现了SD卡的驱动逻辑，实现时思路参考了`circle`中的`emmc.cpp`的逻辑，主要代码如下：

`DoRead`首先判断目前`sd`卡是否是在一个正确的状态，如果不是则返回-1，否则调用`DoDataCommand`函数进行读取，如果读取失败则返回-1，否则返回读取的字节数。

```c
nt DoRead(u8 * buf, size_t buf_size, u32 block_no)
{
	/* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
	if (EnsureDataMode() != 0) {
		printf("[DEBUG] EnsureDataMode() != 0\n");
		return -1;
	}
	if (DoDataCommand(0, buf, buf_size, block_no) < 0) {
		printf("[DEBUG] DoDataCommand(0, buf, buf_size, block_no) < 0\n");
		return -1;
	}
	return buf_size;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```

`DoWrite`和`DoRead`的逻辑很像，首先判断目前`sd`卡是否是在一个正确的状态，如果不是则返回-1，否则调用`DoDataCommand`函数进行写入，如果写入失败则返回-1，否则返回0。
```c
int DoWrite(u8 * buf, size_t buf_size, u32 block_no)
{
	/* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
	if (EnsureDataMode() != 0) {
		printf("[DEBUG] EnsureDataMode() != 0\n");
		return -1;
	}
	
	int ret = DoDataCommand(1, buf, buf_size, block_no);
	if (ret < 0) {
		printf("[DEBUG] DoDataCommand(1, buf, buf_size, block_no) < 0\n");
		return -1;
	}
	return ret;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```

`sd_read`函数负责了读取指定的字节数到`pbuffer`，这里首先判断了到达的偏移量是否是块的整数倍，如果不是则返回-1，否则计算出块号，调用`DoRead`函数进行读取，如果读取失败则返回-1，否则返回读取的字节数。
```c
int sd_Read(void *pBuffer, size_t nCount)
{
	/* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
	if (m_ullOffset % SD_BLOCK_SIZE != 0) {
		return -1;
	}
	u32 nBlock = m_ullOffset / SD_BLOCK_SIZE;
	if (DoRead(pBuffer, nCount, nBlock) != nCount) {
		printf("[DEBUG] sd_Read: DoRead failed\n");
		return -1;
	}

	return nCount;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```

`sd_write`函数负责了将`pbuffer`中的指定字节数写入到SD卡中，这里首先判断了到达的偏移量是否是块的整数倍，如果不是则返回-1，否则计算出块号，调用`DoWrite`函数进行写入，如果写入失败则返回-1，否则返回写入的0（在写入成功的情况下`DoWrite`的返回值是0。
```c
int sd_Write(const void *pBuffer, size_t nCount)
{
	/* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
	if (m_ullOffset % SD_BLOCK_SIZE != 0) {
		printf("[DEBUG] sd_Write: m_ullOffset %% SD_BLOCK_SIZE != 0\n");
		return -1;
	}

	u32 nBlock = m_ullOffset / SD_BLOCK_SIZE;
	int ret = DoWrite(pBuffer, nCount, nBlock);
	if (ret < 0) {
		printf("[DEBUG] sd_Write: DoWrite failed, the ret: %d\n", ret);
		return -1;
	}
	return ret;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```

`Seek`负责将读写头移动到指定的偏移量，为下一步读写操作做准备，这里直接将`m_ullOffset`设置为`ullOffset`，并返回`m_ullOffset`。
```c
u64 Seek(u64 ullOffset)
{
	/* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
	m_ullOffset = ullOffset;
	return m_ullOffset;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```

#### sd_server.c
`sd_server`是对于`emmc.c`中对于`sd`卡操作的基本封装，对外提供了`sdcard_readblock`和`sdcard_writeblock`两个接口，实现了对于指定块号的单个数据块的读写

`sdcard_readblock`首先根据`lba`(logical block address)计算出偏移量，然后调用`sd_Read`函数进行读取，如果读取失败则返回-1，否则返回0。
```c
static int sdcard_readblock(int lba, char *buffer)
{
	/* LAB 6 TODO BEGIN */
	/* BLANK BEGIN */
	u64 cur_address = Seek(lba * BLOCK_SIZE);
	
	int ret = sd_Read(buffer, BLOCK_SIZE);
	if (ret < 0) {
		return -1;
	}

	return 0;
	/* BLANK END */
	/* LAB 6 TODO END */
}
```

`sdcard_writeblock`和`sdcard_readblock`的逻辑很像，首先根据`lba`(logical block address)计算出偏移量，然后调用`sd_Write`函数进行写入，如果写入失败则返回-1，否则返回0。
```c
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
```

### part2
#### 架构
混合了`inode based`和`FAT`文件系统的设计，并从代码实现难易程度上进行了简化，主要设计思路如下：
1. 根目录：根目录采用最开头的四个block进行存储(这里为了简化实现，没有考虑动态伸缩)，目录存储的格式为`[文件名][文件类型][文件大小][文件起始block号]`
2. bitmap: bitmap采用了根目录之后一个block进行存储，每个bit代表一个block，0代表空闲，1代表已经被占用
3. 文件: 文件采用中间的block进行存储，对于不超过`508byte`的文件，不需要进行额外的处理，对于超过`508`个byte的文件，采用了类似链表的方式进行处理，第一个`block`的最后4个`byte`存储了下一个`block`的`block号`，最后一个`block`的最后4个`byte`存储了`-1`或者为空，表示结束

#### 算法
1. `naive_fs_access`：遍历根目录，找到对应的文件，如果找到则返回0，否则返回-1
2. `naive_fs_creat`：遍历根目录，找到对应的文件，如果找到则返回-1，否则在根目录中创建一个新的文件，在bitmap中找到一个空闲块，设为已占用，将文件起始block号写入根目录，返回0
3. `naive_fs_unlink`：遍历根目录，找到对应的文件，如果找到则将bitmap中对应的block设为未占用(如果一个文件占用了多个block，需要将这些block都释放），返回0，否则返回-1
4. `naive_fs_pread`：遍历根目录，找到文件对应的起始block（如果没有找到返回-1），如果偏移量在第一个 block内，从第一个block中对应的偏移量开始向后读取，如果偏移量在第一个block之后，从第一个block中的最后4个byte中读取下一个block的block号，然后从下一个block中对应的偏移量开始向后读取（第二个block以此类推），如果读取的字节数超过了文件的大小，返回读取的字节数
5. `naive_fs_pwrite`: 遍历根目录，找到文件对应的起始block（如果没有找到返回-1）, 首先找到偏移量对应的位置（这一步操作和`naive_fs_pread`类似, 如果偏移量大于了当前文件大小，返回-1），然后从这个位置开始写入，如果写入的字节数超过了文件的block的总大小，需要从`bitmap`中申请新的空闲块，如果申请失败，写操作结束，最后返回写入的字节数

#### version1
我先实现了一版比较简易的文件系统，观察测试用例发现文件大小都在1个block以内，因此我们可以先限制文件大小为`512bytes`，验证一下文件系统的正确性，如果正确的话，再进行改进。

```c
void initialize() {
    if (start) {
        start = 0;
        char bitmap[BLOCK_SIZE] = {'\0'};
        for (int i = 0; i < TEST_BLOCK_NUM; ++i) {
            int ret = sd_bwrite(1, bitmap);
            if (ret == -1) {
                printf("write failed\n");
            }
        }
    }
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


int find_file_by_name (const char *name) {
    char buffer[BLOCK_SIZE] = {'\0'};
    int ret = sd_bread(0, buffer);
    if (ret == -1) {
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
                int block_id = 0;
                ++i;
                while (i < BLOCK_SIZE && buffer[i] != '/') {
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
    initialize();
    int ret = find_file_by_name(name);
    if (ret >= 0) {
        return 0;
    }
    return -1;
    /* BLANK END */
    /* LAB 6 TODO END */
}

int naive_fs_creat(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int ret = find_file_by_name(name);
    if (ret >= 0) {
        return -1;
    }
    // check the bitmap
    char bitmap[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(1, bitmap);
    if (ret == -1) {
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
    if (block_id == BLOCK_SIZE) {
        return -1;
    }

    int i = 0;
    // write the bitmap
    ret = sd_bwrite(1, bitmap);
    if (ret == -1) {
        return -1;
    }

    // write the directory
    char dir[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(0, dir);
    if (ret == -1) {
        return -1;
    }
    i = 0;
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
        return -1;
    }

    dir[i] = '/';
    i += 1;
    if (i == BLOCK_SIZE) {
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
        return -1;
    }

    dir[i] = '/';
    i += 1;

    if (i == BLOCK_SIZE) {
        return -1;
    }

    ret = sd_bwrite(0, dir);
    if (ret == -1) {
        return -1;
    }

    return 0;
    /* BLANK END */
    /* LAB 6 TODO END */
}

int naive_fs_pread(const char *name, int offset, int size, char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int block_id = find_file_by_name(name);
    if (block_id == -1) {
        return -1;
    }
    char buf[BLOCK_SIZE] = {'\0'};
    int ret = sd_bread(block_id, buf);
    if (ret == -1) {
        return -1;
    }
    int read_size = BLOCK_SIZE - offset <  size ? BLOCK_SIZE - offset : size;
    memcpy(buffer, buf + offset, read_size);
    return read_size;
    /* BLANK END */
    /* LAB 6 TODO END */
}

int naive_fs_pwrite(const char *name, int offset, int size, const char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int block_id = find_file_by_name(name);
    if (block_id == -1) {
        return -1;
    }
    char buf[BLOCK_SIZE] = {'\0'};
    int ret = sd_bread(ret, buf);
    if (ret == -1) {
        return -1;
    }
    int write_size = BLOCK_SIZE - offset <  size ? BLOCK_SIZE - offset : size;
    memcpy(buf + offset, buffer, write_size);
    ret = sd_bwrite(block_id, buf);
    if (ret == -1) {
        return -1;
    }
    return write_size;
    /* BLANK END */
    /* LAB 6 TODO END */
}

int naive_fs_unlink(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int ret = find_file_by_name(name);
    if (ret == -1) {
        return -1;
    }

    // delete the file in bitmap 
    char bitmap[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(1, bitmap);
    if (ret == -1) {
        return -1;
    }
    bitmap[ret] = '0';
    ret = sd_bwrite(1, bitmap);
    if (ret == -1) {
        return -1;
    }

    // delete the file in directory
    char dir[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(0, dir);
    if (ret == -1) {
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
        return -1;
    }

    return 0;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```
这里还额外实现了一个`num2str`函数，用于将数字转换为字符串，方便写入到目录中;因为不论是`creat`,`read`,`write`还是`unlink`都需要遍历根目录，因此我将遍历根目录的代码抽取出来，放到了`find_file_by_name`函数中，这样可以减少代码的重复；此外，因为前面测试的数据可能已经写入到`sd`卡中，因此在每次测试之前，需要将`sd`卡中的数据清空，所以我在`initialize`函数中，将`sd`卡中的数据清空。

#### version2
在实现了第一版的基础上实现第二版，最大的改变就是要对于多block文件进行处理，这里我采用了类似链表的方式进行处理，第一个block的最后4个byte存储了下一个block的block号，最后一个block的最后4个byte存储了`-1`或者为空，表示结束，具体实现如下，`creat`, `access`的逻辑都没有改变：
```c
void initialize() {
    ...
}

void num2str(int val, char *result) {
    ...
}

int alloc_new_block() {
    char bitmap[BLOCK_SIZE] = {'\0'};
    int ret = sd_bread(1, bitmap);
    if (ret == -1) {
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
    if (block_id == BLOCK_SIZE) {
        return -1;
    }
    ret = sd_bwrite(1, bitmap);
    if (ret == -1) {
        return -1;
    }
    return block_id;
}

int find_file_by_name (const char *name) {
    ...
}

int naive_fs_pread(const char *name, int offset, int size, char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int block_id = find_file_by_name(name);
    if (block_id == -1) {
        return -1;
    }
    char buf[BLOCK_SIZE] = {'\0'};
    int bias = 0;
    int j = 0, read_size = 0;
    while (bias < size) {
        int ret = sd_bread(block_id, buf);
        if (ret == -1) {
            return -1;
        }
        if (offset < BLOCK_SIZE) {
            read_size = BLOCK_SIZE - 4 - offset <  size - bias ? BLOCK_SIZE - 4 - offset : size - bias;
            memcpy(buffer + bias, buf + offset, read_size);
            bias += read_size;
            offset = 0;
        } else {
            offset -= BLOCK_SIZE;
        }
        int next_block_id = 0;
        for (int i = BLOCK_SIZE - 4; i < BLOCK_SIZE; ++i) {
            next_block_id = next_block_id * 10 + buf[i] - '0';
        }
        if (next_block_id == -1) {
            break;
        }
        block_id = next_block_id;
        j += BLOCK_SIZE;
    }
    return bias;
    /* BLANK END */
    /* LAB 6 TODO END */
}

int naive_fs_pwrite(const char *name, int offset, int size, const char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int block_id = find_file_by_name(name);
    if (block_id == -1) {
        return -1;
    }
    char buf[BLOCK_SIZE] = {'\0'};
    int bias = 0;
    while (bias < size) {
        int ret = sd_bread(block_id, buf);
        if (ret == -1) {
            return -1;
        }
        if (offset < BLOCK_SIZE) {
            int write_size = BLOCK_SIZE - 4 - offset <  size - bias ? BLOCK_SIZE - 4 - offset : size - bias;
            memcpy(buf + offset, buffer + bias, write_size);
            bias += write_size;
            offset = 0;
        } else {
            offset -= BLOCK_SIZE;
        }
        int next_block_id = 0;
        for (int i = BLOCK_SIZE - 4; i < BLOCK_SIZE; ++i) {
            next_block_id = next_block_id * 10 + buf[i] - '0';
        }
        if (next_block_id == -1) {
            next_block_id = alloc_new_block();
            if (next_block_id == -1) {
                return -1;
            }
            char next_block_id_str[10] = {'\0'};
            num2str(next_block_id, next_block_id_str);
            memcpy(buf + BLOCK_SIZE - 4, next_block_id_str, 4);
        }
        ret = sd_bwrite(block_id, buf);
        if (ret == -1) {
            return -1;
        }
        block_id = next_block_id;
    }
    return write_size;
    /* BLANK END */
    /* LAB 6 TODO END */
}


int naive_fs_unlink(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    initialize();
    int block_id = find_file_by_name(name);
    if (block_id == -1) {
        return -1;
    }

    // delete the file in bitmap 
    char bitmap[BLOCK_SIZE] = {'\0'};
    int ret = sd_bread(1, bitmap);
    if (ret == -1) {
        return -1;
    }
    while (true) {
        bitmap[block_id] = '0';
        char buf[BLOCK_SIZE] = {'\0'};
        ret = sd_bread(block_id, buf);
        if (ret == -1) {
            return -1;
        }
        int next_block_id = 0;
        for (int i = BLOCK_SIZE - 4; i < BLOCK_SIZE; ++i) {
            next_block_id = next_block_id * 10 + buf[i] - '0';
        }
        if (next_block_id == -1) {
            break;
        }
        block_id = next_block_id;
    }
    ret = sd_bwrite(1, bitmap);
        if (ret == -1) {
            return -1;
    }

    // delete the file in directory
    char dir[BLOCK_SIZE] = {'\0'};
    ret = sd_bread(0, dir);
    if (ret == -1) {
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
        return -1;
    }

    return 0;
    /* BLANK END */
    /* LAB 6 TODO END */
}
```
这里的`pread`和`pwrite`的逻辑和第一版的逻辑类似，只是多了对于多block文件的处理，`unlink`的逻辑也类似，只是需要将多个block都释放掉。

## 对于chcore的建议
1. `chcore`的`sd`的测试，`register_ipc_client`的时候需要等待一段时间，`sd_server`才能正常启动，这时候会出现类似卡住的现象，需要敲击回车，但是这个时候`shell`会以为运行了多个测试，从而导致测试失败，只有将`run_cmd`中`chcore_procm_spawn`后加入一个`while (true) {}`才能得到一个正确的运行结果

2. chcore可以增加关于网络栈相关的内容，平时对于网络的接触比较少，这次实验中也没有涉及到网络相关的内容，但是在实际的操作系统中，网络相关的内容是非常重要的，因此可以增加关于网络栈的内容