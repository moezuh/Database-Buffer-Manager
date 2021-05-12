// *** Use this to look at the blocks in the pagefile -> testbuffer.bin***
#include <stdlib.h>
#include<stdio.h>
#include<string.h>

void main()
{
    int PAGE_SIZE = 4096;
    typedef char* SM_PageHandle;
    SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
    FILE *fp;
    fp = fopen("testbuffer.bin", "r");
    fseek(fp, 0, SEEK_END);
    int totalNumPages = ftell(fp) / PAGE_SIZE;
    int i;
    fseek(fp,0,SEEK_SET);
    for (i = 0; i < totalNumPages; i++)
    {
		//fseek to set the file position to offset
		
		//Now, read the contents and, store it in the location pointed by memPage
		fread(ph,1,PAGE_SIZE,fp);
        printf("block: %d --- content: %s\n", i, ph);
    }
    fclose(fp);
}