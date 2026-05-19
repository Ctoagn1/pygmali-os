static inline int syscall(int num, int arg1, int arg2, int arg3) {
    int ret;
    asm volatile (
        "int $0x80"             
        : "=a"(ret)            
        : "a"(num),             
          "b"(arg1),            
          "c"(arg2),            
          "d"(arg3)             
        : "memory"              
    );
    return ret;
}
