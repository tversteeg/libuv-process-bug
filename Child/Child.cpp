#include <stdio.h>

#include <Windows.h>

int main()
{
    printf("Hello from child process\n");

    int sleep = 10;
    while (sleep-- > 0)
    {
        printf("Child process sleeping for %d more second(s)\n", sleep + 1);
        fflush(stdout);

        Sleep(1000);
    }

    return 0;
}