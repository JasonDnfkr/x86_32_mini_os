#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_PARAM_COUNT     5

#define SYS_sleep           0
#define SYS_getpid          1

// 系统调用的栈信息
typedef struct _syscall_frame_t {
	int eflags;
	int gs, fs, es, ds;
	int edi, esi, ebp, dummy, ebx, edx, ecx, eax;   // dummy 就是esp，与下面的esp冲突
	int eip, cs;
	int func_id, arg0, arg1, arg2, arg3;
	int esp, ss;
} syscall_frame_t;


void exception_handler_syscall(void);

void do_handler_syscall(syscall_frame_t* frame);

#endif