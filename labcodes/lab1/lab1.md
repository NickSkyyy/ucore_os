# os lab1

---

> 姓名：董佳霖，戚晓睿，孙一丁
>
> 日期：2020.10.07

## 1 Exercise 1

+ 执行make，产生目录bin和obj
+ 目标文件：依赖文件
+ 命令以tab作为开头，$(x)代表变量x
+ call函数调用，$(i)代表函数传递的第i个参数
+ $^：所有依赖文件
+ $@：目标
+ $<：第一个依赖文件

### 1.1 ucore.img

1. 产生代码位于Makefile Line178-186，UCOREIMG目标文件有kernel和两个依赖文件。

`call addprefix`, 前缀, 文件列表（SLASH表示正斜杠）

`@dd`：用指定大小的块拷贝一个文件，并进行指定的转换

> `if=name`，input file 文件名
>
> `of=name`，output file 文件名
>
> `count=num`，仅拷贝num个数的块
>
> `seek=num`，从输出文件开头跳过num个数的块后再开始复制
>
> `conv=conversion`，用指定的参数进行文件转换
>
> `notrunc`，不截短输出文件
>
> 详细内容：<https://baike.baidu.com/item/dd命令>

2. 产生bootblock代码位于Line156-168，配置和产生kernel代码位于Line122-151. 

> 依赖文件中`|`的解释：<https://blog.csdn.net/sdu611zhen/article/details/53011253>

bootblock有个三个依赖文件，分别是sign、bootasm.o和bootmain.o. 对应gcc代码如下：

```
$ gcc -Iboot/ -march=i686 -fno-builtin -fno-PIC -Wall -ggdb -m32 -gstabs -nostdinc -fno-stack-protector -Ilibs/ -Os -nostdinc -c boot/bootasm.S -o obj/boot/bootasm.o
```

> `march`，指定进行优化的型号，此处是i686
>
> `fno-builtin`， 不使用c语言的内建函数（函数重名时使用）
>
> `fno-PIC`， 不生成与位置无关的代码(position independent code)
>
> `Wall`，编译后显示所有警告
>
> `ggdb`， 为GDB生成更为丰富的调试信息
>
> `gstabs`， 以stabs格式生成调试信息，但不包括上一条的GDB调试信息
>
> `nostdinc`，查找头文件时，不在标准系统目录下查找

```
$ ld -m elf_i386 -nostdlib -N -e start -Ttext 0x7C00 obj/boot/bootasm.o obj/boot/bootblock.o -o obj/bootblock.o
```

`@ld`：GNU的链接器，将目标文件链接为可执行文件

> `m`，类march操作，模拟i386的链接器
>
> `nostdlib`，不使用标准库
> 
> `N`，设置全读写权限
>
> `e`，指定程序的入口符号
>
> `Ttext`，指定代码段的开始位置

3. kernel的依赖文件是一个集合KOBJS，而文件具体内容由KSRCDIR指定。内容基本同上。

#### 1.2 主引导扇区

从工具文件sign.c寻找对应的要求，以ucore.img为例。扇区总大小为512字节，obj/bootblock.out占据496字节，不得超过510字节。其中，510字节为0x55，511字节为0xAA.

## 2 Exercise 2 & 3

