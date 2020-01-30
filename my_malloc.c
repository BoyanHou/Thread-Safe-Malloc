#define NDEBUG
#include "my_malloc.h" 

block* head = NULL;
block* free_head = NULL;

///////////////////////////////////////
// common methods for ff and bf:
///////////////////////////////////////

// Function Name: is_valid_block
// input(s):
//   block ptr
// output(s):
//   FALSE = 0; TRUE = 1;
// What it does:
//   invalidate a block by checking the check_number
int is_valid_block(block* block_ptr) {
  if (block_ptr == NULL ||
      block_ptr->check_number != CHECK_NUMBER) {
    return FALSE;
  } else {
    return TRUE;
  }
}

// Function Name: is_split_able
// input(s):
//   block ptr; 
//   required_bytes
// output:
//   FALSE = 0; TRUE =1
// What it does:
//   check if: block->size >= required_bytes + META_SIZE
//   if so: block split-able; return 1
//   otherwise: block un-split-able; return 0
int is_split_able (block* block_ptr, size_t required_bytes) {
  assert(is_valid_block(block_ptr));
  //  if (block_ptr->size >= 2*required_bytes + META_SIZE) {
  if (block_ptr->size >= required_bytes + META_SIZE) {
    return TRUE;
  } else {
    return FALSE;
  }
}

// Function Name: has head & has free_head
// output(s):
//   0; 1
// What it does:
//   check if the list has head (i.e. not NULL)
//              & free list has head
int has_head () {
  if (head) {
    return TRUE;
  } else {
    return FALSE;
  }
}
int has_free_head () {
  if (free_head) {
    return TRUE;
  } else {
    return FALSE;
  }
}

// Function Name: is_head & is_free_head
// input(s):
//   block ptr
// output(s):
//   0; 1
// What it does:
//   check if the block is list's head
//                    & is free list's head
int is_head (block* block_ptr) {
  assert(block_ptr != NULL);
  if (block_ptr == head) {
    return 1;
  } else {
    return 0;
  }
}
int is_free_head (block* block_ptr) {
  assert(block_ptr != NULL);
  if (block_ptr == free_head) {
    return TRUE;
  } else {
    return FALSE;
  }
}

// Function Name: free_list_substitute_block
// input(s):
//   old block ptr; new block ptr
// What is does:
//   subtitute old block with new one in the free block list
//   !ONLY deal with connection, not other block attributes
void free_list_substitute_block (block* old_block, block* new_block) {
  assert(is_valid_block(old_block));
  assert(old_block->is_free);
  assert(is_valid_block(new_block));
  assert(new_block->is_free);
  // prev_free block
  new_block->prev_free = old_block->prev_free;
  if (old_block->prev_free != NULL) {
    old_block->prev_free->next_free = new_block;
  } else {
    assert(is_free_head(old_block));
    free_head = new_block;
  }
  old_block->prev_free = NULL; 
  // next_free block
  new_block->next_free = old_block->next_free;
  if (old_block->next_free != NULL) {
    old_block->next_free->prev_free = new_block;
  }
  old_block->next_free = NULL;
}

// Function Name: list_insert_before_block
// ! corner case--(making a single head) made secure
// input(s):
//   old block, new block
// What it does:
//   insert the new block in front of the old block, in list
void list_insert_before_block (block* old_block, block* new_block) {
  // secure the case that makes a single head block,
  // i.e. old_block is passed as head && head is NULL
  if (old_block == NULL) {
    assert(head == NULL);
    assert(free_head == NULL);
    assert(is_valid_block(new_block));
    head = new_block;
    return;
  } else { // normal case 
    assert(is_valid_block(old_block));
    assert(is_valid_block(new_block));
    // prev block
    new_block->prev = old_block->prev;
    if (old_block->prev != NULL) {
      old_block->prev->next = new_block;
    } else {
      assert(is_head(old_block));
      head = new_block;
    }
    // between old & new block:
    new_block->next = old_block;
    old_block->prev = new_block;
    return;
  }
}

