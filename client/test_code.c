#include <unistd.h>
#include <stdio.h>

int main()
{
    int n=1;
    while(n<15)
    {
        printf("Time: %d\n",n);
        n++;
        sleep(1);
    }
    return 0;
}