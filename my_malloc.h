#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0
#define CHECK_NUMBER 650

size_t total_size = 0;

// struct: meta data
// free blocks are stored as doubly linked list
typedef struct block block;

void pr();

struct block {
  // meta data
  size_t size; // user data's size
  block* prev_free;
  block* next_free;
  int check_number; // should always be 650
};

///////////////////////////////////////
// common methods for ff and bf:
///////////////////////////////////////

// Function Name: split_free_block
// input(s): 
//   ptr to ptr to free list head block;
//   block ptr;
//   required bytes;
// output(s):
//   block* original part:
// What it does:
//   split a block,
//   return the part with lower address as to be used
//   the part with higher address is still in the free list
block* split_free_block (block* block_ptr,
			 size_t required_bytes);

// Function Name: try_combine_with_next_free
// input(s):
//   ptr to ptr to free list's head block
//   block_ptr
// What it does:
//   if a block in the free list is physical neighbors with its
//   next_free block, combine them
void try_combine_with_next_free (block** free_list_head,
				 block* block_ptr);

// Function Name: iteration_record_post_process
// input(s):
//   ptr to ptr to free list's head block
//   the suitable block record after iterating through free list;
//   required_bytes of allocation
// ouput(s):
//   a pointer to a ready-to-use user data segment
// What it does:
//   If no big enough block is found in the free list (i.e. record is NULL):
//     use sbrk() to allocate a new block
//   Else: try to split the block, then return suitable data section ptr

void* iteration_record_post_process (block** free_list_head,
				     block* record,
				     size_t required_bytes);

// Function Name: common_free
// input(s):
//   pointer to the user data segment, of a block
// What it does:
//   get the block ptr from the data ptr
//   assert block is valid (!!SHOULD ALLOW free--free blocks)
//   loop through free list to insert the block into free list in address order
//   !THEN combine free neighbors 
void common_free (block** free_list_head,
		  void* user_data_ptr);

// Function Name: get_data_segment_size
// output(s):
//   (unsigend long) get total size, in bytes
// what it does:
//   record total(META_SIZE + block->size)
unsigned long get_data_segment_size();

// Function Name: get_data_segment_free_space_size
// output(s):
//   (unsigned long) free block size + free block META_SIZE
// what it does:
//   iterate through the whole list, 
//     if is_free, record (block->size) AND  (META_SIZE)
unsigned long get_data_segment_free_space_size();

//   After the iteration, use:
//     iteration_record_post_process
//   to get the pointer to the ready-to-use user data segment
void* bf_malloc (size_t required_bytes, block** free_list_head);

// Function Name: bf_free
// input(s):
//   pointer to the user data segment of a block
//   ptr to ptr to free list's head block
// What it does:
//   free the designated block, using the method "common_free"
void bf_free (void *ptr, block** free_list_head);


////////////////////////////////
///
///  Thread Safe Malloc/Free
///       lock version
///
////////////////////////////////
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr); 

////////////////////////////////
///
///  Thread Safe Malloc/Free
///       NO lock version
///
////////////////////////////////
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);
