/**
 * @file priority_queue.h
 * @brief Priority Queue declarations
 * @author Florin Iucha <florin@signbit.net>
 * @copyright Apache License, Version 2.0
 */

/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of FX3 RTOS for ARM Cortex-M4
 */

#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#ifdef __cplusplus
extern "C"
{
#else
#include <stdbool.h>
#endif

#include <stdlib.h>
#include <stdint.h>

/** Priority queue of pointers to objects. The queue implmentation is
 * intrusive, as it requires the first element in the object structure
 * to be an integer-value priority.
 *
 * The priority queue stores pointers and returns pointers. Objects are
 * compared by dereferencing the pointers and comparing the values as
 * integers.  The client is responsible for ensuring the objects are
 * properly aligned in memory.
 *
 * The lower the numerical value of the priority, the higher actual priority.
 * Priority 0 represents the highest priority.
 */

struct priority_queue
{
   uint32_t    capacity;
   uint32_t    size;

   uint32_t**  memPool;
};

/** Initialize a priority queue
 *
 * @param pq points to priority queue
 * @param memPool represents memory for the queue, its size has to be 1 greater than queueSize
 * @param queue_size represents the maximum number of elements in the queue
 */
void prq_initialize(struct priority_queue* pq, uint32_t** memPool, uint32_t queueSize);

/**
 * @return true if the queue is empty
 */
bool prq_isEmpty(struct priority_queue* pq);

/**
 * @return true if the queue is full
 */
bool prq_isFull(struct priority_queue* pq);

/** Pushes an object into the queue
 *
 * @param pq points to priority queue
 * @param[in] obj is the object
 * @param priority is the priority
 * @return true if successful, false if queue is full
 */
bool prq_push(struct priority_queue* pq, uint32_t* obj);

/** Pops the highest priority element from the queue
 *
 * @param pq points to priority queue
 * @param[out] priority p the priority of the popped object
 * @return obj is the object, or NULL if queue is empty
 */
uint32_t* prq_pop(struct priority_queue* pq);

#ifdef __cplusplus
}
#endif

#endif // __PRIORITY_QUEUE_H__

