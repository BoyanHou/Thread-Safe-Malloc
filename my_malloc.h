#include <unistd.h>
#include <assert.h>

#define TRUE 1
#define FALSE 0
#define META_SIZE 56 // should be manually set correctly
#define CHECK_NUMBER 650
#define PECK_NUMBER 8 // should be manually set correctly

// struct: meta data + user data
// blocks are stored as doubly linked list
typedef struct block block;


struct block {
  // meta data
  int is_free; // 0: free
  size_t size; // this block's size
  block* prev_free;
  block* next_free;
  block* prev;
  block* next;
  int check_number; // should always be 650
  
  // user data
  char user_data_first_byte[1]; // represents first byte of user data
};

// Function Name: is_valid_block
// input(s):
//   block ptr
// output(s):
//   FALSE = 0; TRUE = 1;
// What it does:
//   invalidate a block by checking the check_number
int is_valid_block(block* block_ptr);

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
int is_split_able (block* block_ptr, size_t required_bytes);

// Function Name: has head & has free_head
// output(s):
//   0; 1
// What it does:
//   check if the list has head (i.e. not NULL)
//              & free list has head
int has_head ();
int has_free_head ();

// Function Name: is_head & is_free_head
// input(s):
//   block ptr
// output(s):
//   0; 1
// What it does:
//   check if the block is list's head
//                    & is free list's head
int is_head (block* block_ptr);
int is_free_head (block* block_ptr);

// Function Name: free_list_substitute_block
// input(s):
//   old block ptr; new block ptr
// What is does:
//   subtitute old block with new one in the free block list
//   !ONLY deal with connection, not other block attributes
void free_list_substitute_block (block* old_block, block* new_block);

// Function Name: list_insert_before_block
// ! corner case--(making a single head) made secure
// input(s):
//   old block, new block
// What it does:
//   insert the new block in front of the old block, in list
void list_insert_before_block (block* old_block, block* new_block);

// Function Name: free_list_insert_before_block
// !corner case (make a single free head) made secure
// input(s):
//   old_block, new_block
// What it does:
//   assert both blocks are valid
//   assert both blocks are free
//   insert the new block in front of the old block, in free list
void free_list_insert_before_block (block* old_block,
				    block* new_block);


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
block* split_free_block (block* block_ptr, size_t required_bytes);

// Function Name: get_block_ptr
// input(s):
//   pointer to the user data part of that block
// output(s):
//   pointer to that block
// What it does:
//   assert (*(int* check_number = data_ptr - 6*8) == CHECK_NUMBER)
//   get the pointer of a block, by substracting an offset (META_SIZE)
//   from the address to the user data part of that block
block* get_block_ptr (void* user_data_ptr);

// Funtino Name: free_list_pull_block
// input(s):
//   block pointer
// What it does:
//   pull designated block out from the free list

// !! notice the logic: set block prev_free AND next_free to NULL at last!
void free_list_pull_block (block* block_ptr);

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
				   block* back_block);

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
block* combine_free_neighbors (block* block_ptr);

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
void* allocate_from_ready_block(block* block_ptr);

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
block* make_new_free_block(size_t required_bytes);

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
				     size_t required_bytes);

// Function Name: common_free
// input(s):
//   pointer to the user data segment, of a block
// What it does:
//   get the block ptr from the data ptr
//   assert block is valid (!!SHOULD ALLOW free--free blocks)
//   set block->is_free = TRUE
//   !FIRST insert this block before free_head
//   !THEN combine free neighbors 
void common_free (void* user_data_ptr);

// Function Name: get_data_segment_size
// output(s):
//   (unsigend long)the entire doubly-linked list size, in bytes
// what it does:
//   iterate through the whole list, record (META_SIZE + block->size)
unsigned long get_data_segment_size();

// Function Name: get_data_segment_free_space_size
// output(s):
//   (unsigned long) free block size + free block META_SIZE
// what it does:
//   iterate through the whole list, 
//     if is_free, record (block->size) AND  (META_SIZE)
unsigned long get_data_segment_free_space_size();

//////////////////////////
//  FIRST FIT
//////////////////////////

// Function Name: ff_malloc
// input(s):
//   required bytes of memory allocation
// output(s):
//   a pointer to the user data segment of the allocated block
// What it does:
//   Start from the head block of the free list (free_head)
//   Whenever a big enough block is found, do:
//     record that block and BREAK

//   After the iteration, use:
//     iteration_record_post_process
//   to get the pointer to the ready-to-use user data segment
void *ff_malloc(size_t required_bytes);

// Function Name: ff_free
// input(s):
//   pointer to the user data segment of a block
// What it does:
//   free the designated block, using the method "common_free"
void ff_free(void *ptr);

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
void* bf_malloc (size_t required_bytes);

// Function Name: bf_free
// input(s):
//   pointer to the user data segment of a block
// What it does:
//   free the designated block, using the method "common_free"
void bf_free (void *ptr);
