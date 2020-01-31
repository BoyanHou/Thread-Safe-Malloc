#define NDEBUG
#include "my_malloc.h" 
#include <stdio.h>

block* _free_list_head = NULL;
__thread block* t_free_list_head = NULL;
pthread_mutex_t lock;
pthread_mutex_t sbrk_lock;

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
//   return the part with higher address to be used
//   the part with lower address is still in the free list
block* split_free_block (block* block_ptr,
			 size_t required_bytes) {
  assert(block_ptr != NULL);
  assert(block_ptr->check_number == CHECK_NUMBER);
  block* higher_part = (block*)((size_t)block_ptr + block_ptr->size + sizeof(block) - required_bytes - sizeof(block));
  block* lower_part = block_ptr;
  // configure higher part
  higher_part->check_number = CHECK_NUMBER;
  higher_part->size = required_bytes;
  higher_part->prev_free = NULL;
  higher_part->next_free = NULL;
  // configure lower part
  lower_part->size = block_ptr->size - sizeof(block) - required_bytes;  
  return higher_part;
}

// Function Name: try_combine_with_next_free
// input(s):
//   ptr to ptr to free list's head block
//   block_ptr
// What it does:
//   if a block in the free list is physical neighbors with its
//   next_free block, combine them
void try_combine_with_next_free (block** free_list_head,
				 block* block_ptr) {
  assert(block_ptr != NULL);
  assert(block_ptr->check_number == CHECK_NUMBER);
  // w/ next free block
  if (block_ptr->next_free != NULL) {
    size_t addr = (size_t)(block_ptr->next_free) + block_ptr->next_free->size + sizeof(block); 
    if (addr == (size_t)block_ptr) {
      block_ptr->next_free->size += sizeof(block) + block_ptr->size;
      block_ptr->next_free->prev_free = block_ptr->prev_free;
      if (block_ptr->prev_free == NULL) {
	assert(block_ptr == *free_list_head);
	*free_list_head = block_ptr->next_free;
      } else {
	block_ptr->prev_free->next_free = block_ptr->next_free;
      }
    }
  }
  return;
}

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
				     size_t required_bytes) {
  block* block_for_use = NULL;
  if (record == NULL) {
    pthread_mutex_lock(&sbrk_lock); // lock sbrk
    block_for_use = sbrk(0);
    sbrk(sizeof(block) + required_bytes);
    pthread_mutex_unlock(&sbrk_lock);  // unlock sbrk
    total_size += sizeof(block) + required_bytes;
    block_for_use->size = required_bytes;
    block_for_use->check_number = CHECK_NUMBER;
    block_for_use->prev_free = NULL;
    block_for_use->next_free = NULL;
  } else {
    if (record->size >= sizeof(block) + required_bytes) {
      block_for_use = split_free_block(record,
    				       required_bytes);
    } else {
      //      disconnect the block from free list
      if (record->prev_free != NULL) {
	record->prev_free->next_free = record->next_free;
      } else {
	assert(*free_list_head == record);
	*free_list_head = record->next_free;
      }
      if (record->next_free != NULL) {
	record->next_free->prev_free = record->prev_free;
      }
      record->prev_free = NULL;
      record->next_free = NULL;
      block_for_use = record;
    }
    
  }
  void* data_ptr = (void*)((size_t)block_for_use + sizeof(block));
  return data_ptr;
}

// Function Name: common_free
// input(s):
//   pointer to the user data segment, of a block
// What it does:
//   get the block ptr from the data ptr
//   assert block is valid (!!SHOULD ALLOW free--free blocks)
//   loop through free list to insert the block into free list in address order
//   !THEN combine free neighbors 
void common_free (block** free_list_head,
		  void* user_data_ptr) {
  if (user_data_ptr == NULL) {
    return;
  }
  block* block_ptr = NULL;
  block_ptr = (block*)user_data_ptr - 1;
  assert(block_ptr->check_number == CHECK_NUMBER);
  if (*free_list_head == NULL) {
    *free_list_head = block_ptr;
  } else {  
    if ((size_t)(*free_list_head) < (size_t)block_ptr) { // block should be new head
      block_ptr->next_free = *free_list_head;
      (*free_list_head)->prev_free = block_ptr;
      *free_list_head = block_ptr;
    } else { // not suitable for head
      block* it = *free_list_head;
      // find position in free list
      while (it != NULL &&
	     it->next_free != NULL &&
	     (size_t)(it->next_free) >= (size_t)block_ptr) {
        it = it->next_free;
      }
      // insert block into free list
      block_ptr->next_free = it->next_free;
      if (it->next_free != NULL) {
	it->next_free->prev_free = block_ptr;
      }
      block_ptr->prev_free = it;
      it->next_free = block_ptr;
    }
  }
  if (block_ptr->prev_free != NULL) {
    try_combine_with_next_free(free_list_head, block_ptr->prev_free);
  }
  try_combine_with_next_free(free_list_head, block_ptr);
}

