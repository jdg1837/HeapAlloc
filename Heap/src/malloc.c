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
int k = 0;
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
struct block *MostRecent = NULL; /*tracks the last block used, for next fit*/ 
int next_failed = 1; /*tracks if next fit failed*/

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free blocks
 * \param size size of the block needed in bytes 
 *
 * \return a block that fits the request or NULL if no free block matches
 *
 */
struct block *findFreeBlock(struct block **last, size_t size) 
{
   struct block *curr = FreeList;
   
   if(curr == NULL)
   {
      return curr;
   }

#if defined FIT && FIT == 0
   /* First fit
    * check all blocks in list. If one works, take it and break
    * Mark last as each, so if none is found, we grow heap
    * after the actual last block*/
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0

   struct block *best = NULL;
   size_t min = -1;

   while(curr)
   {
      if(curr->free && (curr->size >= size))
      {
         if((min = -1) || (curr->size < min))
         {
            best = curr;
            min = curr->size;
         }
      }

      *last = curr;
      curr  = curr->next;
   }
   
   curr = best;
   
#endif

#if defined WORST && WORST == 0

   struct block *worst = NULL;
   size_t max = -1;

   while(curr)
   {
      if(curr->free && (curr->size >= size))
      {
         if((max = -1) || (curr->size > max))
         {
            worst = curr;
            max = curr->size;
         }
      }

      *last = curr;
      curr  = curr->next;
   }
   
   curr = worst;

#endif

#if defined NEXT && NEXT == 0

   if(MostRecent == NULL)
   {
      MostRecent = FreeList;
   }

   curr = MostRecent;
   

   int been_here = 0;

   while (!(curr->free && curr->size >= size)) 
   {
      if(curr == MostRecent)
      {
         if(been_here == 0)
         {
            been_here = 1;
         }
         else
         {
            curr = NULL;
            next_failed = 1;
            break;
         }
      }

      if(curr->next != NULL)
      {
         curr  = curr->next;
      }

      else
      {
         *last = curr;
         curr  = FreeList;
      }

   }      

   if(next_failed != 1)
   {
      MostRecent = curr;
   }
   
#endif

   if(curr != NULL)
   {
      num_reuses++;
   }

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

   num_grows++;
   num_blocks++;
   max_heap += size;

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

   num_requested += size;

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

   if(next != NULL && next->size > size)
   {
      struct block *split = (struct block *)((size_t) next + sizeof(struct block) + size);

      split->size = next->size - sizeof(struct block) - size;
      split->next = next->next;
      split->free = true;
      next->size = size;
      next->next = split;
      next->free = false;
      num_splits++;
      num_blocks++;
   }
   
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
   struct block *freed = BLOCK_HEADER(ptr);
   assert(freed->free == 0);
   freed->free = true;

   num_frees++;

   /* TODO: Coalesce free blocks if needed */

   struct block *curr = FreeList;

   while(curr->next != NULL)
   {
      if( (curr->free == true) && (curr->next->free == true) )
      {
         struct block *merger = curr->next;
         if(MostRecent == merger)
         	MostRecent = curr;
         size_t new_size = curr->size + merger->size;
         curr->size = new_size;
         curr->next = merger->next;
         num_coalesces++;
         num_blocks++;
      }

      else
      {
         curr = curr->next;
      }
   }
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/





