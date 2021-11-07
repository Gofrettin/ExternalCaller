#include <stdio.h>


__declspec(noinline) int __cdecl_sum(int a, int b)
{
    printf("test_app::sum_cdecl function was called with arguments: "
        "%d(0x%x), %d(0x%x)\n", a, a, b, b);
    return a + b;
}


int main()
{
    printf("test_app::sum_cdecl function address: %p\n\n", __cdecl_sum);
    __cdecl_sum(1, 1);
    for (;;);
    return 0;
}
