# 调试操作系统的启动过程

## 前言

这是我初次接触linux系统，所以实际在这个实验之外花费的时间要多于实验本身。尤其是几次把系统搞得无法启动，不过在解决这些问题之后，我对于linux系统的使用就已经较为熟悉了。

## 软件准备

在网上查找了一些资料，了解了实验的基本原理：在linux系统上使用qemu虚拟机去模拟另一个内核启动，并使用gdb进行断点调试。

所以第一步就要下载或更新这些必要软件到最新版本，使用apt包管理器很容易就完成了。（但由于对包管理器不熟悉，在这一步花了不少时间）

然后要下载一个轻量的linux内核，这里选择linux-3.18.6，使用wget下载

## 编译linux

1.执行make menuconfig

2.进入kernel hacking&gt;Compile-time checks and compiler options&gt;Compile the kernel with debug info,并保存

3.执行make进行编译


## 结果分析

整个linux的启动过程都是main.c中的start_kernel函数，

> set_task_stack_end_magic(&amp;init_task);

这是一个关键事件，因为它初始化栈底信息，为启动0号进程做准备。主体动作都在init_task中进行，通过静态方式分配内核栈

> rest_init();

这也是个关键事件，它启动了1号进程，1号进程主要由三部分构成：

*   初始化：
```
    static struct task_struct *copy_process(unsigned long clone_flags,
                    unsigned long stack_start,
                    unsigned long stack_size,
                    int __user *child_tidptr,
                    struct pid *pid,
                    int trace)
{
   .... 


    retval = security_task_create(clone_flags);
    if (retval)
        goto fork_out;

    retval = -ENOMEM;

    p = dup_task_struct(current);

    retval = copy_creds(p, clone_flags);

    if (retval)
        goto bad_fork_cleanup_namespaces;


    retval = copy_thread(clone_flags, stack_start, stack_size, p);
     ... ... 


        __this_cpu_inc(process_counts);

  ... .... 

}
```
这里的start_stack对应的值： 
```
(gdb) s
copy_thread(clone_flags=8389876, sp=3245760928, arg=0, p=0x78500000) at arch/x86/kernel/process_32.c.134
```
查看kernel_init的地址：
```
(gdb) disassemble kernel_mit Duap of assembler code for function kernel_mit:
0xc17661a0 <+0>: push %ebp
0xc17661a1 <+1>: mov %esp,%ebp
0xc17661a3 <+3>: sub $Oxc,’esp
0xc17661a6 <+6>: cafl 0xc1a4d524 <kernel._init_freeabJe>
```

0号进程会复制一份新的自己作为新进程,接着将kernel_init的入口地址设置到新进程中，接着设置pid的值利用#define **this_cpu_inc(pcp)     **this_cpu_add(pcp, 1)取得新的PID保证不重复

*   调度过程

在kernel_init处打上断点，然后查看其堆栈信息： 
```
Breakpoint 2, __schedule () at kernel/sched/core.c:2771
2771 {
(gdb) bt
#0 __schedule () at kernel/sched/core.c:2771
#1 0xc176860e in schedule () at kernel/sched/core.c:2870
#2 OxclO5d6cc in kthreadd (unused=<value optimized out>) at kernel/kthread.c:498
#3 0xc176b601 in ?? () at arch/x86/kernel/entry_32.S:311
#4 0xc105d580 in ?? () at kernel/kthread.c:195
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
```



```
asmlinkage __visible sched schedule(void)
    {
       task_struct *tsk = current;
        sched_submit_work(tsk);
        __schedule();
       }
```

可见__schedule（）进行了各种调度工作

*   执行

```
    static int __ref kernel_init(void *unused)
{
   ... ... 
    if (ramdisk_execute_command) {
        ret = run_init_process(ramdisk_execute_command);
        if (!ret)
            return 0;
            pr_err("Failed to execute %s (error %d)\n",
               ramdisk_execute_command, ret);
    }
    ... ... 
}
```
查看ramdisk_execute_command的值：
```
(gdb) p ramdisk_execute_command 
$1 = Oxc18ecc79 “/init”
```
从这里可以很清楚看到它会执行/init，而init就是1号进程。因此我们可以看到0号进程会调用execute()命令将init作为一个可执行程序运行起来。



