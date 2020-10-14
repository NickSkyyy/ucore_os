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

3. kernel的依赖文件是一个集合KOBJS，而文件具体内容由KSRCDIR指定

## 2 Exercise 2



## 3 Exercise 3



## 4 Exercise 4