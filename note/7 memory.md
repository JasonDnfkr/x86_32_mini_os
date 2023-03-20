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

在目前的设计中，0x10000（64KiB）以上的内存空间，是空白的。

设定，将1MiB(0x100000)以上的空间给进程使用。

<img src="7 memory_pic/image-20230319175853099.png" alt="image-20230319175853099" style="zoom:67%;" />



将整个系统的内存位示图放在了 0x10000后面一些的位置，占用大小不多，用这一块位示图来管理整个OS的内存分页。



#### 3 大页表



采用大页表 4MiB

<img src="7 memory_pic/image-20230319202408463.png" alt="image-20230319202408463" style="zoom:67%;" />

Page Directory中的每一项，高10位存放了指向大页的地址，低位是标志位？？



使用结构体：

```C
typedef struct _memory_map_t {
    void* vstart;   // 虚拟地址的起始处
    void* vend;     // 虚拟地址的结束处
    void* pstart;   // 这一块区域的物理地址起始处
    uint32_t perm;  // 特权控制字段
} memory_map_t;

memory_map_t memory_map[] = {
    { kernbase,        text_start_addr,       kernbase,        0 },
    { text_start_addr, text_end_addr,         text_start_addr, 0 },
    { data_start_addr, (void*)MEM_EBDA_START, data_start_addr, 0 },
};

```

用memory_map保存了虚拟地址对物理地址的映射关系