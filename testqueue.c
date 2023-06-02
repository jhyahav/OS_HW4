#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include "queue.c"

#define NUM_OPERATIONS 100

void test_enqueue_dequeue()
{
    initQueue();

    int items[] = {1, 2, 3, 4, 5};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Dequeue items and check the order
    for (size_t i = 0; i < num_items; i++)
    {
        int *item = (int *)dequeue();
        printf("Dequeued: %d\n", *item);
        assert(*item == items[i]);
    }

    // Queue should be empty
    assert(size() == 0);

    destroyQueue();
}

void test_tryDequeue()
{
    initQueue();

    int items[] = {1, 2, 3, 4, 5};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Try dequeueing when the queue is empty
    void *item;
    assert(!tryDequeue(&item));
    printf("tryDequeue when queue is empty - Assertion failed: Expected false\n");

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Try dequeueing items and check the order
    for (size_t i = 0; i < num_items; i++)
    {
        assert(tryDequeue(&item));
        printf("Dequeued: %d\n", *(int *)item);
        assert(*(int *)item == items[i]);
    }

    // Queue should be empty
    assert(size() == 0);

    destroyQueue();
}

void test_waiting_visited()
{
    initQueue();

    int items[] = {1, 2, 3};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Create threads that will wait for items in the queue
    thrd_t threads[3];
    for (int i = 0; i < 3; i++)
    {
        thrd_create(&threads[i], (int (*)(void *))dequeue, NULL);
    }

    // Wait for all threads to finish
    for (int i = 0; i < 3; i++)
    {
        thrd_join(threads[i], NULL);
    }

    // Queue should be empty
    assert(size() == 0);
    // All items should have been visited
    assert(visited() == num_items);
    // No threads should be waiting
    assert(waiting() == 0);

    destroyQueue();
}

void test_random_operations()
{
    initQueue();

    int items[NUM_OPERATIONS];
    bool isEnqueued[NUM_OPERATIONS];
    size_t numEnqueued = 0;

    // Initialize items and isEnqueued array
    for (size_t i = 0; i < NUM_OPERATIONS; i++)
    {
        items[i] = i + 1;
        isEnqueued[i] = false;
    }

    // Perform random operations
    for (size_t i = 0; i < NUM_OPERATIONS; i++)
    {
        int randomOp = rand() % 2; // 0 for enqueue, 1 for dequeue

        if (randomOp == 0)
        {
            // Enqueue an item
            enqueue(&items[i]);
            isEnqueued[i] = true;
            numEnqueued++;
        }
        else
        {
            // Dequeue an item if the queue is not empty
            if (size() > 0)
            {
                int *item = (int *)dequeue();
                isEnqueued[*item - 1] = false;
                numEnqueued--;
            }
        }

        // Compare state after each operation
        assert(size() == numEnqueued);
        // assert(visited() == i + 1);
        assert(waiting() == 0);
    }

    // Verify the state at the end
    assert(size() == numEnqueued);
    // assert(visited() == NUM_OPERATIONS);
    assert(waiting() == 0);
    printf("Random ops test passed\n");
    destroyQueue();
}

void test_edge_cases()
{
    initQueue();

    // Try dequeue from an empty queue - Should return false
    void *tryItem = (void *)"tryDequeue from empty queue: tryItem untouched passed";
    assert(!tryDequeue(&tryItem));
    printf((char *)tryItem);

    // Enqueue items
    int item1 = 1;
    int item2 = 2;
    int item3 = 3;
    enqueue(&item1);
    enqueue(&item2);
    enqueue(&item3);

    // Dequeue multiple items and check the order
    assert(*(int *)dequeue() == item1);
    assert(*(int *)dequeue() == item2);
    assert(*(int *)dequeue() == item3);

    // Try dequeue from an empty queue after all items have been dequeued - Should return false
    assert(!tryDequeue(&tryItem));

    // Dequeue from an empty queue - Should block until an item is enqueued
    int *item = (int *)dequeue();
    assert(item == NULL);
    printf("Dequeue from an empty queue - Assertion failed: Expected NULL\n");

    destroyQueue();
}

int main()
{
    test_enqueue_dequeue();
    test_tryDequeue();
    test_waiting_visited();
    test_random_operations();
    test_edge_cases();

    printf("All tests passed!\n");

    return 0;
}