#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

struct block *findprev(struct block **target);

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct block 
{
   size_t size;  /* Size of the allocated block of memory in bytes */
   struct block *next;  /* Pointer to the next block of allcated memory   */
   bool free;  /* Is this block free?                     */
};

struct block *FreeList = NULL; /* Free list to track the blocks available */
struct block *MostRecent = NULL; 
int next_failed = 1;

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free blocks
 * \param size size of the block needed in bytes 
 *
 * \return a block that fits the request or NULL if no free block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct block *findFreeBlock(struct block **last, size_t size) 
{
   struct block *curr = FreeList;
   if(curr == NULL)
   {
      return curr;
   }

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   
   size_t min = curr->size;
   int found = 0;
   int looped = 0;

   while(1)
   {
      if(curr->free && (curr->size >= size))
      {
         found = 1;

         if( (looped == 1) && (curr->size == min) )
         {
            break;
         }

         if(curr->size < min)
         {
            min = curr->size;
         }
      }

      if(curr->next != NULL)
      {
         curr  = curr->next;
      }

      else
      {
         *last = curr;
         
         if(found == 0 || looped ==2)
         {
            curr = NULL;
            break;
         }

         else
         {  
            looped = 1;
            curr  = FreeList;            
         }
      }

   }
#endif

#if defined WORST && WORST == 0
   size_t max = curr->size;
   int found = 0;
   int looped = 0;

   while(1)
   {
      if(curr->free && (curr->size >= size))
      {
         found = 1;

         if( (looped == 1) && (curr->size == max) )
         {
            break;
         }

         if(curr->size > max)
         {
            max = curr->size;
         }
      }

      if(curr->next != NULL)
      {
         curr  = curr->next;
      }

      else
      {
         *last = curr;
         
         if(found == 0 || looped ==2)
         {
            curr = NULL;
            break;
         }

         else
         {  looped = 1;
            curr  = FreeList;            
         }
      }

   }
#endif

#if defined NEXT && NEXT == 0
   if(MostRecent == NULL)
   {
      MostRecent = FreeList;
   }

   curr = MostRecent;

   while (curr && !(curr->free && curr->size >= size)) 
   {
      if(curr->next != NULL)
      {
         curr  = curr->next;
      }

      else
      {
         *last = curr;
         curr  = FreeList;
      }

      if(curr == MostRecent)
      {
         curr = NULL;
         next_failed = 1;
         break;
	}      
   }

   if(next_failed != 1)
   {
      MostRecent = curr;
   }
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated block of NULL if failed
 */
struct block *growHeap(struct block *last, size_t size) 
{
   /* Request more space from OS */
   struct block *curr = (struct block *)sbrk(0);
   struct block *prev = (struct block *)sbrk(sizeof(struct block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct block *)-1) 
   {
      return NULL;
   }

   /* Update FreeList if not set */
   if (FreeList == NULL) 
   {
      FreeList = curr;
   }

   if(next_failed == 1)
   {
      MostRecent = curr;
      next_failed = 0;
   }

   /* Attach new block to prev block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free block of heap memory for the calling process.
 * if there is no free block that satisfies the request then grows the 
 * heap and returns a new block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free block */
   struct block *last = FreeList;
   struct block *next = findFreeBlock(&last, size);

/*   if(next != NULL && next->size > size)
   {
	struct block *split = NULL;
	next->size = size;
	split->size = next->size - size;
	split->next = next->next;
	next->next = split;
	num_splits++;
   }
*/
   /* Could not find free block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
      num_grows++;
   }

   /* Could not find free block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   else
   {
      num_mallocs++;
      /* Mark block as in use */
      next->free = false;

      /* Return data address associated with block */
      return BLOCK_DATA(next);   
   }
}

/*
 * \brief free
 *
 * frees the memory block pointed to by pointer. if the block is adjacent
 * to another block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   /* Make block as free */
   struct block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   num_frees++;

   /* TODO: Coalesce free blocks if needed */
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/