### ~/lab1  指令： make

    \+ cc kern/init/init.c
    kern/init/init.c:95:1: warning: ‘lab1_switch_test’ defined but not used [-Wunused-function]  
    lab1_switch_test(void) {
    ^
    \+ cc kern/libs/stdio.c
    \+ cc kern/libs/readline.c
    \+ cc kern/debug/panic.c
    kern/debug/panic.c: In function ‘__panic’:
    kern/debug/panic.c:27:5: warning: implicit declaration of function 
    ‘print_stackframe’ [-Wimplicit-function-declaration]
        print_stackframe();
        ^
    \+ cc kern/debug/kdebug.c
    kern/debug/kdebug.c:251:1: warning: ‘read_eip’ defined but not used [-Wunused-function]
    read_eip(void) {
    ^
    \+ cc kern/debug/kmonitor.c
    \+ cc kern/driver/clock.c
    \+ cc kern/driver/console.c
    \+ cc kern/driver/picirq.c
    \+ cc kern/driver/intr.c
    \+ cc kern/trap/trap.c
    kern/trap/trap.c:14:13: warning: ‘print_ticks’ defined but not used [-Wunused-function]
    static void print_ticks() {
                ^
    kern/trap/trap.c:30:26: warning: ‘idt_pd’ defined but not used [-Wunused-variable]
    static struct pseudodesc idt_pd = {
                            ^
    \+ cc kern/trap/vectors.S
    \+ cc kern/trap/trapentry.S
    \+ cc kern/mm/pmm.c
    \+ cc libs/string.c
    \+ cc libs/printfmt.c
    \+ ld bin/kernel
    \+ cc boot/bootasm.S
    \+ cc boot/bootmain.c
    \+ cc tools/sign.c
    \+ ld bin/bootblock
    'obj/bootblock.out' size: 484 bytes
    build 512 bytes boot sector: 'bin/bootblock' success!
    10000+0 records in
    10000+0 records out
    5120000 bytes (5.1 MB, 4.9 MiB) copied, 0.21819 s, 23.5 MB/s
    1+0 records in
    1+0 records out
    512 bytes copied, 0.000121855 s, 4.2 MB/s
    146+1 records in
    146+1 records out
    74828 bytes (75 kB, 73 KiB) copied, 0.00337746 s, 22.2 MB/s

### make "v=" 没有尝试


### 进入gdb界面后（完全记录）

    warning: A handler for the OS ABI "GNU/Linux" is not built into this configurati
    on
    of GDB.  Attempting to continue with the default i8086 settings.
    
    The target architecture is assumed to be i8086
    0x0000fff0 in ?? ()
    (gdb) x /2i 0xffff0
    0xffff0:     ljmp   $0xf000,$0xe05b
    0xffff5:     xor    %dh,0x322f
     (gdb) x /10i 0xfe05b
    0xfe05b:     cmpl   $0x0,%cs:0x6c48
    0xfe062:     jne    0xfd2e1
    0xfe066:     xor    %dx,%dx
    0xfe068:     mov    %dx,%ss
    0xfe06a:     mov    $0x7000,%esp
    0xfe070:     mov    $0xf3691,%edx
    0xfe076:     jmp    0xfd165
    ---Type <return> to continue, or q <return> to quit---return
    0xfe079:     push   %ebp
    0xfe07b:     push   %edi
    0xfe07d:     push   %esi
    (gdb) si
    0x0000e062 in ?? ()
    (gdb) si
    0x0000e066 in ?? ()
    (gdb) si
    0x0000e068 in ?? ()
    (gdb) x /10i 0xfe068
    0xfe068:     mov    %dx,%ss
    0xfe06a:     mov    $0x7000,%esp
    0xfe070:     mov    $0xf3691,%edx
    0xfe076:     jmp    0xfd165
    0xfe079:     push   %ebp
    0xfe07b:     push   %edi
    0xfe07d:     push   %esi
    0xfe07f:     push   %ebx
    0xfe081:     sub    $0x20,%esp
    0xfe085:     mov    %eax,%ebx

### 这里错误

    (gdb) b 0xf7c00
    No symbol table is loaded.  Use the "file" command.
    Make breakpoint pending on future shared library load? (y or [n]) y
    Breakpoint 1 (0xf7c00) pending.
    (gdb) x /10i 0xf7c00
    0xf7c00:     mov    %edx,0x10(%esp)
    0xf7c06:     mov    %cl,0x16(%esp)
    0xf7c0b:     movzbl 0x1c(%eax),%eax
    0xf7c11:     mov    0x19(%ebx),%cl
    0xf7c15:     mov    %cl,0x3(%esp)
    0xf7c1a:     mov    0x18(%ebx),%cl
    0xf7c1e:     mov    %cl,%dl
    0xf7c20:     and    $0x3f,%edx
    0xf7c24:     mov    %dl,0x1(%esp)
    0xf7c29:     mov    0x15(%ebx),%dl

## 这里错误

错误点：前面要加\*而且地址是07c00而不是f7c00,。

    (gdb) b *0x07c00
    Breakpoint 1 at 0x7c00
    (gdb) continue
    Continuing.
    Breakpoint 1, 0x00007c00 in ?? ()

### 正确的从7c00开始调试：

    Breakpoint 1, 0x00007c00 in ?? ()
    (gdb) x /10i 0x07c00
    => 0x7c00:      cli
    0x7c01:      cld
    0x7c02:      xor    %ax,%ax
    0x7c04:      mov    %ax,%ds
    0x7c06:      mov    %ax,%es
    0x7c08:      mov    %ax,%ss
    0x7c0a:      in     $0x64,%al
    0x7c0c:      test   $0x2,%al
    0x7c0e:      jne    0x7c0a
    0x7c10:      mov    $0xd1,%al
    0x00007c10 in ?? ()
    (gdb) x /10i 0x7c10
    => 0x7c10:      mov    $0xd1,%al
    0x7c12:      out    %al,$0x64
    0x7c14:      in     $0x64,%al
    0x7c16:      test   $0x2,%al
    0x7c18:      jne    0x7c14
    0x7c1a:      mov    $0xdf,%al
    0x7c1c:      out    %al,$0x60
    0x7c1e:      lgdtw  0x7c6c
    0x7c23:      mov    %cr0,%eax
    0x7c26:      or     $0x1,%eax
    (gdb) x /10i 0x07c26
    => 0x7c26:      or     $0x1,%eax
    0x7c2a:      mov    %eax,%cr0
    0x7c2d:      ljmp   $0x8,$0x7c32
    0x7c32:      mov    $0xd88e0010,%eax
    0x7c38:      mov    %ax,%es
    0x7c3a:      mov    %ax,%fs
    0x7c3c:      mov    %ax,%gs
    0x7c3e:      mov    %ax,%ss
    0x7c40:      mov    $0x0,%bp
    0x7c43:      add    %al,(%bx,%si)
    (gdb) x /10i 0x07c45
    => 0x7c45:      mov    $0x7c00,%sp
    0x7c48:      add    %al,(%bx,%si)
    0x7c4a:      call   0x7d07
    0x7c4d:      add    %al,(%bx,%si)
    0x7c4f:      jmp    0x7c4f
    0x7c51:      lea    0x0(%bp),%si
    0x7c54:      add    %al,(%bx,%si)
    0x7c56:      add    %al,(%bx,%si)
    0x7c58:      add    %al,(%bx,%si)
    0x7c5a:      add    %al,(%bx,%si)
    ......
    (gdb) 

### 单步调试 vs bootasm.S

    0x7c01:      cld
    0x7c02:      xor    %ax,%ax
    0x7c04:      mov    %ax,%ds
    0x7c06:      mov    %ax,%es
    0x7c08:      mov    %ax,%ss
    0x7c0a:      in     $0x64,%al
    0x7c0c:      test   $0x2,%al
    0x7c0e:      jne    0x7c0a
    0x7c10:      mov    $0xd1,%al
    0x7c12:      out    %al,$0x64
    0x7c14:      in     $0x64,%al
    0x7c16:      test   $0x2,%al
    0x7c18:      jne    0x7c14
    0x7c1a:      mov    $0xdf,%al
    0x7c1c:      out    %al,$0x60
    0x7c1e:      lgdtw  0x7c6c
    0x7c23:      mov    %cr0,%eax
    0x7c26:      or     $0x1,%eax
    0x7c2a:      mov    %eax,%cr0
    0x7c2d:      ljmp   $0x8,$0x7c32
    0x7c32:      mov    $0xd88e0010,%eax
    0x7c38:      mov    %ax,%es
    0x7c3a:      mov    %ax,%fs
    0x7c3c:      mov    %ax,%gs
    0x7c3e:      mov    %ax,%ss
    0x7c40:      mov    $0x0,%bp
    0x7c43:      add    %al,(%bx,%si)
    0x7c45:      mov    $0x7c00,%sp
    0x7c48:      add    %al,(%bx,%si)
    0x7c4a:      call   0x7d07
    0x7c4d:      add    %al,(%bx,%si)
    0x7c4f:      jmp    0x7c4f
    0x7c51:      lea    0x0(%bp),%si
    0x7c54:      add    %al,(%bx,%si)
    0x7c56:      add    %al,(%bx,%si)
    0x7c58:      add    %al,(%bx,%si)
    0x7c5a:      add    %al,(%bx,%si)
    ......

## 3 Exercise 4

## 4 Exercise 5

- 基本就是按照需要填写代码部分给出的逻辑提示进行编写。

#### 填写后的代码块如下

```C
void print_stackframe(void) {
     /* LAB1 YOUR CODE : STEP 1 */
     /* (1) call read_ebp() to get the value of ebp. the type is (uint32_t);
      * (2) call read_eip() to get the value of eip. the type is (uint32_t);
      * (3) from 0 .. STACKFRAME_DEPTH
      *    (3.1) printf value of ebp, eip
      *    (3.2) (uint32_t)calling arguments [0..4] = the contents in address (uint32_t)ebp +2 [0..4]
      *    (3.3) cprintf("\n");
      *    (3.4) call print_debuginfo(eip-1) to print the C calling function name and line number, etc.
      *    (3.5) popup a calling stackframe
      *           NOTICE: the calling funciton's return addr eip  = ss:[ebp+4]
      *                   the calling funciton's ebp = ss:[ebp]
      */
	uint32_t ebp = read_ebp(); //(1)
    uint32_t eip = read_eip(); //(2)
    for(int i=0;i<STACKFRAME_DEPTH && ebp!=0;i++){
    	cprintf("ebp:0x%08x eip:0x%08x args:",ebp,eip); //(3.1) 
    	uint32_t *calling_arguments = (uint32_t *) ebp; 
    	for(int j=0;j<4;j++){
    		cprintf(" 0x%08x ", calling_arguments[j]); //(3.2)
		}
		cprintf("\n"); //(3.3)
		print_debuginfo(eip-1); //(3.4)
    	eip = ((uint32_t *)ebp)[1]; 
    	ebp = ((uint32_t *)ebp)[0]; //(3.5)
	}
}
```

- 这个exer想干什么？单纯的想实现栈帧信息打印。

- 什么原理？ebp可以想象成是一个线性链表。ebp指向上层函数的基地址，跳跳跳跳到最后。

- ebp和eip可以通过某个内联函数直接获取当前的ebp和eip。假设我们递归调用了一个函数，我们怎么从内层回溯到外层函数信息？利用函数调用时最后push进去的返回地址（ebp）。

- 怎么利用？见下图。ss:[ebp+4]为返回地址，ss:[ebp+8]为传入的第一个参数（具体是什么意思？不知道，没有给出解释。参数为什么读4个？不知道，提示要求的。）

- 参数到哪里？从地址ebp+8开始到ss:[ebp+4]读出的值（返回地址）

  ![image-20201015154006190](C:\Users\78479\AppData\Roaming\Typora\typora-user-images\image-20201015154006190.png)

一些可能不熟悉的点：

1. %08x表示把输出的整数按照8位16进制格式（不包括‘0x’）输出，不足8位的部分用0填充。

一些踩了的坑：

1. 一开始使用的printf而不是printf，发现报错了，说没引入stdio.h，但其实引了，不知道为啥。

2. 一开始循环判断没写ebp!=0，导致都到栈底了，还在打印，如下图。

   ![image-20201015154653589](C:\Users\78479\AppData\Roaming\Typora\typora-user-images\image-20201015154653589.png)

   正常的应该是下图。

   ![image-20201015154753217](C:\Users\78479\AppData\Roaming\Typora\typora-user-images\image-20201015154753217.png)

   

```asm
rexxar@rexxar-virtual-machine:~/ucore_os/labcodes/lab1$ make qemu
+ cc kern/debug/kdebug.c
+ ld bin/kernel
记录了10000+0 的读入
记录了10000+0 的写出
5120000 bytes (5.1 MB, 4.9 MiB) copied, 0.092992 s, 55.1 MB/s
记录了1+0 的读入
记录了1+0 的写出
512 bytes copied, 0.000121417 s, 4.2 MB/s
记录了154+1 的读入
记录了154+1 的写出
78912 bytes (79 kB, 77 KiB) copied, 0.000961922 s, 82.0 MB/s
WARNING: Image format was not specified for 'bin/ucore.img' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
(THU.CST) os is loading ...

Special kernel symbols:
  entry  0x00100000 (phys)
  etext  0x0010341d (phys)
  edata  0x0010fa16 (phys)
  end    0x00110d20 (phys)
Kernel executable memory footprint: 68KB
ebp:0x00007b28 eip:0x00100ab3 args: 0x00010094  0x00010094  0x00007b58  0x00100096 
    kern/debug/kdebug.c:306: print_stackframe+25
ebp:0x00007b38 eip:0x00100db5 args: 0x00000000  0x00000000  0x00000000  0x00007ba8 
    kern/debug/kmonitor.c:125: mon_backtrace+14
ebp:0x00007b58 eip:0x00100096 args: 0x00000000  0x00007b80  0xffff0000  0x00007b84 
    kern/init/init.c:48: grade_backtrace2+37
ebp:0x00007b78 eip:0x001000c4 args: 0x00000000  0xffff0000  0x00007ba4  0x00000029 
    kern/init/init.c:53: grade_backtrace1+42
ebp:0x00007b98 eip:0x001000e7 args: 0x00000000  0x00100000  0xffff0000  0x0000001d 
    kern/init/init.c:58: grade_backtrace0+27
ebp:0x00007bb8 eip:0x00100111 args: 0x0010343c  0x00103420  0x0000130a  0x00000000 
    kern/init/init.c:63: grade_backtrace+38
ebp:0x00007be8 eip:0x00100055 args: 0x00000000  0x00000000  0x00000000  0x00007c4f 
    kern/init/init.c:28: kern_init+84
ebp:0x00007bf8 eip:0x00007d74 args: 0xc031fcfa  0xc08ed88e  0x64e4d08e  0xfa7502a8 
    <unknow>: -- 0x00007d73 --
++ setup timer interrupts

```

## 6 Challenges
相比之下，2更好设计。在KBD中断内设置对字符的检查，实现0-ring3，3-ring0的转换，并设置有当前