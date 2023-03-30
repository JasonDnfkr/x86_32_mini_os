#include "lib_syscall.h"

int main(int argc, char** argv) {

    for (int i = 0; i < argc; i++) {
        print_msg("[arg]: %s", (int)argv[i]);
    }

    fork();


    while (1) {
        // if (pid == 0) {
        //     print_msg("child pid in main.c: %d", getpid());
        // }
        print_msg("main.c: %d", getpid());
        msleep(3000);

        return 0;
    }
    
}