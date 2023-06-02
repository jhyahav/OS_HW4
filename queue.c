#include "queue.h"

// We implement the queue as a linked list, saving its head and tail.
struct Queue
{
    struct Element *head;
    struct Element *tail;
    size_t queue_size;
    size_t waiting_count;
    size_t visited_count;
};

struct Element
{
    struct Element *next;
    const void *data;
};

static struct Queue queue;

void initQueue(void)
{
}

void destroyQueue(void)
{
}

void enqueue(const void *element_data)
{
}

void *dequeue(void)
{
}

bool tryDequeue(void **element)
{
}

size_t size(void)
{
    return queue.queue_size;
}

size_t waiting(void)
{
    return queue.waiting_count;
}

size_t visited(void)
{
    return queue.visited_count;
}