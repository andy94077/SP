#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char const *argv[])
{
	FILE *f;
	int charset[256]={0};
	for (int i = 0; argv[1][i] != '\0';i++)
		charset[argv[1][i]]=1;
	
	if (argc == 2)
		f = stdin;
	else
	{
		f = fopen(argv[2], "r");
		if (f == NULL)
		{
			fprintf(stderr, "error\n");
			return 1;
		}
	}

	int c,sum=0;
	while((c=fgetc(f))!=EOF)
	{
		if(c=='\n')
		{
			printf("%d\n", sum);
			sum = 0;
		}
		else if(charset[c])
			sum++;
	}
	
	return 0;
}
