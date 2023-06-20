# lab1

### 赵浩如 520021910352

## 思考题1

1. 通过 `mrs x8, mpidr_el1` 和 `and x8, x8, #0xFF` 从系统寄存器 `mpidr_el1` 中获取当前核的 ID，并将其存储在寄存器 `x8` 中。
2. 使用 `cbz x8, primary` 指令检查 `x8` 是否为零，即当前核是否是主核（核 ID 为 0）。如果是主核，跳转到 `primary` 标签处执行主核的初始化流程；如果不是主核，继续执行后续步骤。
3. 在主核之外的其他核会进入 `wait_until_smp_enabled` 循环，在该循环中会等待主核设置一个标志位，表示其他核可以开始执行初始化流程。
4. 在循环中，使用核 ID 计算出在 `secondary_boot_flag` 数组中的索引位置，检查相应的标志位。如果标志位为零，表示还不能开始执行初始化流程，继续等待；如果标志位非零，表示可以开始执行初始化流程，跳出循环。
5. 在跳出循环后，通过 `mov x0, x8` 将当前核的 ID 存储在寄存器 `x0` 中，然后调用 `secondary_init_c` 函数执行其他核的初始化流程。
6. 主核会继续执行后续的指令，包括进入 `primary` 标签处执行主核的初始化流程。

## 练习题2

添加的代码为

```
mrs x9, CurrentEL
```

这行代码的作用为移动`CurrentEL`中的值到`x9`中，下面需要用`x9`寄存器中的值check当前的exception level.

debug运行效果:
![pic2](asset/2.png)
![pic](asset/1.png)
当前还是在`EL3`,所以不会跳转

## 练习题3

添加的代码为

```
    adr x9, .Ltarget
    msr elr_el3, x9
    mov x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
    msr spsr_el3, x9
```

这段代码的主要工作是设置`EL3`的`exception link register`, 将`ret`位置的`label`设置到这里, 为`eret`做准备，设置`EL3`的状态寄存器`SPSR`, 包括`debug`,`error`, `interrupt`和`fast interrupt`

## 思考题4

C作为一种高级语言，它调用过程机制中的关键特性在于使用了栈数据结构提供的后进先出的内存管理原则，在函数调用的过程中，伴随着参数压栈、返回地址压栈等行为，所以在C函数运行之前需要有栈，如果没有栈的话，C函数可能会出现无法正常传比较多的参数、无法正确返回调用者等情况

## 思考题5

将`.bss`初始化为0是操作系统的任务，如果这里没有初始化的话，C函数如果使用了未初始化的全局变量或者是静态变量，会导致未知的错误

## 练习题6

添加的代码为

```C
void uart_send_string(char *str)
{
        /* LAB 1 TODO 3 BEGIN */
        int i = 0;
        while (*(str + i) != '\0') {
                early_uart_send(*(str + i));
                ++i;
        }
        /* LAB 1 TODO 3 END */
}
```

这段代码的运行效果为
![pic3](asset/3.png)

## 练习题7

添加的代码为

```asm
orr     x8, x8, 
```

这个命令的主要功能是将`sctlr_el1`的值写入`x8`,  SCTLR_EL1 寄存器的 M（MMU）位设置为 1，表示启用 MMU
