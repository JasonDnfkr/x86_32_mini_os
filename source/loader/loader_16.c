/*
    这里的代码要在16位执行，编译后生成的代码
    都是16位的
 */

__asm__(".code16gcc");

static void show_msg(const char* msg) {
    char c;
    while ((c = *msg++) != '\0') {
        __asm__ __volatile__(
            "mov $0xe, %%ah\n\t"
            "mov %[ch], %%al\n\t"
            "int $0x10"::[ch]"r"(c)
        );
    }
}

void loader_entry(void) {
    show_msg("....loading....\n\r");
    while (1) { }
}

 