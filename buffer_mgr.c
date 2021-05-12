#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>
#include "test_helper.h"


//prototypes for replacement strategies
extern void LRU(BM_BufferPool *const bm, BM_PageHandle *page);
extern void Clock(BM_BufferPool *const bm, BM_PageHandle *page);
extern void FIFO(BM_BufferPool *const bm, BM_PageHandle *page);
extern void LFU(BM_BufferPool *const bm, BM_PageHandle *page);
extern void displaycontents(BM_BufferPool *const bm);  // Helper function to display each frame's detail


// Define a pageframe using struct
struct Frame 
{
    BM_PageHandle page;  // contains page content and position of the page inside pagefile
    bool is_Dirty;
    bool is_pinned;
    int fixCount;
    int readCount;
    int writeCount;
    int score; // used by LRU and LFU
    int ref_bit; // used by clock
};
typedef struct Frame PageFrames;


// Global variable
int Frameptr = 0; // Frameptr will point to 0th frame initially -- Used by FIFO and Clock


// Function definitions
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    PageFrames *pool = (PageFrames *)malloc(sizeof(PageFrames) * numPages); // Initialize bufferpool in memory
    bm->pageFile = (char *const)pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    int i;
    for (i = 0; i < numPages; i++)
    {
        pool[i].is_Dirty = false;
        pool[i].is_pinned = false;
        pool[i].fixCount = 0;
        pool[i].page.pageNum = NO_PAGE; // store NO_PAGE (-1) initially
        pool[i].page.data = NULL;
        pool[i].readCount = 0;
        pool[i].writeCount = 0;
        pool[i].score = 0;
        pool[i].ref_bit = 0;
    }

    bm->mgmtData = (PageFrames *)pool; // Store memory pointer to pool in mgmtData
    printf("buffer manager has been initialized\n");
    
    return RC_OK;
}


RC shutdownBufferPool(BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int i;
    for(i = 0; i < bm->numPages; i++)
    {
        if (pool[i].is_pinned == true)
            return RC_BUFFER_IN_USE_BY_CLIENT; // return error if page is in use by a client

        if (pool[i].is_Dirty == true)
            forcePage(bm, &pool[i].page); // call forcepage to write back, if the page is dirty
    }
    free(pool); // free memory after everything is written on disk
    return RC_OK;
}


RC forceFlushPool(BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    SM_FileHandle fh;
    SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
    openPageFile (bm->pageFile, &fh);
    int i;
    for(i = 0; i < bm->numPages; i++)
    {
        if (pool[i].is_pinned == true || pool[i].fixCount > 0)
            return RC_BUFFER_IN_USE_BY_CLIENT;

        else if (pool[i].is_Dirty == true)
        {
            ph = pool[i].page.data;
            writeBlock (pool[i].page.pageNum, &fh, ph);
            readPreviousBlock(&fh, ph);
            pool[i].is_Dirty = false;
            pool[i].writeCount += 1;
        }
    }
    return RC_OK;
}


RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int i;
    for(i = 0; i < bm->numPages; i++)
    {
        if (pool[i].page.pageNum == page->pageNum)
        {
            pool[i].is_Dirty = true;
            break;
        }
    }
    return RC_OK;
}


RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int i;
    for(i = 0; i < bm->numPages; i++)
    {
        if (pool[i].page.pageNum == page->pageNum)
        {
            pool[i].fixCount -= 1;
            if (pool[i].fixCount == 0)
                pool[i].is_pinned = false;
            break;
        }
    }
    return RC_OK;
}


RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    SM_FileHandle fh;
    SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
    openPageFile (bm->pageFile, &fh);
    int i;
    for(i = 0; i < bm->numPages; i++)
    {
        if (pool[i].page.pageNum == page->pageNum)
        {
            if (pool[i].is_Dirty == false) 
                return RC_PAGE_WAS_NOT_MODIFIED;  // return error if page remained unchanged while in buffer
            else
            {
                ph = pool[i].page.data;
                writeBlock(pool[i].page.pageNum, &fh, ph);
                pool[i].is_Dirty = false;
                pool[i].writeCount += 1;
                break;
            }
        }
    }
    return RC_OK;
}


RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    // Check if buffer manager already has the requested page
    int i, index;
    bool pageFound = false;
    bool spaceFound = false;
    for (i = 0; i < bm->numPages; i++)
    {
        if (pool[i].page.pageNum == pageNum) // Found requested page in buffer pool
        {
            index = i;
            pool[i].fixCount += 1;  // increase fixCount of that frame
            pool[i].is_pinned = true;
            pageFound = true;
            break;
        }
    }
    
    if (!pageFound) // If page not found in the pool, check for empty frame
    {
        //Check for the space in bufferpool
        for (i = 0; i < bm->numPages; i++)
        {
            if (pool[i].page.pageNum == NO_PAGE) // Found empty frame
            {
                SM_FileHandle fh;  
                SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
                openPageFile (bm->pageFile, &fh);
                
                if (pageNum >= fh.totalNumPages)
                {
                    appendEmptyBlock(&fh);
                }
                    
                readBlock(pageNum, &fh, ph);  // read page from disk into the frame
                pool[i].page.data = ph;
                pool[i].page.pageNum = pageNum;
                pool[i].readCount += 1; // increase readCount
                pool[i].fixCount += 1;
                pool[i].is_pinned = true;
                pool[i].ref_bit = 1; // Set reference bit to 1 (used by clock alg)
                index = i; // Store index of frame
                spaceFound = true;
                break;
            }
        }
    }
    
    if (pageFound && !spaceFound) // Update scores and reference bit of frames
    {
        if (bm->strategy == RS_LRU)
        {
            for (i = 0; i < bm->numPages; i++)
            {
                if (pool[i].score >= pool[index].score) // Decrease score of frames which had greater scores
                    pool[i].score -= 1;
            }
            pool[index].score = bm->numPages - 1; // Assign current pageframe highest LRU score
        }
        else if (bm->strategy == RS_CLOCK)
        {
            pool[index].ref_bit = 1; // Set reference bit to 1 (for Clock)
            Frameptr += 1; // move pointer to next frame
            if (Frameptr >= bm->numPages)
                Frameptr = 0;
        }
        else if (bm->strategy == RS_LFU)
            pool[index].score += 1; // Increase the score if requested page was a hit
    }
    else if (spaceFound && !pageFound) // decrement scores of existing pages in the frame
    {
        if (bm->strategy == RS_LRU)
        {
            for (i = 0; i < bm->numPages; i++)
            {
                if (pool[i].page.pageNum != NO_PAGE) // Decrease score of all frames in which data exists
                    pool[i].score -= 1;
            }
            pool[index].score = bm->numPages - 1; // Assign pageframe highest LRU score
        }
        else if (bm->strategy == RS_CLOCK || bm->strategy == RS_FIFO)
            Frameptr = index;  // Set Frame pointer to this frame
            
        else if (bm->strategy == RS_LFU)
        {
            pool[index].score = 1; // Set score for 
            Frameptr = index;  // Set Frame pointer to this frame
        }
    }
    else if (!spaceFound && !pageFound) // If requested page is not in buffer and there is no space in the pool, replace an existing page using a strategy
    {
        SM_FileHandle fh;  
        SM_PageHandle ph = (SM_PageHandle) malloc(PAGE_SIZE);
        openPageFile (bm->pageFile, &fh);
        if (pageNum >= fh.totalNumPages)
        {
            appendEmptyBlock(&fh);
        }
        readBlock(pageNum, &fh, ph);  // read page from disk into the tempframe
        page->data = ph;
        page->pageNum = pageNum;
        switch(bm->strategy) // 
        {			
            case RS_LRU: // Using LRU algorithm
                LRU(bm, page);
                return RC_OK;
            
            case RS_CLOCK:
                Clock(bm, page);
                return RC_OK;

            case RS_FIFO:
                FIFO(bm, page);
                return RC_OK;

            case RS_LFU:
                LFU(bm, page);
                return RC_OK;

            default:
                printf("\nAlgorithm Not Implemented\n");
                break;
        }
    }

    //Store the information into page which is used by the client
    
    page->pageNum = pageNum;
    page->data = pool[index].page.data;
    return RC_OK;
}


PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    PageNumber *PageNumbers = (PageNumber *)malloc(sizeof(bm->numPages)); // declare a PageNumber type array of size = number of frames in bufferpool
    int i;
    for (i = 0; i < bm->numPages; i++)
    {
        PageNumbers[i] = pool[i].page.pageNum;
    }
    return PageNumbers;
}


bool *getDirtyFlags (BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    bool *DirtyFlags = (bool *)malloc(sizeof(bm->numPages)); // declare a boolean array of size = number of frames in buffer
    int i;
    for (i = 0; i < bm->numPages; i++)
    {
        DirtyFlags[i] = pool[i].is_Dirty;
    }
    
    return DirtyFlags;
}


int *getFixCounts (BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int *FixCounts = (int *)malloc(sizeof(bm->numPages)); // declare a integer array of size = number of frames in buffer
    int i;
    for (i = 0; i < bm->numPages; i++)
    {
        FixCounts[i] = pool[i].fixCount;
    }

    return FixCounts;
}


int getNumReadIO (BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int NumReadIO = 0;
    int i;
    for (i = 0; i < bm->numPages; i++)
    {
        NumReadIO += pool[i].readCount;  // Add readCount for each frame
    }

    return NumReadIO;
}


int getNumWriteIO (BM_BufferPool *const bm)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int NumWriteIO = 0;
    int i;
    for (i = 0; i < bm->numPages; i++)
    {
        NumWriteIO += pool[i].writeCount;  // Add writeCount for each frame
    }

    return NumWriteIO;
}


extern void LRU(BM_BufferPool *const bm, BM_PageHandle *page)
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int i, index, replace_score = 0; // score of least recently used frame would be zero
    bool spaceFound = false;
    while (!spaceFound)
    {
        for (i = 0; i < bm->numPages; i++)
        {
            if(pool[i].score == replace_score) // Find frame with least score
            {
                if (pool[i].is_pinned == true || pool[i].fixCount > 0) // if page in the frame is in use
                {
                    replace_score += 1; // move to next least recently used frame
                    break;
                }
                else
                {
                    spaceFound = true; // break for and while loop and store the index of this frame
                    index = i;
                    break;
                }
            }
        }
    }

    // Check if the page was modified in this frame
    if(pool[index].is_Dirty == true) 
        forcePage(bm, &pool[i].page); // write page onto the disk

    // Replace with new page information
    pool[index].page.data = page->data;
    pool[index].page.pageNum = page->pageNum;
    pool[index].is_pinned = true;
    pool[index].fixCount = 1;
    pool[index].is_Dirty = false;
    pool[index].readCount += 1;
    for(i = 0; i < bm->numPages; i++)
    {
        if (pool[i].score >= pool[index].score) // Decrease score of frames which had greater scores
            pool[i].score -= 1;
    }
    pool[index].score = bm->numPages - 1; // assign highest score as it will be the most recently used frame

}


extern void Clock(BM_BufferPool *const bm, BM_PageHandle *page) // Need to include checks for fixCount
{
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    Frameptr += 1;

    if (Frameptr == bm->numPages) // Set Frameptr to 0 if it moves past last frame (to move in circle)
            Frameptr = 0;

    while(1)
    {
        if(pool[Frameptr].ref_bit == 0) // if reference bit was 0 (frame to be replaced, found)
        {
            if (pool[Frameptr].is_Dirty == true) //check if the page is dirty
                forcePage(bm, &pool[Frameptr].page); // Write page back to the disk

            break; // break from while loop
        }
            
        else // if reference bit was 1
        {
            pool[Frameptr].ref_bit = 0;
            Frameptr += 1;
            if (Frameptr == bm->numPages) // Set Frameptr to 0 if it reaches last frame (to move in circle)
                Frameptr = 0;
        }
    }

    //Write data into the frame
    pool[Frameptr].page.data = page->data;
    pool[Frameptr].page.pageNum = page->pageNum;
    pool[Frameptr].is_pinned = true;
    pool[Frameptr].is_Dirty = false;
    pool[Frameptr].fixCount = 1;
    pool[Frameptr].readCount += 1;
    pool[Frameptr].ref_bit = 1;
}

