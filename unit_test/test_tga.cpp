
#if !defined(_CRT_SECURE_NO_WARNINGS) || !_CRT_SECURE_NO_WARNINGS
#	define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <memory.h>
#include "tga.h"


typedef void (*ptfn)(FILE* fout, FILE* fin);
void encodeFile(FILE* fout, FILE* fin);
void decodeFile(FILE* fout, FILE* fin);

struct {
	char	m_id;
	ptfn	m_ptfn;
} g_commands[] =
{
	{'c',	encodeFile},
	{'d',	decodeFile}
};

int main(int argc, char** argv)
{
	if(argc<4)
	{
		puts("usage: pgm cmd infile outfile\n");
		puts("cmd: c - compress / d - decompress");
	}
	else
	{
		char* cmd = argv[1];
		char* inputFileName = argv[2];
		char* outputFileName = argv[3];

		for(int i=0; i<sizeof(g_commands)/sizeof(g_commands[0]); ++i)
		{
			if(*cmd == g_commands[i].m_id)
			{
				FILE* fin = fopen(inputFileName, "rb");
				FILE* fout = fopen(outputFileName, "wb");
				if(fin && fout)
				{
					g_commands[i].m_ptfn(fout, fin);

					fclose(fin);
					fclose(fout);
				}
				else
					perror("error on file opening");
			}
		}
	}

	return 0;
}

