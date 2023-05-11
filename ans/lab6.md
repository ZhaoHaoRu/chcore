# lab5

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

在`CardReset`中，首先通过判断USE_SDHOST是否被定义，确定是否是使用SD卡寄存器或者SD Host寄存器。如果没有定义USE_SDHOST，则使用SD卡寄存器。

如果`#ifndef USE_SDHOST`, `read32`函数读取`EMMC_CONTROL1`寄存器的值，将bit24设置为1，表示使能SD卡控制器；将bit2和bit0设置为0，表示禁止SD卡控制器时钟。然后通过`write32`函数将`EMMC_CONTROL1`寄存器写入设置后的值。之后等待EMMC_CONTROL1寄存器的bit31-bit24的值变为0，代表SD卡控制器已经重置好了。若在1秒内仍未重置好，则打印"Controller did not reset properly"，并返回-1。

接着通过`read32`函数读取`EMMC_STATUS`寄存器的值，判断SD卡是否已插入。若未插入则打印"no card inserted"，并返回-1。

然后将`EMMC_CONTROL2`寄存器清零，并通过`GetBaseClock`函数获取SD卡时钟，若未成功获取，则设置为`250000000`。之后设置SD卡时钟为400 kHz。

发送`CMD0`指令将SD卡置为空闲状态，再发送`CMD8`指令检查SD卡的电压范围和接口版本，若超时或发送失败，则返回`-1`。否则，判断是否支持高速模式，以及电压是否符合标准，根据结果设置变量    `m_card_supports_sdhc`和`m_card_supports_18v`，同时获取OCR和RCA等信息，对于SD卡的寄存器进行一系列初始化操作，最后返回0表示成功。

> 在驱动代码的初始化当中，设置时钟频率的意义是什么？为什么需要调用`TimeoutWait`进行等待?

设置时钟频率的意义是为了使SD卡能够正常工作。SD卡的时钟频率是SD卡和SD卡控制器之间进行数据传输的时钟频率，如果时钟频率设置不正确，SD卡和SD卡控制器之间的数据传输就会出错，导致SD卡无法正常工作。

在设置时钟频率后，需要等待一段时间，以确保时钟稳定。这是因为在设置时钟频率之后，时钟需要一定的时间来稳定到正确的速率。在这段等待时间内，使用TimeoutWait等待系统时钟稳定。TimeoutWait的作用是让代码等待一段时间，以确保系统时钟稳定，以便后续代码正确地执行。