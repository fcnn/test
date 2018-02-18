#include <setjmp.h>
#include <stdio.h>

struct defer_t {
    long rsp_;
    jmp_buf env1_;
    jmp_buf env2_;

    ~defer_t() {
        int n = setjmp(env2_);
        if (n == 0) {
            asm("mov %%rsp, %0\n" :"=m"(rsp_) ::);
            printf("save rsp %p %p %p\n", (void *) rsp_, this, __builtin_return_address(0));
            longjmp(env1_, 1);
        }
    }
};

#define _defer(l, ...) defer_t df_##l; \
    if (1 == setjmp(df_##l.env1_)) { \
        __asm__ __volatile ("mov %0, %%rsp\n" ::"m"(df_##l.rsp_) :"rsp"); \
        __VA_ARGS__; \
        longjmp(df_##l.env2_, 1); \
    }

#define _defer_2(l, ...) _defer(l, ##__VA_ARGS__)
#define defer(...) _defer_2(__LINE__, ##__VA_ARGS__)

int test() {
    struct a {
        ~a() {
            printf("now in %s\n", __func__);
        }
    };

    int var = 9;
    if (var == 9) {
        puts("normal 1");
        defer({
            delete (new a);
            puts("deferred delete done");
        });
    }

    defer(printf("%d: deffered %d\n", __LINE__, var));
    puts("test returning");
    return 0;
}

int main() {
    test();
    puts("main exiting");
}
