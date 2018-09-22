#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char const *argv[])
{
	FILE *f;
	const char *charset = argv[1];
	if(argc==2)
		f=stdin;
	else
	{
		f = fopen(argv[2], "r");
		if(f==NULL)
		{
			fprintf(stderr,"error\n");
			exit(8);
		}
	}

	unsigned long long table[128] = {0};
	int c;
	while((c=fgetc(f))!=EOF)
	{
		if(c=='\n')
		{
			unsigned long long sum = 0;
			for (int i = 0; charset[i] != '\0';i++)
				sum += table[charset[i]];
			printf("%llu\n", sum);
			memset(table, 0, sizeof(table));
		}
		else
			table[c]++;
	}
	
	unsigned long long sum = 0;
	for (int i = 0; charset[i] != '\0';i++)
		sum += table[charset[i]];
	printf("%llu\n", sum);
	return 0;
}
