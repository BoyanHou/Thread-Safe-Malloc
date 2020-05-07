# BoyanHou_bh214_ECE650PRJ1_Report



## 1  Project Overview

This project aimed at implementing the "malloc" and "free" functions. This was done using two different allocation policies (First Fit and Best Fit), with the performance of the two observed, analyzed and compared to each other.

The following content would cover: the implementation detail, result analysis and conclusion of the project.



## 2 Implementation 

The memory in heap to be allocated was organized into blocks in a doubly linked list. The First Fit and Best Fit malloc functions were both developed based on the same chain of tool functions as the only difference between them is their free-block finding policy. The following parts make further explanation on the block, the doubly-linked list, the functions and the big picture of the memory allocating/ freeing process.

### 2.1 Block Description

A block is the basic unit of memory allocation and is implemented using a C struct. Each block is divided into two sections: the Meta Data section for recording the key information of each and User Data Section.

There are seven fields for Meta Data: 

| filed name   | type           | responsibility                                               |
| ------------ | -------------- | ------------------------------------------------------------ |
| is_free      | integer        | 1 indicates the block is free, 0 for otherwise               |
| size         | size_t         | describes the size of the User Data Section in this block (in bytes) |
| prev_free    | struct pointer | block pointer, points at the previous free block             |
| next_free    | struct pointer | block pointer, points at the next free block                 |
| prev         | struct pointer | block pointer , points at the previous block                 |
| next         | struct pointer | block pointer, points at the next block                      |
| check number | integer        | a fixed number, set to check whether the block was allocated by this program or not |

There is also a virtual field, called "user_data_first_byte", which isn't really used for storing any Meta Data, but to conveniently get a pointer to the first byte of the user space in each block.

From the prospective of heap address, the Meta Data section sits "below" the User Data section in each block in the heap, meaning its address is always smaller than the user data's in the same block. This comes more convenient when initially allocating the memory in heap with `sbrk()` because `sbrk(0)` would return the address where the heap break pointer is currently at. This will be used as the address of the block pointer and all other meta data will be built upon that, then finally we have additional space on top of the Meta Data that can be used by the user (which is pointed by the `user_data_first_byte` pointer). 

### 2.2 Doubly Linked List

There are two doubly linked list in this implementation:

The basic one is the list of all the blocks we have built on top of each other in the heap using `sbrk()`. In this program, the recorded `head` pointer is designed to always to point at the the top block (i.e. the one that was built most recently and has the largest address in the heap). Following by their `next` pointer, we can go all the way down to the very first block allocated in the beginning, and `prev` pointer would supports this operation in the reverse order.

