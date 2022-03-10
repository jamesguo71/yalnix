# Yalnix - Team Unics
This repository contains our (Fei & Taylor) code for the COSC258 Yalnix project.

See the branch `final` for our final submission.

### Extra functionality:

We did `semaphore` syscalls as the extra functionality. See `semaphore.c" and "semaphore.h" files for the implementation.

### Caveat about `Reclaim` 

For resources like pipe, lock, cvar, and semaphore, the process that initialized the resource by calling PipeInit, LockInit, CvarInit, SemaphoreInit are considered to be the owner of the resource, and only the owner of a resource will be able to Reclaim it. 
If the owner doesn't explicitly Reclaim a resource, it will be reclaimed when the process exits.
Moreover, when the process calls reclaim, it should make sure that the other processes depending on the resources have already terminated. Otherwise, the behavior is undefined.


