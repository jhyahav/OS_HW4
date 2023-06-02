#include "queue.h"
#include <threads.h>

// We implement the queue as a linked list, saving its head and tail.
struct Queue
{
    struct Element *head;
    struct Element *tail;
    size_t queue_size; // TODO: make me atomic?
    size_t waiting_count;
    size_t visited_count;
};

struct Element
{
    struct Element *next;
    const void *data;
};

static struct Queue queue;

struct Element *create_element(const void *data);

void initQueue(void)
{
}

void destroyQueue(void)
{
}

void enqueue(const void *element_data)
{
    struct Element *new_element = create_element(element_data);
    // TODO: add locking around next line
    add_element_to_queue(new_element);
}

struct Element *create_element(const void *data)
{
    // We assume malloc does not fail, as per the instructions.
    struct Element *element = (struct Element *)malloc(sizeof(struct Element));
    element->data = data;
    element->next = NULL;
    return element;
}

void add_element_to_queue(struct Element *new_element)
{
    queue.queue_size == 0 ? add_element_to_empty_queue(new_element) : add_element_to_nonempty_queue(new_element);
}

void add_element_to_empty_queue(struct Element *new_element)
{
    queue.head = new_element;
    queue.tail = new_element;
    queue.queue_size += 1;
}

void add_element_to_nonempty_queue(struct Element *new_element)
{
    queue.tail->next = new_element;
    queue.tail = new_element;
    queue.queue_size += 1;
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
    // TODO: add locking here(?)
    return queue.waiting_count;
}

size_t visited(void)
{
    return queue.visited_count;
}