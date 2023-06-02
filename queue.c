#include "queue.h"
#include <threads.h>
#include <stdatomic.h>

struct ThreadQueue
{
    struct ThreadElement *head;
    mtx_t thread_queue_lock;
};

struct ThreadElement
{
    struct ThreadElement *next;
    cnd_t cnd_thread;
};

// We implement the queue as a linked list, saving its head and tail.
struct DataQueue
{
    struct DataElement *head;
    struct DataElement *tail;
    atomic_ulong queue_size;
    size_t waiting_count;
    atomic_ulong visited_count;
    mtx_t data_queue_lock;
    cnd_t enq;
    cnd_t deq;
};

struct DataElement
{
    struct DataElement *next;
    const void *data;
};

/*
    We keep track of the threads in order of sleep time in order to
    always signal the oldest one and thus maintain the FIFO order between them.
*/
static struct ThreadQueue thread_queue;
static struct DataQueue data_queue;

struct DataElement *create_element(const void *data);

void initQueue(void)
{
}

void destroyQueue(void)
{
    mtx_destroy(&data_queue.data_queue_lock);
    cnd_destroy(&data_queue.enq);
    cnd_destroy(&data_queue.deq);
}

void enqueue(const void *element_data)
{
    struct DataElement *new_element = create_element(element_data);
    mtx_lock(&data_queue.data_queue_lock);
    add_element_to_queue(new_element);
    mtx_unlock(&data_queue.data_queue_lock);
}

struct DataElement *create_element(const void *data)
{
    // We assume malloc does not fail, as per the instructions.
    struct DataElement *element = (struct DataElement *)malloc(sizeof(struct DataElement));
    element->data = data;
    element->next = NULL;
    return element;
}

void add_element_to_queue(struct DataElement *new_element)
{
    data_queue.queue_size == 0 ? add_element_to_empty_queue(new_element) : add_element_to_nonempty_queue(new_element);
}

void add_element_to_empty_queue(struct DataElement *new_element)
{
    data_queue.head = new_element;
    data_queue.tail = new_element;
    data_queue.queue_size++; // FIXME: blocking for size()?
}

void add_element_to_nonempty_queue(struct DataElement *new_element)
{
    data_queue.tail->next = new_element;
    data_queue.tail = new_element;
    data_queue.queue_size++; // FIXME: blocking for size()?
}

void *dequeue(void)
{
    mtx_lock(&data_queue.data_queue_lock);
    while (1)
    {
    }

    // TODO: add implementation
    mtx_unlock(&data_queue.data_queue_lock);
}

bool tryDequeue(void **element)
{
}

size_t size(void)
{
    return data_queue.queue_size;
}

size_t waiting(void)
{
    // TODO: add locking here(?)
    return data_queue.waiting_count;
}

size_t visited(void)
{
    return data_queue.visited_count;
}