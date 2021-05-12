								        BUFFER MANAGER
							-----------------------------------


CONTENTS OF THE FILE
--------------------
* Introduction
* Technologies
* Source files
* Functions
* Execution

INTRODUCTION
-------------
The buffer manager manages a fixed number of pages that are temporarily stored in memory, to minimize disk I/O. Buffer manager is able to 
handle more than one buffer pool for each page file simultaneously. Our program implements 4 replacement strategies: FIFO, LRU, LIFO and Clock 
alongside with storage manager for reading and writing blocks from the disk.

To know about detailed implementation of storage manager:
https://github.com/moezuh/Database_Storage_Manager


TECHNOLOGIES
-------------
Language used to implement Buffer manager is 'C'


SOURCE FILES
-------------
Below are the list of files needed.
C Files : buffer_mgr.c, buffer_mgr_stat.c, dberror.c, storage_mgr.c, test_assign2_1.c, readfile.c
Header files : buffer_mgr.h, buffer_mgr_stat.h, dberror.h, dt.h, storage_mgr.h, test_helper.h
Make fie


buffer_mgr.h
-------------
In this file two data structures BM_BufferPool and BM_PageHandle are defined. 
BM_BufferPool stores information about a buffer pool like name of the page, size of the buffer pool, page replacement strategy and a pointer that stores 
the page frames or data structures needed by the page replacement strategy to make replacement decisions.
BM_PageHandle stores information about a page like pagenumber, to know about the position of the page in the page file and a pointer to store the content 
of the page.

storage_mgr.h
--------------
In this file the two data structures are defined and all the functions to manipulate the files, read blocks from disc, writing blocks to a page file 
are declared.

db_error.h
-----------
In this file page, size is defined and all the error codes are defined. 
All the functions will return the integer code defined in the error codes. These values indicate whether an operation was successful or not.

dt.h
-----
In this file bool values are defined.

buffer_mgr_stat.h
------------------
This file contains functions used for outputting buffer or page content into a string or stdout.

buffer_mgr_stat.c
------------------
This file contains the implementations of all the functions that are defined in buffer_mgr_stat.h such as printPoolContent, sprintPoolContent, 
printPageContent, sprintPageContent and also printStrat

buffer_mgr.c
-------------
Most implementation of the buffer manager is present in this file. It contains a structure Frame, which contains all the necessary information about a frame
in the buffer pool i.e. dirty bit, fixCount, pagenumber in pagefile etc. All the necessary functions that are needed by the buffer manager such as initBufferPool,
shutdownBufferPool and forceFlushPool and page management functions such as markDirty, unpinPage, forcePage and pinPage and statistic functions such as getFrameContents,
getDirtyFlags, getFixCounts, getNumReadIO and getNumWriteIO and implementation of the page replacement strategies like LRU, Clock, FIFO and LFU.


FUNCTIONS
-----------

BufferPool Functions:
----------------------

initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData):
creates a buffer pool with size(number of frames) = numPages, assigns a page replacement strategy to buffer pool(passed as parameter) and initializes each frame's variables. 
First we reserve space for the buffer pool in memory and assign a pointer to that, into variable 'mgmtData'. Initially we set frame's pageNum to NO_PAGE (-1) and store name 
of the page file, whose pages are being cached in memory, to pageFileName. StratData can be used to pass additional parameters for the page replacement strategies like LRU-k 
(which we have not implemented). Once the buffer pool initializes successfully, program returns a success code: RC_OK.

shutdownBufferPool(BM_BufferPool *const bm):
This function has buffer manager struct as parameter. This function is used to destroy buffer pool i.e. it frees the memory we reserved for buffer pool. We will traverse through
the buffer pool and return a code RC_BUFFER_IN_USE_BY_CLIENT if page is in use by the client. Write back all the dirty pages to the disk before destroying
the buffer pool and free memory after making sure every dirty page in the page frames, were written back to the disk.

forceFlushPool(BM_BufferPool *const bm):
In this function, every dirty page in the buffer pool is written back to the page file, on disk. However, if the buffer manager is still in use i.e. there were pinned pages in the buffer 
pool, we return an error RC_BUFFER_IN_USE_BY_CLIENT.

Page Management Functions:
---------------------------

markDirty (BM_BufferPool *const bm, BM_PageHandle *const page):
Contains two structures as parameters. We will use pageNum to go to the provided page in the frame, and mark it as dirty.

unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page):
Contains two structures as paramters. We will use pageNum field to traverse through the pages in frames and reduce number of users i.e. fixCounts. If they are reduced to 0 after the
decrement, the page is marked as unpinned by making 'is_pinned' boolean variable to false.

forcePage (BM_BufferPool *const bm, BM_PageHandle *const page):
Contains two structures as parameters. Traverses to the page frame where page is stored in the pool, using pageNum field and checks if page, provided by the client was dirty or not.
If the page was not dirty, then program returns an error code 'RC_PAGE_WAS_NOT_MODIFIED'. If the page was actually dirty, program opens the pagefile and writes that page back to the
page file and then mark the page frame as not dirty.

pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum):
Used to pin a page with pageNum field. First, we check if the buffer manager already has the requested page. If yes, fixCount of that frame is incremented by 1 and that page is 
returned back to the client by passing the data of that frame into page data structure. If requested page was not found in the memory, we check for space in the buffer pool. If 
there is a space available, we read that page into available frame and return data of that frame to the client by storing its information into a page data structure. That frame is 
also pinned by setting its fixCount to 1. In case the requested page was not already cached and buffer pool was full, we call a page replacement strategy function, to free up the 
space in the buffer pool, so that we can make room for the requested page. Replacement strategy function frees a Frame, after which requested page is read from the disk and 
stored on that frame. This function also updates the variables used by replacement strategies, depending upon whether page was found in memory, or if there was a slot available for 
new page. These parameters include frame score (for LFU and LRU), reference bit (for Clock) and a Frame pointer (used by FIFO and Clock).

Statistic Functions:
---------------------

getFrameContents (BM_BufferPool *const bm):
Contains buffer pool struct as parameter. We declare PageNumber type of array (int), of size equal to number of frames in the buffer pool. We will fetch the page numbers of the
pages, that are stored in the buffer pool, store it into the array declared and return that array back to the user.

getDirtyFlags (BM_BufferPool *const bm):
Contains buffer pool struct as parameter. We declare a bool array - DirtyFlags - of size equal to number of frames in the buffer pool. We Traverse through the pool and assign the 
dirty bit status of each frame to the bool array. We then return the array of DirtyFlags to the user.

getFixCounts (BM_BufferPool *const bm):
Contains buffer pool struct as parameter. We declare int array FixCounts of size as number of frames in the buffer pool. Traverse through the buffer pool and store the value
of fixCount of the page stored in the page frames. We then return the array of FixCounts to the user.

getNumReadIO (BM_BufferPool *const bm):
Contains buffer pool struct as parameter. Counts the number of read IO's that were made by the Buffer manager, and returns the total back to the user.

getNumWriteIO (BM_BufferPool *const bm):
Contains buffer pool struct as parameter. Counts the number of write IO's that were made by the Buffer manager, and returns the total back to the user.

Page Replacement Strategies:
-----------------------------

LRU(BM_BufferPool *const bm, BM_PageHandle *page):
Contains buffer pool and page struct as parameters. Each frame has an LRU score associated with it, captured by the variable 'score'. This score ranges from 0 (being the lowest)
and (number of frames - 1), being the highest. Frame with highest score indicates that the page was most recently used and viceversa. So, we traverse through each frame in the buffer pool
to check which frame had the least score (hence being least recently used frame). Once that frame is found, we check if the page was modified while in buffer. If yes, write it back 
to the disk. If not, replace that page with the new page(requested by client). Since now, the frame in which we just replaced the page is most recently used frame, we assign highest 
score to it and decrement other frame's score by 1.

Clock(BM_BufferPool *const bm, BM_PageHandle *page):
Contains buffer pool and page struct as parameters. Clock replacement strategy uses a global variable Frameptr, which will point to 0th frame initially. Each page frame in the buffer pool
has a reference bit, which can be either 0 or 1. Frameptr points to a frame in the buffer pool, to keep track at which frame it currently is. We iterate through buffer pool, after the
frame pointer by frameptr, and stop on a frame with reference bit 0. We also set reference bits of frames with '1'to '0' on our way and increment Frameptr, as we go clockwise. Once we 
find our frame (with reference bit '0'), we check if that page inside that frame was dirty. If yes, it is written back to the disk using forcepage function. If not, we simply replace 
the page in that frame, by the requested page. We also set the reference bit of that frame to '1'.

FIFO(BM_BufferPool *const bm, BM_PageHandle *page):
Contains buffer pool and page struct as parameters. FIFO replacement strategy uses a global variable Frameptr, which will point to 0th frame initially. Frameptr 
points to a frame in the buffer pool, to keep track at which frame it currently is. Check if space is found in pool using spaceFound variable. We iterate till
space is available in pool and move to the next frame if the page is in use. Increment the Frameptr by 1 and if Frameptr value is greater than the size of the 
pool, set framePtr to 0. If page is not in use, find the Frame where page is to be replaced. we check if that page inside that frame was dirty. If yes, it is written back to the disk using forcepage function. If not, we simply replace 
the page in that frame, by the requested page. We also set the reference bit of that frame to '1'.

LFU(BM_BufferPool *const bm, BM_PageHandle *page):
Contains buffer pool and page struct as parameters.we check if that page inside that frame was dirty. Each frame has an LRU score associated with it, 
captured by the variable 'score'. This score ranges from 0 (being the lowest) and (number of frames - 1), being the highest. So, we traverse through each 
frame in the buffer poolto check which frame had the least score (hence being least recently used frame). Otherwise, Increment the Frameptr by 1 and if Frameptr 
value is greater than the size of the pool, set framePtr to 0. we check if that page inside that frame was dirty. If yes, it is written back to the disk 
using forcepage function. If not, we simply replace the page in that frame, by the requested page. We also set the reference bit of that frame to '1'.


EXECUTION
----------

make file is provided alongside the code.
- Open the commandline
- Make sure you are in the correct directory
- Run the below command to compile:
	make 
- Run the below command for execution:
	make run_test1
