#include <stdlib.h>
#include<stdio.h>
#include "storage_mgr.h"
#include "dberror.h"
#include<string.h>

FILE *fp;
//function definitions
extern void initStorageManager (void)
{
	printf("SM Storage Manager has been initialized\n");  // Initialize storage manager
	return;
}


extern RC createPageFile (char *fileName)
{
	fp = fopen(fileName, "w");  // Create/Overwrite a file
	if (fp == NULL)  
	{
		fclose(fp);
		exit(1);  // Exit if there is an error creating the file
	}
	else
	{
		int i;
   		for(i = 0; i < PAGE_SIZE; i++)  // Fill in single page with 0 byte
		{
			fwrite("\0",1, 1,fp);
			fseek(fp,0,SEEK_END);
		}
		fclose(fp);
		return RC_OK;  // Return success code
	}
}
	

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
	fp = fopen(fileName, "r+");  // Open in read + write mode and check if the file exists
	if (fp == NULL)
		return RC_FILE_NOT_FOUND;  // Return corresponding error code
	else
	{
		fseek(fp, 0, SEEK_END);  // Move the stream to the end of file
		// Update fHandle with file details
		fHandle->fileName = fileName;  
		fHandle->totalNumPages = ftell(fp) / PAGE_SIZE;  // Get total number of pages and store it
		fHandle->mgmtInfo = fp;  //  Store the address of file pointer, so that it can be accessed by other functions
		fHandle->curPagePos = 0;
		fclose(fp);
		return RC_OK;  // Return success code
	}
}


extern RC closePageFile (SM_FileHandle *fHandle)
{
	fp = fHandle->mgmtInfo;  // Get file pointer stored in the data structure
	if(fp != NULL)
		fp = NULL;
	return RC_OK;  // Return success code
}


extern RC destroyPageFile (char *fileName)
{
	fp = fopen(fileName, "r");  // Check if file exists
	if (fp != NULL)
	{
		fclose(fp);
		remove(fileName);  // Destroy the file
		return RC_OK;
	}
	else
		return RC_FILE_NOT_FOUND;
}


//Implementing function 1 readBlock
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{			
	//check if the file is present or not
	if(fopen(fHandle->fileName,"r") == NULL)
		return RC_FILE_NOT_FOUND;
	else
	{
		fp = fopen(fHandle->fileName, "r");
		//defining offset
		int offset = pageNum*PAGE_SIZE;
		//fseek to set the file position to offset
		fseek(fp,offset,SEEK_SET);
		//Now, read the contents and, store it in the location pointed by memPage
		fread(memPage,1,PAGE_SIZE,fp);
		//updating to the current pointer position using ftell
		fHandle->curPagePos=ftell(fp)/PAGE_SIZE; 
		fclose(fp);              
		return RC_OK;
    }
}


//implementing function 2 getBlockPos (SM_FileHandle *fHandle)
int getBlockPos (SM_FileHandle *fHandle)
{
	//In order to get the current page position in a file 
	return fHandle->curPagePos;
}


//implementing function 3 readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{ 			
	if (fHandle->mgmtInfo == NULL)
		return RC_FILE_NOT_FOUND;
    
	// reading first block
	readBlock(0, fHandle, memPage);

   //use ftell to set current file position to curPagePos 
    fHandle->curPagePos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;
	return RC_OK;
}


//implementing function 4 readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //check for Null
    if (fopen(fHandle->fileName,"r")==NULL)
		return RC_FILE_NOT_FOUND;

	if (fHandle->curPagePos<=0)
        return RC_READ_NON_EXISTING_PAGE;

	int offset;
    //defining offset
    offset=fHandle->curPagePos-1;
    //calling readBlock function
    readBlock(offset,fHandle,memPage);

   //use ftell to set current file position to curPagePos 
    fHandle->curPagePos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;	
	return RC_OK;
}


//implementing function 5 readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //check for if File exists
    if (fopen(fHandle->fileName,"r")==NULL)
        return RC_FILE_NOT_FOUND;
    
    //check for invalid page number
    if (fHandle->curPagePos>fHandle->totalNumPages-1)
        return RC_READ_NON_EXISTING_PAGE;
    
    if (fHandle->curPagePos<0)
        return RC_READ_NON_EXISTING_PAGE;
	
    //defining offset
    int offset;
    offset=fHandle->curPagePos;
    //calling function 
    readBlock(offset,fHandle,memPage);
   //use ftell to set current file position to curPagePos 
    fHandle->curPagePos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;
	return RC_OK;
}