// Function Name: get_data_segment_size
// output(s):
//   (unsigend long) get total size, in bytes
// what it does:
//   record total(META_SIZE + block->size)
unsigned long get_data_segment_size() {
  return (unsigned long)total_size;
}

// Function Name: get_data_segment_free_space_size
// output(s):
//   (unsigned long) free block size + free block META_SIZE
// what it does:
//   iterate through the whole list, 
//     if is_free, record (block->size) AND  (META_SIZE)
unsigned long get_data_segment_free_space_size() {
  unsigned long space_record = 0;
  block* it = _free_list_head;
  while (it != NULL) {
    space_record += it->size + sizeof(block);
    it = it->next_free;
  }
  return space_record;
}

//////////////////////////
// BEST FIT
//////////////////////////

// Function Name: bf_malloc
// input(s):
//   required bytes of memory allocation
//   ptr to ptr to free list's head block
// output(s):
//   a pointer to the user data segment of the allocated block
// What it does:
//   Iterate start from the head block of the free list (free_head)
//   if (record==NULL OR that block's size is smaller than record's)
//      AND (block_size > required_bytes + META_SIZE)
//     record = that block
//   else if block_size >= required_bytes
//     record = that block
//     break

//   After the iteration, use:
//     iteration_record_post_process
//   to get the pointer to the ready-to-use user data segment
void* bf_malloc (size_t required_bytes, block** free_list_head) { 
  block* bf_record = NULL;
  block* it = *free_list_head;
  while (it != NULL) {
    if (it->size > required_bytes + sizeof(block) &&
	(bf_record == NULL || it->size < bf_record->size)) {
      bf_record = it;
    } else if (it->size >= required_bytes) {
      bf_record = it;
      break;
    }
    it = it->next_free;
  }
  void* allocation = iteration_record_post_process(free_list_head,
						   bf_record,
						   required_bytes);
  assert(allocation != NULL);
  return allocation;
}

// Function Name: bf_free
// input(s):
//   pointer to the user data segment of a block
// What it does:
//   free the designated block, using the method "common_free"
void bf_free (void *ptr, block** free_list_head) { 
  common_free(free_list_head, ptr);
}

////////////////////////////////
///
///  Thread Safe Malloc/Free
///       lock version
///
////////////////////////////////

// Function Name: ts_malloc_lock
// input(s): size of required space
// ouput(s): a pointer to a block's user space
// What it does:
//   Based on the bf_malloc (BEST FIT) function
//   lock before entering bf_malloc
//   unlock after bf_malloc
//   !! share the same lock with ts_free_lock
void *ts_malloc_lock(size_t size) {
  // critical section: lock
  pthread_mutex_lock(&lock);  
  void* ptr = bf_malloc(size, &_free_list_head);
  // critical section: unlock
  pthread_mutex_unlock(&lock);
  return ptr;
}

// Function Name: ts_free_lock
// input(s): a pointer to the user section of a block
// What it does:
//   based on the bf_free function
//   lock before entering bf_free
//   unlock after bf_free
//   !! share the same lock with ts_malloc_lock
void ts_free_lock(void *ptr) {
  // critical section: lock
  pthread_mutex_lock(&lock);
  bf_free(ptr, &_free_list_head);
  // critical section: unlock
  pthread_mutex_unlock(&lock);
  return;
}

////////////////////////////////
///
///  Thread Safe Malloc/Free
///     NO lock version: using TLS
///
////////////////////////////////

// Function Name: ts_malloc_nolock
// input(s): required space size
// output(s): a pointer to a block's user space
// What it does:
//   Based on malloc_ff()
//   Use Thread Local Storage to create individual linked-lists
//   for each thread
//   The difference is list head & tail are declared as "__thread" now 
void *ts_malloc_nolock(size_t size) {
  return bf_malloc(size, &t_free_list_head);
}

// Function Name: ts_free_nolock
// input(s): a pointer to the user section of a block
// What it does:
//   Based on bf_free()
//   Use TLS to create individual linked-lists for each thread
//   The difference is list head & tail are declared as "__thread" now
void ts_free_nolock(void *ptr) {
  bf_free(ptr, &t_free_list_head);
}

