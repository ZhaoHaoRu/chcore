# lab4

### 赵浩如 520021910352

## 思考题1
> 阅读汇编代码`kernel/arch/aarch64/boot/raspi3/init/start.S`。说明ChCore是如何选定主CPU，并阻塞其他其他CPU的执行的。

寄存器`mpidr_el1`使软件能够确定它在哪个核上执行，在一个簇中或在一个有多个簇的系统中，该寄存器可以确定它在在哪个簇中的哪个核上执行。在`_start`函数的开头，有如下语句：

```as
mrs	x8, mpidr_el1
and	x8, x8,	#0xFF
cbz	x8, primary
```

这段指令的逻辑是将`mpidr_el1`中的值移动到`x8`中，只保留最低的8位，其他清空，然后判断`x8`是否为0，为0时跳转到`primary`对应的位置执行。由此可以判断Chcore选定主CPU的依据是主CPU的低8位为0（0号CPU），这里的逻辑保证了只有主CPU会跳转到`primary`执行`init_c`中相关的初始化逻辑，其他CPU会`wait_until_smp_enabled`处不断循环等待，直到主CPU完成了`MMU`,页表初始化等操作系统引导工作后设置了`secondary_boot_flag`

## 思考题2

> 阅读汇编代码`kernel/arch/aarch64/boot/raspi3/init/start.S, init_c.c`以及`kernel/arch/aarch64/main.c`，解释用于阻塞其他CPU核心的`secondary_boot_flag`是物理地址还是虚拟地址？是如何传入函数`enable_smp_cores`中，又该如何赋值的（考虑虚拟地址/物理地址）？

最开始是物理地址，在`init_c.c`中定义并初始化为`{NOT_BSS, 0, 0, ...}`, 在`init_c`中通过`start_kernel`函数，将`secondary_boot_flag`放置在`x0`寄存器中，然后通过`bl main`调用`main`函数中，从而将`secondary_boot_flag`作为`main`的参数；在`main`中，以`secondary_boot_flag`作为参数调用`enable_smp_cores`,然后通过`phys_to_virt`转换为虚拟地址完成赋值。

## 思考题5
> 在`el0_syscall`调用`lock_kernel`时，在栈上保存了寄存器的值。这是为了避免调用`lock_kernel`时修改这些寄存器。在`unlock_kernel`时，是否需要将寄存器的值保存到栈中，试分析其原因。

不需要，此时已经执行完了`syscall`的处理函数，下面要恢复之前保存的用户态上下文，目前寄存器的值是"将亡值"，不会再次使用

## 思考题6
> 为何`idle_threads`不会加入到等待队列中？请分析其原因？

因为`idle_threads`的优先级是最低的，它只有在就绪队列是为空时才能调用；如果将`idle_threads`放入等待队列中，就会出现还有其他线程在等待时`idle_threads`也被调用的情况，相当于时间片被浪费了

## 思考题8
> 如果异常是从内核态捕获的，CPU核心不会在`kernel/arch/aarch64/irq/irq_entry.c`的`handle_irq`中获得大内核锁。但是，有一种特殊情况，即如果空闲线程（以内核态运行）中捕获了错误，则CPU核心还应该获取大内核锁。否则，内核可能会被永远阻塞。请思考一下原因。

空闲线程运行时，没有获取大内核锁，但是在异常处理返回调用`eret_to_thread`时会放锁，这时候会出现没有拿锁就放锁的情况，可能会导致`lock->owner > lock->next`的出现从而引起死锁。 
