###USAGE 
* make clean : ensure you have called before any make as library compilation updates rather than resets.
* make : generates a library called libmythreads.a
* to use the library, use gcc with -o flag and include libmythreads.a at end of command.
  * example: gcc -o program programcode.c libmythreads.a

###DESIGN 
The contents are a "process threading" library. It is intended to allow
multitasking by sharing a single cpu thread. It uses a Round Robin approach. This is
done by using a 4 Circular Queues to store all threads:

1. "readyList" - this queue stores all active threads, those that can be run
2. "exitList" - this queue stores all threads that have returned from their function or called threadExit.
3. "lockWaiting" - this queue stores all threads that are waiting on a mutex lock
4. "conditionWaiting" - this queue stores all threads that are waiting on a signal attached to a specific mutual expression (a tlock in our case)

There is no priority or preference, except that child threads always get a chance to
run immediately after being called. 

The threads are handled by storing execution information in ucontext, then swapping
between them any time they voluntarily yield, forced to yield by a lock, or forced to
yield through some intentionally preemptive code in the calling program if following
"interruptsAreDisabled" to ensure it doesn't interrupt the library code (by using
threadYield on some sort of timer typically).

The code is hopefully well described in the actual files using a set comment scheme.

###KNOWN PROBLEMS
* In theory, there is chance for a race condition as checking "interruptsAreDisabled" and assigning the interruptsAreDisabled are not done atomically.
* The Queues are currently not super memory efficient. Extremely high thread counts, especially with sufficiently complex stacks, would likely hit stack overflow.
* Any errors in a child function that cause a flow stop would end the entire process. This is a consequence of the design using ucontext. However, I imagine there is some way of catching errors in called functions to prevent program escape. It's worth exploring
* There is definitely a lot of excess complication in the code that could (and needs) to be tuned and made more concise, something that time will clean.
* I tried to limit the amount of time spent within library code. Unfortunately, there is still quite a bit spent in the lock release code due to a lazy search algorithm.
