# Lock & No-Lock Synchronization Implementation & Study Report

Based on the previous project, this project implements thread-safe malloc and free functions using two different strategies (lock and no-lock). The following content discusses where the implementation allows concurrency, the identified critical section and the synchronization strategies to prevent race condition. Also discussed is the results from the experiment and the tradeoff between these two strategies.

This implementation allows multiple threads to allocate and free memory in the same heap. Based on the previous project's implementation of best-fit malloc and free functions and the new strategies for synchronizing operations of multiple threads, memory addresses from the same heap will be guaranteed to malloc and free safely without facing the risk of race condition.

Some critical sections were identified in the basic malloc and free function:

1. The sbrk() system call. Threads should not be allowed to acquire the current heap pointer or move the heap pointer at the same time as this would result in overlapping in the allocation of heap memory addresses.
2. The while() loop for searching a suitable block in the malloc function. If more than one thread enter the loop at the same time, they may end up with the same block and hence a race condition would happen.  
3. Also, the code for processing the block look-up result after the while loop should also be a part of an atomic operation together with the while-loop because that part contains code to pull the found suitable free block out from the free list. If one thread is in the loop iterating through the free list and the other is doing post-iteration operations to some free blocks while that block is still in the free list, chances are that these two thread can still be doing something at the same time to that same block and hence a race condition could happen.
4. The "insert block into free list" code in the free() function. This is in case that two threads try to free two neighboring blocks -- at that time the two do not find the neighboring blocks free and hence will not perform the "combine" operation properly. 

The two synchronization strategies used here are using mutex and using Local Thread Storage (LTS). One thing worth mentioning here is that when doing the implementation, the author noticed that in the test case "thread_test_malloc_free_change_thread" and also the measuring test case "thread_test_malloc_free_measurement", threads are allowed to free blocks that were malloced by other threads. This makes no difference in the lock version of implementation since it basically does not have any parallelism and threads just queue up for malloc and free function, however this will result in failure of the LTS method if the basic malloc function was implemented using a free list and a physical list. If the malloc function was implemented with only a free list, then everything is fine since the physical list is public and hence no interference could be possible between threads. This is why the basic implementation transits from one physical list + one free list to a single free list. 

For the mutex version, the malloc function and free function are both locked using one mutex, so that whenever a thread enters either of the two, other thread will be blocked and wait for it to finish. This strategy works intuitively and can exclude any potential race condition from happening.

The LTS version of implementation gives each thread a local storage for storing their pointer to the free list's head. In doing so, each thread will have its own free list of blocks while sharing the non-free blocks with all other threads. This precludes any race condition in the operation of mallocing a block, and also in the process of merging a newly freed block with its neighbors. This LTS version of synchronizing threads for malloc and free function works nicely without any overlapping reported during all experiments.

As for the measurement results of the two implementations, the LTS version is seemingly faster than its counterpart, with a running time of just over 0.44 seconds, while the mutex version spent 1.56 seconds to finish the same test. This is because in the mutex is always heavily contended for by all the threads. This leads to the fact that the whole code is actually severely serialized and does not improve much compared to a single-threaded setup with respect to speed. On the other hand, the LTS version can sufficiently leverage the advantage of multi-threading, thanks to its strategy for eliminating any contending for locks and do not block any thread during run time.

The data segment sizes of the two strategies' test measurement did not imply much of a difference in allocation efficiency, with the LTS version has a data segment size of 43,015,328 bytes and the mutex version has 45,214,304 bytes. In the author's opinion, since these two strategies are both based on the same "Best Fit" malloc and free functions, it is reasonable that their allocation efficiencies are close to each other because either serialized or parallelized, the basic logic of allocating heap memory does not change very much.

However, one down-side of the LTS strategy would be that each thread would need a piece of separate memory to store their free-list's head block pointer. The overhead seems not a very big problem but the space complexity does grow linearly as the the number of threads increases. Hence TLS may not be the best choice when storage space is very limited and parallelism is not the most required characteristic.







