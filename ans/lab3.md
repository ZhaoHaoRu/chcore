# lab1

### 赵浩如 520021910352

## 思考题1

过程如下；

1. 首先运行 `start.S` 中的 `_start` 函数，调用 `init_c`, 完成包括为 `kernel image` 清理 `bss`, 初始化 `UART`, 初始化 `boot page table`, 启用 `MMU` 等一系列工作，最后调用 `start_kernel` 跳转到高地址，进而跳转到内核的 `main`函数
2. 在 `main` 函数中调用 `uart_init` 再完成 `uart`的初始化，调用 `mm_init` 完成内存管理的初始化，调用 `arch_interrupt_init()`完成异常向量表的初始化
3. 调用函数 `create_root_thread `, 创建第一个 `user thread`，具体的工作是：

   a. 创建 `root_cap_group`

   b. 在 `root_cap_group` 中创建 `root_thread`

   c. 使用 `switch_to_thread` 将当前线程切换为 `root_thread`
4. 使用 `switch_context`获取设置成栈指针寄存器的上下文指针。
5. 调用 `eret_to_thread` 完成上下文的切换，完成了内核模式和用户模式的切换，开始在用户模式下运行代码
7.