// Function Name: free_list_insert_before_block
// !corner case (make a single free head) made secure
// input(s):
//   old_block, new_block
// What it does:
//   assert both blocks are valid
//   assert both blocks are free
//   insert the new block in front of the old block, in free list
void free_list_insert_before_block (block* old_block,
				    block* new_block) {
  if (old_block == NULL) {
    assert(free_head == NULL);
    assert(is_valid_block(new_block));
    assert(new_block->is_free);
    free_head = new_block;
    return;
  } else {
    assert(is_valid_block(old_block));
    assert(is_valid_block(new_block));
    assert(old_block->is_free);
    assert(new_block->is_free);
    // configure prev_free block
    new_block->prev_free = old_block->prev_free;
    if (old_block->prev_free != NULL) {
      old_block->prev_free->next_free = new_block;
    } else {
      assert(is_free_head(old_block));
      free_head = new_block;
    }
    // between old & new block
    new_block->next_free = old_block;
    old_block->prev_free = new_block;
    return;
  }
}

// Function Name: split_free_block
// ! this functino ASSERTS the given block:
// !   has already been en-listed in both free list and list
// !   AND is free
// !   AND is_split_able
// input(s): 
//   block ptr; required bytes
// output(s):
//   block* original part:
// !   This origianl part is the one to be occupied
// !   The part is still left as free
// !   The part is still in the free list
// What it does:
//   assert:
//     block is (free) AND (split-able)
//   split the block into 2:
//     1.rest_part:
//         make a new block:
//           address = origin_address + META_SIZE + required_bytes,
//           free,
//           check_number = CHECK_NUMBER
//           size = META_SIZE + size(whole) - 2*META_SIZE - requried_bytes
//                = size(whole) - META_SIZE - required_bytes
//     2.original_part:
//         origin META header modified:
//           size = required_bytes
//           is_free = FALSE !!set this after substitution
//           !this is the part to be used

//   insert (rest_part) in front of (original_part) in the list
//   insert (rest_part) in front of (original_part) in the free list
block* split_free_block (block* block_ptr, size_t required_bytes) {
  assert(is_valid_block(block_ptr));
  assert(block_ptr->is_free);
  assert(is_split_able(block_ptr, required_bytes));
  block* rest_part = (block*)((size_t)block_ptr + META_SIZE + required_bytes);
  block* original_part = block_ptr;
  // configure rest_part
  rest_part->check_number = CHECK_NUMBER;
  rest_part->is_free = TRUE;
  rest_part->size = original_part->size - META_SIZE - required_bytes;
  // configure original_part
  original_part->size = required_bytes;
  // insert new block in front of old block in list
  list_insert_before_block(original_part, rest_part);
  // insert new block in front of old block in free list
  free_list_insert_before_block(original_part, rest_part);
  return original_part;
}

// Function Name: get_block_ptr
// input(s):
//   pointer to the user data part of that block
// output(s):
//   pointer to that block
// What it does:
//   assert (*(int* check_number = data_ptr - 6*8) == CHECK_NUMBER)
//   get the pointer of a block, by substracting an offset (META_SIZE)
//   from the address to the user data part of that block
block* get_block_ptr (void* user_data_ptr) {
  assert(user_data_ptr != NULL);
  assert(*((int*)((size_t)user_data_ptr - PECK_NUMBER)) == CHECK_NUMBER);
  block* block_ptr = (block*)((size_t)user_data_ptr - META_SIZE);
  assert(is_valid_block(block_ptr));
  return block_ptr;
}

// Funtino Name: free_list_pull_block
// input(s):
//   block pointer
// What it does:
//   pull designated block out from the free list

// !! notice the logic: set block prev_free AND next_free to NULL at last!
void free_list_pull_block (block* block_ptr) {
  assert(is_valid_block(block_ptr));
  // prev_free block
  if (block_ptr->prev_free != NULL) {
    assert(is_valid_block(block_ptr->prev_free));
    block_ptr->prev_free->next_free = block_ptr->next_free;
  } else {
    assert(is_free_head(block_ptr));
    free_head = block_ptr->next_free;
  }
  // next_free block
  if (block_ptr->next_free != NULL) {
    assert(is_valid_block(block_ptr->next_free));
    block_ptr->next_free->prev_free = block_ptr->prev_free;
  }
  block_ptr->prev_free = NULL;
  block_ptr->next_free = NULL;
}

