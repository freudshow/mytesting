#include <stdio.h>
#include <setjmp.h>

jmp_buf jump_buffer;

void function_b()
{
    printf("进入 function_b\n");
    longjmp(jump_buffer, 42); // 跳转回 setjmp 的位置，并传递值 42
    printf("这行不会执行\n");
}

void function_a()
{
    printf("进入 function_a\n"); // 调用 function_b
    function_b();
    printf("这行也不会执行\n");
}

/*
 * 输出:​
 *
 * 首次调用 setjmp，进入 function_a
 * 进入 function_a
 * 进入 function_b
 * 从 longjmp 返回，ret = 42
 */
void jmpmain(void)
{
    int ret = setjmp(jump_buffer); // 第一次调用返回 0，跳转后返回 42
    if (ret == 0)
    {
        printf("首次调用 setjmp，进入 function_a\n");
        function_a();
    }
    else
    {
        printf("从 longjmp 返回，ret = %d\n", ret);
    }
}
