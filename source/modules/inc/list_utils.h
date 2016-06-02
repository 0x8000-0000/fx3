/**
 * @file list_utils.h
 * @brief List utilities
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

#ifndef __LIST_UTILS_H__
#define __LIST_UTILS_H__

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

struct list_element
{
   struct list_element* next;
};

static inline unsigned lst_computeLength(struct list_element* listElem)
{
   unsigned len = 0;
   while (listElem)
   {
      len ++;
      listElem = listElem->next;
   }
   return len;
}

typedef int (* list_element_comparator)(const struct list_element* left, const struct list_element* right);

static inline bool lst_isSortedAscending(struct list_element* listElem, list_element_comparator comparator)
{
   bool isSorted = true;

   while (listElem && listElem->next)
   {
      if (comparator(listElem, listElem->next) < 0)
      {
         listElem = listElem->next;
      }
      else
      {
         isSorted = false;
         break;
      }
   }

   return isSorted;
}


static inline void lst_insertIntoSortedList(struct list_element** listHead, struct list_element* newElement, list_element_comparator comparator)
{
   if (newElement)
   {
      while (*listHead)
      {
         if (comparator(*listHead, newElement) < 0)
         {
            listHead = &(*listHead)->next;
         }
         else
         {
            break;
         }
      }

      newElement->next = *listHead;
      *listHead        = newElement;
   }
}

static inline void lst_mergeListIntoSortedList(struct list_element** listHead, struct list_element* newElement, list_element_comparator comparator)
{
   while (newElement)
   {
      struct list_element* toAdd = newElement;
      newElement                 = newElement->next;
      toAdd->next                = 0;

      lst_insertIntoSortedList(listHead, toAdd, comparator);
   }
}

#ifdef __cplusplus
}
#endif

#endif // __LIST_UTILS_H__