/*defining function FIFO*/ 
extern void FIFO(BM_BufferPool *const bm, BM_PageHandle *page)
{
    Frameptr += 1;
    if (Frameptr >= bm->numPages)
        Frameptr = 0;
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    bool spaceFound = false;
    while (!spaceFound)
    {
        if (pool[Frameptr].is_pinned == true) // Move to next frame if page is in use
        {
            Frameptr += 1;
            if (Frameptr >= bm->numPages)
                Frameptr = 0;
        }
        
        else  // Page not in use
            spaceFound = true; // Found the Frame where page is to be replaced
    }
    
    if (pool[Frameptr].is_Dirty)  //check if the page is dirty
        forcePage(bm, &pool[Frameptr].page);  // Write page back to the disk

    pool[Frameptr].page.data = page->data;
    pool[Frameptr].page.pageNum = page->pageNum;
    pool[Frameptr].is_pinned = true;
    pool[Frameptr].is_Dirty = false;
    pool[Frameptr].fixCount = 1;
    pool[Frameptr].readCount += 1;
    pool[Frameptr].ref_bit = 1;
}



extern void LFU(BM_BufferPool *const bm, BM_PageHandle *page)
{
    int replace_score = 1,index; 
    Frameptr += 1;
    if (Frameptr >= bm->numPages)
        Frameptr = 0;
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    index = Frameptr;
    while (1)
    {
        if (pool[Frameptr].score == replace_score) // Least frequent score
            break; // Found the Frame where page is to be replaced
        else  // Page not in use
        {
            Frameptr += 1;
            if (Frameptr >= bm->numPages)
                Frameptr = 0;
        }   
        if (Frameptr == index) // If Frameptr reaches original frame, increase replace score
        {
            replace_score += 1;
            Frameptr += 1;
            if (Frameptr >= bm->numPages)
            Frameptr = 0;
        }
    }
    
    if (pool[Frameptr].is_Dirty)  //check if the page is dirty
        forcePage(bm, &pool[Frameptr].page);  // Write page back to the disk

    pool[Frameptr].page.data = page->data;
    pool[Frameptr].page.pageNum = page->pageNum;
    pool[Frameptr].is_pinned = true;
    pool[Frameptr].is_Dirty = false;
    pool[Frameptr].fixCount = 1;
    pool[Frameptr].readCount += 1;
    pool[Frameptr].score = 1;
}

extern void displaycontents(BM_BufferPool *const bm)
{
    printf("\n\nBufferpool's information:\n");
    printf("Number of frames: %d\n",bm->numPages);
    printf("Page file: %s\n",bm->pageFile);
    printf("Strategy: %d\n",bm->strategy);
    PageFrames *pool = (PageFrames *)bm->mgmtData;
    int i;
    for (i = 0; i < bm->numPages; i++)
    {
        printf("\nFrame: %d\n", i);
        printf("pagenum: %d\n", pool[i].page.pageNum);
        printf("page content: %s\n",pool[i].page.data);
        printf("is_dirty: %d\n",pool[i].is_Dirty);
        printf("is_pinned: %d\n",pool[i].is_pinned);
        printf("fixCount: %d\n",pool[i].fixCount);
        printf("readCount: %d\n",pool[i].readCount);
        printf("writeCount: %d\n",pool[i].writeCount);
        printf("score: %d\n",pool[i].score);
        printf("ref_bit: %d\n",pool[i].ref_bit);
        printf("Frame pointer value %d\n", Frameptr);
    }
}