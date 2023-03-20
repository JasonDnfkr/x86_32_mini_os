### 7 memory

#### 1 位图管理

用位图管理内存中的页。1对应该页占用，0为空闲

<img src="7 memory_pic/image-20230319160603258.png" alt="image-20230319160603258" style="zoom:67%;" />



关键结构：内存分配器

```C
// 内存分配器
// mutex: 互斥锁
// bitmap: 位图
// start: 起始地址
// size: 需要分配的内存大小
// page_size: 分配的页大小 (4096等)
typedef struct _addr_alloc_t {
    mutex_t mutex;
    bitmap_t bitmap;

    uint32_t start;
    uint32_t size;
    uint32_t page_size; // 页大小
} addr_alloc_t;
```

可以指定start起始内存地址，指定size该内存块的大小，指定page_size页大小，然后划定一段区域。可以通过`addr_alloc_page`创建页表，`addr_free_page`释放页表。



#### 2 内存规划

<img src="7 memory_pic/image-20230319175853099.png" alt="image-20230319175853099" style="zoom:67%;" />



将整个系统的内存位示图放在了 0x10000后面一些的位置，占用大小不多，用这一块位示图来管理整个OS的内存分页。



#### 3 大页表



采用大页表 4MiB。只是为了前期用下

<img src="7 memory_pic/image-20230319202408463.png" alt="image-20230319202408463" style="zoom:67%;" />

Page Directory中的每一项，高10位存放了指向大页的地址，低位是标志位。



#### 4 二级页表

使用结构体：

```C
// 地址映射表, 用于建立内核级的地址映射
// 地址不变，但是添加了属性
static memory_map_t kernel_map[] = {
    { kernel_base,   s_text,                         kernel_base,    PTE_W },     // 内核栈区
    { s_text,        e_text,                         s_text,         0 },         // 内核代码区
    { s_data,        (void *)(MEM_EBDA_START - 1),   s_data,         PTE_W },     // 内核数据区
    // 扩展存储空间一一映射，方便直接操作
    { (void *)MEM_EXT_START, (void *)MEM_EXT_END,    (void *)MEM_EXT_START, PTE_W },
};

```

用memory_map保存了虚拟地址对物理地址的映射关系。

设计了一个内存管理器 addr_alloc_t，用于管理整个内存区域的地址。主要用来检测哪些内存区域可用，哪些地方不可用。其实是可以用数组来索引位置的，但是用bitmap来管理，更加节省空间。



**问题：为什么创建的页表地址都是4k对齐的？**

因为，链接器里指定了.text和.rodata都是从0x10000处开始的，这里是4K对齐的；

然后使用了 .ALIGN(4096)，使得.data段到结束位置也是4K对齐的



##### 关键函数

在给定的页表中建立虚拟地址到物理地址的映射，与 xv6 的 walk 极为相似

`memory_create_map(kernel_page_dir, vstart, (uint32_t)map->pstart, page_count, map->perm);`

函数流程：

取vstart的31~22位，作为一级页表的下标，在PDE页表中根据这个下标，读出二级页表的地址；

这个地址可能是不存在的，如果不存在，就当场创建一个：用内存管理器创建一个物理页，把PDE的字段内容设置成该物理页的地址 + PDE_PRESENT。

根据vstart的21~12位，作为二级页表的下标，索引这个字段。这个字段的内容就是物理地址所在地。然后加上 perm | PTE_P



**对页表权限控制有决定性作用的是二级页表PTE，即，二级页表的perm会自动覆盖一级页表的perm**



#### 5 内核页表





#### 6 进程空间隔离

进程初始化时，会给TSS结构中的CR3字段，生成一个地址。该地址是`memory_create_uvm()`的结果，这个函数创建了一个一级页表，

回顾：进程切换时，将当前的寄存器保存至TR储存的地址中，这个地址放的是该进程的tss结构体。





#### ====== 自我总结 ======

**1. 内存布局**

项目内存布局如下：

0x7c00: boot  0x8000: loader   0x10000(64KiB): kernel  ... bitmap ... 

0x80000 ... 0xFFFFF: EBDA

0x100000(1MiB): memory init 后所有的内容



**2. 内存分配**

使用内存管理器来完成这个事情。

bitmap管理器：可以从指定的起始地址，划定一块区域（指定内存结束处），指定每个bit代表的内存大小。项目里，起始地址是1MiB处，结束内存区域是127MiB处，每个bit代表大小是一页(4096Bytes)

需要分配内存页时，会调用bitmap管理器，从设计好的起始地址处自动搜索可用的内存位置，映射到实际内存中，然后修改bit位。

**3. 内存映射**

- 针对一个虚拟内存，先找它是否有物理地址对应：
  - 先找它的二级页表是否存在，如果存在则说明是有映射关系的。具体做法是，读取虚拟地址的31~22位，作为页目录表（一级页表）的下标，如果该下标指向的元素（也就是pde_t）存在PRESENT位，则说明该映射是存在的
  - 如果PRESENT位为0，则新建一个物理页，将这个物理页的地址作为pde_t的值，然后加上PRESENT位。这个物理页就是二级页表。把虚拟地址的21~12位作为二级页表的下标，把该下标指向的元素的值设为虚拟地址的 31~12 位，再加上标识位。