// Function Name: combine_two_free_neighbors
// input(s):
//   front_block; back_block
// output(s):
//   The result_block of the combinition
// What it does:
// combine two free list-neighbor blocks, in both list and free list 
//   assert the two inputs are valid blocks
//   assert the two inputs are free
//   assert the back_block's address = front_block - META_SIZE - back_block->size

//   configure the result_block:
//     address = back_block
//     size = back_block->size + front_block->size + META_SIZE
//   configure prev block in list
//   !!Chances are that font and back blocks are not neighbors in free list
//   configure in free list:
//     keep back_block as-is
//     pull front_bock from the free list
block* combine_two_free_neighbors (block* front_block,
				 block* back_block) {
  assert(is_valid_block(front_block));
  assert(is_valid_block(back_block));
  assert(front_block->is_free);
  assert(back_block->is_free);
  // assert two blocks are list-neighbors
  assert(back_block->prev == front_block &&
	 front_block->next == back_block);
  // configure the resulting block:
  block* result_block = back_block;
  result_block->size = back_block->size + front_block->size + META_SIZE;
  // configure prev block in list
  result_block->prev = front_block->prev;
  if (front_block->prev != NULL) {
    assert(is_valid_block(front_block->prev));
    front_block->prev->next = result_block;
  } else {
    assert(is_head(front_block));
    head = result_block;
  }
  // don't need to configure next block in list
  // !!Chances are that font and back blocks are not neighbors in free list
  // keep back_block as-is, while pulling front_block out from free list
  free_list_pull_block(front_block);
  return result_block;
}

// Function Name: combine_free_neighbors
// input(s):
//   block_ptr
// output(s):
//   block_result
// What it does:
//   assert input block is free
//   for its prev block:
//     if it is free:
//       combine it with this block in listx
//       combine it with this block in free list
//   repeat for its next block;
block* combine_free_neighbors (block* block_ptr) {
  assert(is_valid_block(block_ptr));
  assert(block_ptr->is_free);
  // this block and its prev block
  if (block_ptr->prev != NULL && block_ptr->prev->is_free) {
    block_ptr = combine_two_free_neighbors(block_ptr->prev, block_ptr);
  }
  // this block and its next block
  if (block_ptr->next != NULL && block_ptr->next->is_free) {
    block_ptr = combine_two_free_neighbors(block_ptr, block_ptr->next);
  }
  return block_ptr;
}

// Function Name: allocate_from_ready_block
// ! this function assumes the block:
// !   has already been tried to splitted
// !   status is still FREE, here it will be set as NOT_FREE
// input(s):
//   block pointer
// output(s):
//   void* pointer the data segment of that block
// What it does:
//   assert block is valid
//   assert block is free
//   set block->is_free = FALSE
//   return data pointer = block + META_SIZE
void* allocate_from_ready_block(block* block_ptr) {
  assert(is_valid_block(block_ptr));
  assert(block_ptr->is_free);
  block_ptr->is_free = FALSE; 
  void* user_data_ptr = (void*)((size_t)(block_ptr) + META_SIZE);
  return user_data_ptr;
}

// Function Name: make_new_free_block
// ! The output block is UN-EN-LISTED in both list and free list
// input(s):
//   required bytes of allocation   
// output(s):
//   block pointer to an UN-FREE-LISTED free block
// What it does:
//   FIRST use sbrk(0) to get the the new block's address in heap
//   THEN use sbrk(META_SIZE + required_bytes) to move the break pointer up
//   configure the new free block:
//     is_free = TRUE
//     size = requried_bytes
//     check_number = CHECK_NUMBER
block* make_new_free_block(size_t required_bytes){
  block* block_ptr = sbrk(0);
  sbrk(META_SIZE + required_bytes);
  // configure the block:
  block_ptr->is_free = TRUE;
  block_ptr->size = required_bytes;
  block_ptr->check_number = CHECK_NUMBER;
  block_ptr->prev = NULL;
  block_ptr->next = NULL;
  block_ptr->prev_free = NULL;
  block_ptr->next_free = NULL;
  return block_ptr;
}

