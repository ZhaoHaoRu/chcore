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

## 思考题8
在`irq_entry.S`中定义了异常向量表`el1_vector`，并用`EXPORT`导出使得其他文件中可以访问到,异常向量表中每个条目都是`exception_entry`, 这是在`irq_entry.S`的开头定义的宏，主要作用是首先保证$2^7$对齐，然后用相对跳转的方式跳转到`label`指示的位置。

对于`syscall`,根据异常向量表的指示，首先会跳转到`sync_el0_64`对应的位置，首先使用`exception_enter`完成上下文的保存，然后将异常状态表征寄存器`esr_el1`的内容赋给`x25`，比较其`[31:26]`位对应的位置是否和`0b010101`相等，如果相等，说明该异常对应的是AArch64状态下的SVC指令执行，从而跳转到`el0_syscall`的位置进行`syscall`的处理。

在`irq_entry.S`的开头，通过`.extern`引用了`syscall_table`，这是一个由`syscall`处理函数指针构成的数组，