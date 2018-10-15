#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	int i;
	srand(time(0));
	for(i = 0; i < 15; i++)
		printf("%d %d\n", rand()%5, rand()%15);
}