// Function Name: iteration_record_post_process
// input(s):
//   the suitable block record after iterating through free list;
//   required_bytes of allocation
// ouput(s):
//   a pointer to a ready-to-use user data segment
// What it does:
//   If no big enough block is found in the free list (i.e. record is NULL):
//     new_block = make_new_free_block
//     list_insert_before_block(head, new_block)
//     free_list_insert_before_block(free_head, new_block)

//   If the recorded block is split-able:
//     split_free_block;
//   free_list_pull_block;
//   allocate_from_ready_block
void* iteration_record_post_process (block* record,
				     size_t required_bytes) {
  if (record == NULL) {
    record = make_new_free_block(required_bytes);
    list_insert_before_block(head, record);
    free_list_insert_before_block(free_head, record);
  }
  assert(is_valid_block(record));
  assert(record->is_free);
  if (is_split_able(record, required_bytes)) {
    record = split_free_block(record, required_bytes);
  }
  free_list_pull_block(record);
  return allocate_from_ready_block(record);
}

// Function Name: common_free
// input(s):
//   pointer to the user data segment, of a block
// What it does:
//   get the block ptr from the data ptr
//   assert block is valid (!!SHOULD ALLOW free--free blocks)
//   set block->is_free = TRUE
//   !FIRST insert this block before free_head
//   !THEN combine free neighbors 
void common_free (void* user_data_ptr) {
  if (user_data_ptr == NULL) {
    return;
  }
  block* block_ptr = get_block_ptr(user_data_ptr);
  assert(is_valid_block(block_ptr));
  block_ptr->is_free = TRUE;
  free_list_insert_before_block(free_head, block_ptr);
  block_ptr = combine_free_neighbors(block_ptr);
  assert(is_valid_block(block_ptr));
}

// Function Name: get_data_segment_size
// output(s):
//   (unsigend long)the entire doubly-linked list size, in bytes
// what it does:
//   iterate through the whole list, record (META_SIZE + block->size)
unsigned long get_data_segment_size() {
  unsigned long space_record = 0;
  block* it = head;
  while (it != NULL) {
    space_record += META_SIZE + it->size;
    it = it->next;
  }
  return space_record;
}

// Function Name: get_data_segment_free_space_size
// output(s):
//   (unsigned long) free block size + free block META_SIZE
// what it does:
//   iterate through the whole list, 
//     if is_free, record (block->size) AND  (META_SIZE)
unsigned long get_data_segment_free_space_size() {
  unsigned long space_record = 0;
  block* it = head;
  while (it != NULL) {
    if (it->is_free == 1) {
      space_record += it->size;
      space_record += META_SIZE;
    }
    it = it->next;
  }
  return space_record;
}


//////////////////////////
// BEST FIT
//////////////////////////

// Function Name: bf_malloc
// input(s):
//   required bytes of memory allocation
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
void* bf_malloc (size_t required_bytes) { 
  block* bf_record = NULL;
  block* it = free_head;
  while (it != NULL) {
    if (it->size > required_bytes + META_SIZE &&
	(bf_record == NULL || it->size < bf_record->size)) {
      bf_record = it;
    } else if (it->size >= required_bytes) {
      bf_record = it;
      break;
    }
    it = it->next_free;
  }
  void* allocation = iteration_record_post_process(bf_record, required_bytes);
  assert(allocation != NULL);
  return allocation;
}

// Function Name: bf_free
// input(s):
//   pointer to the user data segment of a block
// What it does:
//   free the designated block, using the method "common_free"
void bf_free (void *ptr) { 
  common_free(ptr);
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
  void* ptr = bf_malloc(size);
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
  bf_free(ptr);
  // critical section: unlock
  pthread_mutex_unlock(&lock);
  return;
}
