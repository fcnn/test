#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

void testf(ucontext_t *ctx)
{
	setcontext(ctx);
}

int i = 0;
int main()
{
	ucontext_t *ctx = (ucontext_t*)malloc(sizeof(ucontext_t));
	getcontext(ctx);
	 for (; i < (1<<21); ) {
		++i;
		testf(ctx);
	}
	printf("returned. %i\n", (int)sizeof (ucontext_t));
	free(ctx);
}