Although being the trickier one, the Free List, indicated by the `prev` and `next` pointer in each block, helps a lot to boost the speed of searching for a suitable free block to allocate from. Without keeping track of this list of free blocks, every search have to go through both free and not-free blocks to find the target. (Note that although there is a visible improvement in actual speed by using the free list, the time complexity is still O(n), meaning that this is not a stable method of improving efficiency and could lead to the same run time in the worst case. The author's thought on this is maybe instead of using a linear, un-ordered list, other data structures, such as a Min-Heap, could be used.) Unlike the basic list in this implementation, this free list of blocks does not have any specific order in their size or address, but rather just organized by the order they are added into the free list during runtime.

### 2.3 Functions

Following the Single Responsibility Principle of software development, the required functions were broken down to more than twenty different pieces, each with a unique responsibility. See below for the table of all functions used in this project and their details.

| Function Name                    | input                                      | output                                        | Description                                                  |
| -------------------------------- | ------------------------------------------ | --------------------------------------------- | ------------------------------------------------------------ |
| is_valid_block                   | block pointer                              | 1/0                                           | judge whether or not  a given block is a valid one allocated by this program |
| is_split_able                    | block pointer, required user space size    | 1/0                                           | judge whether or not a given block is a free, split-able one |
| has_head/ has_free_head          |                                            | 1/0                                           | judge whether there is a list head (free list head) or not   |
| is_head/ is_free_head            | block pointer                              | 1/0                                           | judge whether the given block is the head of the list (free list) or not |
| free_list_substitute_block       | old & new block pointer                    |                                               | substitute the position of an old block with a new block in the free list |
| list_insert_before_block         | old & new block pointer                    |                                               | insert a new block in front of the old one in the list       |
| free_list_insert_before_block    | old & new block pointer                    |                                               | insert a new block in front of the old one in the  free list |
| split_free_block                 | block pointer, size of required user space | a pointer to one of the split result block    | split a free block, allocating META_SIZE + user required size for one of the two and return a pointer to it; the other one remains as a free block in the free list |
| get_block_ptr                    | pointer to a user space                    | a pointer to the related block                | apply an offset to the user space pointer to get the pointer to the related block |
| free_list_pull_block             | pointer to a block                         |                                               | pull a free block out from the free list (to allocate to the  user) |
| combine_two_free_neighbors       | two neighbor blocks                        | a pointer to the combined block               | combine two free blocks which are neighbors to each other in the list (but not necessarily neighboring each other in the free list) into one block |
| combine_free_neighbors           | pointer to a free block                    | a pointer to the combined block               | make use of the function "combine_two_free_neighbors": combine a block with potentially its previous block and next block in the list (if they are free) into one block |
| allocate_from_ready_block        | pointer to a free block                    | pointer to the free block's user section      | given a free block, first try to split (if is split-able); then get the user data section pointer from it |
| make_new_free_block              | required user space size                   | pointer to a free block                       | make a new free block on top of the current list in the heap |
| iteration_record_post_process    | pointer to a free block                    | pointer to its user section                   | this function catches whatever the iteration result is from either bf_malloc or ff_malloc, processing it and gives out a valid pointer to the user section for use |
| common_free                      | pointer to a user section                  |                                               | convert the status of a free block into not-free             |
| get_data_segment_size            |                                            | the size of the whole current list (in bytes) | loop through the whole list and record the space occupied by all blocks' Meta Data and User Data |
| get_data_segment_free_space_size |                                            | the size of all free blocks (in bytes)        | loop through the whole list and record the space occupied by all free blocks' Meta Data and User Data |
| ff_malloc                        | required user space size                   | a pointer to a block's user space             | iterate through the free list until a big enough block is met, then handle the block pointer to "iteration_record_post_process" for processing and get a valid pointer to the required user data space. If there is no such block, make a new one. |
| ff_free                          | pointer to a user space to be freed        |                                               | free a block using the method "common_free"                  |
| bf_malloc                        | required user space size                   | a pointer to a block's user space             | iterate through the free list until a suitable block is met, then handle the block pointer to "iteration_record_post_process" for processing and get a valid pointer to the required user data space. If there is no such block, make a new one. |
| bf_free                          | pointer to a user space to be freed        |                                               | free a block using the method "common_free"                  |

The functions were designed in a pattern that there are in a layer-like organization. The lowest level contains some simple tool functions like the "is_head/ is_free_head", bearing in mind the Don't Repeat Yourself (DRY) principle of software development. These minor tool functions may seem trivial but in the actual development process, they did save the author a lot of code and time. 

The middle layer consists of a few larger size tools which deal with some more complicated logics in the linked list, such as pulling/inserting a new block into the given list. Although not as simple, these functions are still the base stones and none of them are handling the process in the big picture.

One layer up, there are the processing blocks in the pipeline of each memory operation. These functions, such as the "combine_free_neighbors" function, utilize all the aforementioned tools to implement desired goals in each step.

Finally, on top of all these components, "iteration_record_post_process" is the function that assemble them all together into one whole pipeline. Since the only difference in First Fit and Best Fit allocation policy is the searching step, different iteration code were made separately for "ff_malloc" and "bf_malloc", but after the suitable block is found/not found, the two function both use "iteration_record_post_process" to process the iteration result. For the memory freeing part, these two policies does exactly the same and thus their "ff_free" and "bf_free" function both call the "common_free" function to accomplish their mission.

### 2.4 The Big Picture: Memory Allocating and Freeing Process

In this project, when a user requires a certain bytes of memory by calling the malloc function, two different policies could be used to search for a free block in the list: First Fit and Best Fit.

Starting from the head block of the free list, First Fit will examine the free blocks one by one. Whenever a block that is big enough to fit in the required user data size, the iteration will stop and use that block.

Whereas in Best Fit, the search will also go linearly starting form the free list's head, but the main difference is that this search will not stop until a block that has "perfect size" has been met. During the iteration, it will continuously drop its recorded fit block for a smaller (but still larger than required) one. Here, the "perfect size" is defined as "required space" <= "the user space size of block"  <= "required space + META_SIZE".

After the search, there could be two results, with the easier one being that no suitable free block was found. In this situation, a new block will be created and the heap break pointer will be increased by the `sbrk()` function to accommodate this growth in the doubly linked list's size. This new block will be regarded as the list's new head, while the free list remains as unchanged since the new block will be used by the user.

If a free block was found during the search, what would happen to it depends on its current user space size and the required user space size. A "threshold" can be set that: whenever the found free block's user space size is larger than that "threshold", this block will be split into two, with the one with smaller address occupying "META_SIZE + required user space size", and the other taking the rest of the original block's space. The former will be returned and designated to the user for later use. This is where the function "free_list_pull_block" comes into use: this half of the block will be pulled out of the free list, while remaining in the list. Here the threshold is set as "block's user data space" >= "required data space size + META_DATA".

After the free block is ready to be allocated, one more step should be done: returning the pointer to the user space instead of to the block, because we don't want the Meta Data covered by user data and thus the failure of the whole list. With the virtual field in the struct, this is really easy: just return a pointer to "user_data_first_byte" field because that is the last field in the struct (i.e. top field in the heap).

As for the "freeing operation", three steps are to be done. First, since the passed in pointer points at the user data segment not the block, an offset should be applied to get the correct address of the block. Notice that due to struct data alignment and difference of environment, this offset should be set correctly. The author agree that a possible improvement would be to use a way that is environment-independent, and possibly also independent of what fields and what alignment policy the struct has. 

Then, if the block's neighbors in the list are also free blocks, a block combination will be performed to merge those blocks in to a single one. This process is very much like splitting the block during allocation, but happens in a reverse order. One noticeable difference is that no operation would need to be performed in the free list, since the free list does not have anything to do with the actual list's connection and the block to be freed was not in it anyways.

After that, the block's is_free status is set to be 0. No operation on the heap break pointer or initialization of the block's user data segment is performed.



## 3 Result Analysis

In this section, the result of running the evaluation programs "large_range_rand_allocs", "small_range_rand_allocs" and "equal_size_allocs" for both of the allocation policies (First Fit) and (Best Fit) are analyzed and compared. Also discussed is the author's thoughts and the possible reasons for the observed results. Finally, a recommendation is given based on the test results.

The following tests used the default settings: 

|                         | NUM_ITERS | NUM_ITEMS |
| ----------------------- | --------- | --------- |
| large_range_rand_allocs | 50        | 10000     |
| small_range_rand_allocs | 100       | 10000     |
| equal_size_allocs       | 10000     | 10000     |

### Execution Time:	

|                         | First Fit         | Best Fit          |
| ----------------------- | ----------------- | ----------------- |
| large_range_rand_allocs | 23.818411 seconds | 21.681550 seconds |
| small_range_rand_allocs | 3.587459 seconds  | 1.359807 seconds  |
| equal_size_allocs       | 2.237515 seconds  | 2.310541 seconds  |

### Fragmentation:

|                         | First Fit | Best Fit |
| ----------------------- | --------- | -------- |
| large_range_rand_allocs | 0.118034  | 0.160375 |
| small_range_rand_allocs | 0.077785  | 0.067848 |
| equal_size_allocs       | 0.450000  | 0.450000 |

The first thing that was noticed from this result was that why the "large_range_rand_allocs" and "small_range_rand_allocs" tests have different result in both execution time and fragmentation, since they were following the same process except for the scale of their size?

To further study this question, another test was done for the First Fit policy, with both "large_range_rand_allocs" and "small_range_rand_allocs" have the same setting: 

|                         | NUM_ITERS | NUM_ITEMS |
| ----------------------- | --------- | --------- |
| large_range_rand_allocs | 100       | 10000     |
| small_range_rand_allocs | 100       | 10000     |

And the result was as follows:

|                         | Execution Time    | Fragmentation |
| ----------------------- | ----------------- | ------------- |
| large_range_rand_allocs | 47.131407 seconds | 0.118034      |
| small_range_rand_allocs | 3.572504 seconds  | 0.077785      |

When the NUM_ITERS parameter of "large_range_rand_allocs" increased from 50 to 100, the fragmentation almost did not change, but the execution time increased significantly, leaving an even larger difference between "large_range_rand_allocs" and "small_range_rand_allocs".

Logically, if the result mainly depends on the operations provided by this project's functions, it would be almost the same between the "large_range_rand_allocs" and "small_range_rand_allocs", since these two test case had the same settings and roughly the same number of operations (despite the randomness added into each test). But the execution time is always longer for the "large_range_rand_allocs". This results in the author's conclusion that the execution time will  increase along with the scale of memory allocation. And the reason behind this could be that the runtime of `sbrk` depends on the size of the allocation.

Also, the fact that the fragmentation remained unchanged indicates that it does not depend on the scale of allocation, but rather the logic itself.

For "equal_size_allocs", since it always allocates blocks of the same size, and then free them in the same order of allocation, it is natural that both "First Fit" and "Best Fit" have almost the same result, since "Best Fit" would act the same  way as "Best Fit" does when the blocks' size are all the same.

It can also be observed that in both "small_range_rand_allocs" and "large_range_rand_allocs", First Fit spends more time than "Best Fit". But when it comes to fragmentation, First Fit performs worse in "small_range_rand_allocs", but better in "large_range_rand_allocs". 

Considering the fact that the actual world is always full of randomness, when it comes to the question of a "recommending a polity", the answer would really be "it depends". For example, if speed is the priority, then Best Fit could be a better choice. But if the allocation will all be large enough and fragmentation is more important, then going with First Fit will also not be wrong. However, in general, Best Fit is potentially a better choice because the above result has shown its advantage over First Fit in most cases.



## 4 Conclusion

This project has built the malloc and free function for both the First Fit and Best Fit allocation policy, with their performance tested and analyzed. Through the process, the author's understanding of how memory in heap gets allocated and freed was tested and improved. The D.R.Y principle and the Single Responsibility principle really showed their use meaning in actual software development, which helped this project a lot. 