//implementing function 6 readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    //check for Null
    if (fopen(fHandle->fileName,"r")==NULL)
        return RC_FILE_NOT_FOUND;

    //defining offset
    int offset;
    offset=fHandle->curPagePos + 1;

    // calling function
    readBlock(offset,fHandle,memPage);

   //use ftell to set current file position to curPagePos 
    fHandle->curPagePos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;
	return RC_OK;
}


//implementing function 7 readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    //check for Null
    if (fopen(fHandle->fileName,"r")==NULL)
        return RC_FILE_NOT_FOUND;
    
    //defining offset
    int offset;
    offset=fHandle->totalNumPages - 1;
    //calling function
    readBlock(offset,fHandle,memPage);
   //use ftell to set current file position to curPagePos 
    fHandle->curPagePos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;
	return RC_OK;
}

extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if(fHandle->fileName==NULL)
		return RC_FILE_NOT_FOUND;
	
	int offset = pageNum * PAGE_SIZE;
	fp=fopen(fHandle->fileName,"r+");
	if(fp == NULL)//check if the write output withe the page is not 1 then throw an error saying the write failed
	{
		printf("Segmentation fault here\n");
		return RC_FILE_NOT_FOUND;
	}
		
	fseek(fp,offset,SEEK_SET);
 	if(fwrite(memPage,PAGE_SIZE,1,fp) != 1)//check if the write output withe the page is not 1 then throw an error saying the write failed
		return RC_WRITE_FAILED;

	offset = fHandle->curPagePos*PAGE_SIZE;
	//seeks to the particular page
	fseek(fp,offset,SEEK_SET);
	//returns the current pointer position of the file 
    fHandle->curPagePos = ftell(fp)/PAGE_SIZE;
    
 	fseek(fp,(fHandle->curPagePos)*PAGE_SIZE , SEEK_SET);
 	fHandle->mgmtInfo = fp;
	//Close fp
    fclose(fp);
	//return success code
    return RC_OK;
}

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if(fHandle->fileName==NULL)
		return RC_FILE_NOT_FOUND;
	int offset;
	offset = fHandle->curPagePos;
	fp = fopen(fHandle->fileName,"r+");  // Open the file in read + write mode
	fseek(fp, offset*PAGE_SIZE , SEEK_SET);
	if(fwrite(memPage,PAGE_SIZE,1,fp) != 1)// writes the data from the buffer to the file
		return RC_WRITE_FAILED;  // if output is -1 then its writing has failed 

	// set the offset value which returns the current pointer of block.
 	offset = (ftell(fHandle->mgmtInfo)/PAGE_SIZE);
	// seeks the pointer and writes the data to file
 	fseek(fp, offset*PAGE_SIZE , SEEK_SET);
 	fHandle->mgmtInfo = fp;
	fHandle->curPagePos = ftell(fp) / PAGE_SIZE; // Update current page position
	fclose(fp);
	// return success code
    return RC_OK;
}

RC appendEmptyBlock (SM_FileHandle *fHandle)
{
	int i;
	SM_PageHandle empty_page = (SM_PageHandle)malloc(sizeof(PAGE_SIZE));
	fp = fopen(fHandle->fileName,"r");
	if (fp == NULL)
		return RC_FILE_NOT_FOUND;
	fclose(fp);
	
	fp = fopen(fHandle->fileName,"a");  // Open the file in read + write mode

	//writes data to the empty page
	i = fwrite(empty_page, 1, PAGE_SIZE, fp);
	if (i != PAGE_SIZE)
		printf("%d\n",i);
	fHandle->mgmtInfo = fp;
	fHandle->totalNumPages = (ftell(fp)/PAGE_SIZE);
	
	fclose(fp);
	//return success code
	return RC_OK;
}


RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
	int offset;
	fp = fopen(fHandle->fileName,"r+");
	if(fp == NULL)
		return RC_FILE_NOT_FOUND;
	offset = fHandle->totalNumPages;
	if (offset < numberOfPages) //Check if total number of pages is less than number of pages
	{
		while (offset != numberOfPages) 
		{
			appendEmptyBlock(fHandle); // based on the if clause append the empty block
			offset += 1;
		}
	}
	fclose(fp);
	return RC_OK;
}