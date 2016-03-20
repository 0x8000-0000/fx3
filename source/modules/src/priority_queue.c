/**
 * @file priority_queue.c
 * @brief Priority Queue implementation
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
 *
 * Implementation based on the "Programming Pearls, 2nd ed" by Jon Bentley
 */

#include <priority_queue.h>

void prq_initialize(struct priority_queue* pq, uint32_t** memPool, uint32_t queueSize)
{
   pq->capacity = queueSize;
   pq->size     = 0;
   pq->memPool  = memPool;
}

bool prq_isEmpty(struct priority_queue* pq)
{
   return (0 == pq->size);
}

bool prq_isFull(struct priority_queue* pq)
{
   return (pq->size == pq->capacity);
}

static void siftdown(uint32_t** A, uint32_t count)
{
   unsigned parent = 1;

   while (true)
   {
      unsigned child = parent * 2;

      if (child > count)
      {
         break;
      }

      if ((child + 1) <= count)
      {
         if (*(A[child + 1]) < *(A[child]))
         {
            child ++;
         }
      }

      if (*(A[parent]) <= *(A[child]))
      {
         break;
      }

      uint32_t* temp = A[parent];
      A[parent]      = A[child];
      A[child]       = temp;
      parent         = child;
   }
}

static void siftup(uint32_t** A, uint32_t count)
{
   uint32_t child = count;

   while (child > 1)
   {
      uint32_t parent = child / 2;
      if (*(A[parent]) > *(A[child]))
      {
         uint32_t* temp = A[parent];
         A[parent]      = A[child];
         A[child]       = temp;
         child          = parent;
      }
      else
      {
         break;
      }
   }
}

bool prq_push(struct priority_queue* pq, uint32_t* obj)
{
   bool isFull = prq_isFull(pq);

   if (! isFull)
   {
      pq->size ++;
      pq->memPool[pq->size] = obj;

      siftup(pq->memPool, pq->size);
   }

   return isFull;
}

uint32_t* prq_pop(struct priority_queue* pq)
{
   uint32_t* val = NULL;
   if (! prq_isEmpty(pq))
   {
      val = pq->memPool[1];
      pq->memPool[1] = pq->memPool[pq->size --];
      siftdown(pq->memPool, pq->size);
   }

   return val;
}

