# Details

Date : 2023-03-24 09:26:00

Directory e:\\Data\\Work\\Job\\Cpp\\diy-x86os\\start\\start

Total : 68 files,  3429 codes, 585 comments, 1670 blanks, all 5684 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [README.md](/README.md) | Markdown | 3 | 0 | 0 | 3 |
| [note/10 fork.md](/note/10%20fork.md) | Markdown | 13 | 0 | 8 | 21 |
| [note/11 exec.md](/note/11%20exec.md) | Markdown | 15 | 0 | 15 | 30 |
| [note/1_bios.md](/note/1_bios.md) | Markdown | 49 | 0 | 43 | 92 |
| [note/2_loader.md](/note/2_loader.md) | Markdown | 150 | 0 | 116 | 266 |
| [note/3_interrupt.md](/note/3_interrupt.md) | Markdown | 56 | 0 | 66 | 122 |
| [note/4_printf.md](/note/4_printf.md) | Markdown | 3 | 0 | 2 | 5 |
| [note/5_task_sche.md](/note/5_task_sche.md) | Markdown | 63 | 0 | 70 | 133 |
| [note/6_mutex.md](/note/6_mutex.md) | Markdown | 16 | 0 | 19 | 35 |
| [note/7 memory.md](/note/7%20memory.md) | Markdown | 72 | 0 | 77 | 149 |
| [note/8 process.md](/note/8%20process.md) | Markdown | 46 | 0 | 59 | 105 |
| [note/9 syscall.md](/note/9%20syscall.md) | Markdown | 29 | 0 | 43 | 72 |
| [script/img-write-linux.sh](/script/img-write-linux.sh) | Shell Script | 18 | 16 | 10 | 44 |
| [script/img-write-osx.sh](/script/img-write-osx.sh) | Shell Script | 18 | 15 | 11 | 44 |
| [script/img-write-win.bat](/script/img-write-win.bat) | Batch | 15 | 24 | 10 | 49 |
| [script/qemu-debug-linux.sh](/script/qemu-debug-linux.sh) | Shell Script | 1 | 1 | 1 | 3 |
| [script/qemu-debug-osx.sh](/script/qemu-debug-osx.sh) | Shell Script | 1 | 1 | 1 | 3 |
| [script/qemu-debug-win.bat](/script/qemu-debug-win.bat) | Batch | 1 | 1 | 1 | 3 |
| [source/applib/crt0.S](/source/applib/crt0.S) | RISC-V Assembly | 10 | 1 | 2 | 13 |
| [source/applib/cstart.c](/source/applib/cstart.c) | C | 8 | 0 | 2 | 10 |
| [source/applib/lib_syscall.h](/source/applib/lib_syscall.h) | C | 69 | 4 | 26 | 99 |
| [source/boot/boot.c](/source/boot/boot.c) | C | 6 | 9 | 6 | 21 |
| [source/boot/boot.h](/source/boot/boot.h) | C | 3 | 5 | 2 | 10 |
| [source/boot/start.S](/source/boot/start.S) | RISC-V Assembly | 44 | 5 | 9 | 58 |
| [source/comm/boot_info.h](/source/comm/boot_info.h) | C | 14 | 0 | 7 | 21 |
| [source/comm/cpu_instr.h](/source/comm/cpu_instr.h) | C | 92 | 14 | 40 | 146 |
| [source/comm/elf.h](/source/comm/elf.h) | C | 39 | 0 | 8 | 47 |
| [source/comm/types.h](/source/comm/types.h) | C | 6 | 0 | 2 | 8 |
| [source/kernel/core/memory.c](/source/kernel/core/memory.c) | C | 297 | 113 | 120 | 530 |
| [source/kernel/core/syscall.c](/source/kernel/core/syscall.c) | C | 28 | 1 | 8 | 37 |
| [source/kernel/core/task.c](/source/kernel/core/task.c) | C | 438 | 89 | 185 | 712 |
| [source/kernel/cpu/cpu.c](/source/kernel/cpu/cpu.c) | C | 60 | 14 | 25 | 99 |
| [source/kernel/cpu/irq.c](/source/kernel/cpu/irq.c) | C | 234 | 40 | 78 | 352 |
| [source/kernel/dev/timer.c](/source/kernel/dev/timer.c) | C | 24 | 7 | 10 | 41 |
| [source/kernel/fs/fs.c](/source/kernel/fs/fs.c) | C | 56 | 7 | 22 | 85 |
| [source/kernel/include/core/memory.h](/source/kernel/include/core/memory.h) | C | 41 | 25 | 24 | 90 |
| [source/kernel/include/core/syscall.h](/source/kernel/include/core/syscall.h) | C | 20 | 1 | 7 | 28 |
| [source/kernel/include/core/task.h](/source/kernel/include/core/task.h) | C | 63 | 8 | 38 | 109 |
| [source/kernel/include/cpu/cpu.h](/source/kernel/include/cpu/cpu.h) | C | 75 | 29 | 46 | 150 |
| [source/kernel/include/cpu/irq.h](/source/kernel/include/cpu/irq.h) | C++ | 102 | 6 | 35 | 143 |
| [source/kernel/include/cpu/mmu.h](/source/kernel/include/cpu/mmu.h) | C++ | 63 | 8 | 18 | 89 |
| [source/kernel/include/cpu/os_cfg.h](/source/kernel/include/cpu/os_cfg.h) | C | 11 | 1 | 9 | 21 |
| [source/kernel/include/dev/timer.h](/source/kernel/include/dev/timer.h) | C++ | 11 | 0 | 5 | 16 |
| [source/kernel/include/fs/fs.h](/source/kernel/include/fs/fs.h) | C++ | 8 | 0 | 6 | 14 |
| [source/kernel/include/ipc/mutex.h](/source/kernel/include/ipc/mutex.h) | C++ | 15 | 0 | 8 | 23 |
| [source/kernel/include/ipc/sem.h](/source/kernel/include/ipc/sem.h) | C | 12 | 0 | 8 | 20 |
| [source/kernel/include/tools/bitmap.h](/source/kernel/include/tools/bitmap.h) | C | 14 | 5 | 9 | 28 |
| [source/kernel/include/tools/klib.h](/source/kernel/include/tools/klib.h) | C++ | 30 | 4 | 24 | 58 |
| [source/kernel/include/tools/list.h](/source/kernel/include/tools/list.h) | C | 45 | 0 | 23 | 68 |
| [source/kernel/include/tools/log.h](/source/kernel/include/tools/log.h) | C | 5 | 0 | 4 | 9 |
| [source/kernel/init/first_task.c](/source/kernel/init/first_task.c) | C | 56 | 2 | 8 | 66 |
| [source/kernel/init/first_task_entry.S](/source/kernel/init/first_task_entry.S) | RISC-V Assembly | 10 | 2 | 2 | 14 |
| [source/kernel/init/init.c](/source/kernel/init/init.c) | C | 96 | 47 | 49 | 192 |
| [source/kernel/init/init.h](/source/kernel/init/init.h) | C++ | 3 | 0 | 2 | 5 |
| [source/kernel/init/start_kernel.S](/source/kernel/init/start_kernel.S) | RISC-V Assembly | 109 | 19 | 34 | 162 |
| [source/kernel/ipc/mutex.c](/source/kernel/ipc/mutex.c) | C | 44 | 0 | 10 | 54 |
| [source/kernel/ipc/sem.c](/source/kernel/ipc/sem.c) | C | 39 | 2 | 15 | 56 |
| [source/kernel/kernel.lds](/source/kernel/kernel.lds) | LinkerScript | 29 | 0 | 7 | 36 |
| [source/kernel/tools/bitmap.c](/source/kernel/tools/bitmap.c) | C | 50 | 18 | 17 | 85 |
| [source/kernel/tools/klib.c](/source/kernel/tools/klib.c) | C | 192 | 9 | 43 | 244 |
| [source/kernel/tools/list.c](/source/kernel/tools/list.c) | C | 63 | 0 | 24 | 87 |
| [source/kernel/tools/log.c](/source/kernel/tools/log.c) | C | 35 | 1 | 12 | 48 |
| [source/loader/loader.h](/source/loader/loader.h) | C | 16 | 0 | 7 | 23 |
| [source/loader/loader_16.c](/source/loader/loader_16.c) | C | 63 | 9 | 27 | 99 |
| [source/loader/loader_32.c](/source/loader/loader_32.c) | C | 71 | 19 | 31 | 121 |
| [source/loader/start_loader.S](/source/loader/start_loader.S) | RISC-V Assembly | 18 | 1 | 5 | 24 |
| [source/shell/link.lds](/source/shell/link.lds) | LinkerScript | 16 | 0 | 5 | 21 |
| [source/shell/main.c](/source/shell/main.c) | C | 7 | 2 | 4 | 13 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)