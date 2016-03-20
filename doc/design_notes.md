Design Notes for FX3
====================

Principles
----------

Use best known algorithms and implementation techniques.

Optimize for speed and space (both data structures and code size).


Kernel
------

### Ready-queue

Store non-blocking, non-sleeping tasks on a priority queue implemented using heap.

Tasks can be in one of the following states:

* UNINITIALIZED - the task object was not initialized, added to the runnable queue.
* RUNNING - this is the current running task
* READY - this task could run right now but its effective priority is lower than the task that is in RUNNING state
* RESTING - this task is READY, was RUNNING, but it yielded its turn
* EXHAUSTED - this task is READY, but it exhausted its round-robin quanta
* SLEEPING - this task is requested to sleep for a definite amount of time
* WAITING - this task is waiting for an event

#### Round-robin scheduling

To implement round-robin scheduling, introduce intermediary priorities. Once a task has exhausted its quanta, place it in an EXHAUSTED state.

All tasks that share the same priority belong to a 'linked ring'. The ring is created as the tasks are created, before the OS starts multitasking (the tasks as they are created, are inserted in to the 'sleeping' queue; before multitasking starts, the tasks are removed from the sleeping queue in priority order, and inserted into runnable queue - this is when we get 'streaks' of tasks with the same priority.)

Let's consider four tasks: A, B, C, D.  The priority of A, B and C is X and for D is (X + 1).  All four tasks are in READY state, with no higher priority tasks in READY state.

One of the A, B, C tasks is selected to be run and starts running, let's say A. A runs until its quanta is expired. At that point its quanta is replenished and A transitions to EXHAUSTED state.

Then one of the B and C tasks is selected to be running, let's say B. B runs, but before its quanta is expired, it yields.  B transitions to the RESTING state and is placed back on the task queue.  At this point, C is the only runnable task at priority X. C runs until it yields, the quanta expires, or a higher priority task becomes ready.

If the highest priority task is in EXHAUSTED state, then all the tasks with the same priority are EXHAUSTED: traverse the ring, refill their quanta buckets and switch them back to READY state. Note: the tasks do not need to be removed from the runnable queue for this adjustment, because their effective priorities are higher than all other tasks on the runnable queue.

### Clocks and delays

Timer queue implemented using a priority queue of pending tasks, where
the priority is the absolute deadline when the task should transition
from SLEEPING to READY.

Board Support Package
---------------------

Abstract the hardware requirements in a board-support package module.
