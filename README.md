# Thread-Safe Malloc Implementation

- Intro:  
  This project implements two memory allocation functions from the C standard library: `malloc` and `free`  
  - `malloc()`: takes in a size (number of bytes) for a memory allocation, locates an address in the program’s data region where there is enough space to fit the specified number of bytes, and returns this address for use by the calling program.  
  - `free()`: takes an address (that was returned by a previous malloc operation) and marks that data region as available again for use. 
  <br>
- Features:    
  1. No-Lock Synchronization for multithreading:  
    By using Local Thread Storage (LTS), each thread will be allocated a 
