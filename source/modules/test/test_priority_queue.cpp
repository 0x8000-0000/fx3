#include <algorithm>

#include <priority_queue.h>

#include <CppUTest/TestHarness.h>

TEST_GROUP(PriorityQueue)
{
   static const uint32_t queueSize = 32;

   struct priority_queue pq;
   uint32_t* memPool[queueSize];

   void setup()
   {
      prq_initialize(&pq, memPool, queueSize);
   }

   void tearDown()
   {
   }
};

TEST(PriorityQueue, NewQueueIsEmpty)
{
   CHECK(prq_isEmpty(&pq));
}

TEST(PriorityQueue, AfterAddingOneQueueIsNotEmpty)
{
   uint32_t val = 43;

   prq_push(&pq, &val);

   CHECK(! prq_isEmpty(&pq));
}

TEST(PriorityQueue, AfterAddingOneThenRemovingOneQueueIsEmpty)
{
   uint32_t val = 43;

   prq_push(&pq, &val);

   uint32_t* ptr = prq_pop(&pq);

   CHECK(prq_isEmpty(&pq));

   CHECK(&val == ptr);
}

TEST(PriorityQueue, TwoElementsInOrder)
{
   uint32_t first  = 5;
   uint32_t second = 9;

   prq_push(&pq, &first);
   prq_push(&pq, &second);

   uint32_t* firstPtr = prq_pop(&pq);
   uint32_t* secondPtr = prq_pop(&pq);

   CHECK(prq_isEmpty(&pq));

   CHECK(&first  == firstPtr);
   CHECK(&second == secondPtr);
}

TEST(PriorityQueue, TwoElementsOutOfOrder)
{
   uint32_t first  = 9;
   uint32_t second = 5;

   prq_push(&pq, &first);
   prq_push(&pq, &second);

   uint32_t* firstPtr = prq_pop(&pq);
   uint32_t* secondPtr = prq_pop(&pq);

   CHECK(prq_isEmpty(&pq));

   CHECK(&second == firstPtr);
   CHECK(&first  == secondPtr);
}

TEST(PriorityQueue, PermutationsOfXValues)
{
#ifdef HAS_A_LOT_OF_TIME_AVAILABLE
   const uint32_t XX = 10;
#else
   const uint32_t XX = 5;
#endif

   uint32_t* values = new uint32_t[XX];

   for (uint32_t ii = 0; ii < XX; ii ++)
   {
      values[ii] = ii;
   }

   uint32_t* sortedValues = new uint32_t[XX];
   std::copy(values, values + XX, sortedValues);
   std::sort(sortedValues, sortedValues + XX);

   uint32_t** checkValues = new uint32_t*[XX];

   do
   {
      CHECK(prq_isEmpty(&pq));

      for (uint32_t ii = 0; ii < XX; ii ++)
      {
         prq_push(&pq, &values[ii]);
      }

      for (uint32_t ii = 0; ii < XX; ii ++)
      {
         checkValues[ii] = prq_pop(&pq);

         CHECK(values <= checkValues[ii]);
         CHECK(checkValues[ii] < (values + XX));

         if (ii)
         {
            CHECK(*(checkValues[ii - 1]) < *(checkValues[ii]));
         }
      }
   }
   while (std::next_permutation(values, values + XX));

   delete[] checkValues;
   delete[] sortedValues;
   delete[] values;
}
