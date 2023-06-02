#include "queue.h"
#include <threads.h>
#include <stdatomic.h>

struct ThreadQueue
{
    struct ThreadElement *head;
    struct ThreadElement *tail;
    mtx_t thread_queue_lock;
    atomic_ulong waiting_count;
};

struct ThreadElement
{
    thrd_t id;
    struct ThreadElement *next;
    cnd_t cnd_thread;
};

// We implement the queue as a linked list, saving its head and tail.
struct DataQueue
{
    struct DataElement *head;
    struct DataElement *tail;
    atomic_ulong queue_size;
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
    destroy_thread_queue();
}

void destroy_thread_queue(void)
{
    while (thread_queue.head != NULL)
    {
        // TODO: implement
    }
}

void enqueue(const void *element_data)
{
    struct DataElement *new_element = create_element(element_data);
    mtx_lock(&data_queue.data_queue_lock);
    add_element_to_data_queue(new_element);
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

void add_element_to_data_queue(struct DataElement *new_element)
{
    data_queue.queue_size == 0 ? add_element_to_empty_data_queue(new_element) : add_element_to_nonempty_data_queue(new_element);
}

void add_element_to_empty_data_queue(struct DataElement *new_element)
{
    data_queue.head = new_element;
    data_queue.tail = new_element;
    data_queue.queue_size++; // FIXME: blocking for size()?
}

void add_element_to_nonempty_data_queue(struct DataElement *new_element)
{
    data_queue.tail->next = new_element;
    data_queue.tail = new_element;
    data_queue.queue_size++; // FIXME: blocking for size()?
}

void *dequeue(void)
{
    mtx_lock(&data_queue.data_queue_lock);
    // This loop blocks as required
    while (current_thread_should_sleep())
    {
        thread_enqueue();
        cnd_wait(&thread_queue.tail->cnd_thread, &thread_queue.thread_queue_lock); // FIXME: might we need the other lock here?
        thread_dequeue();
    }

    struct DataElement *dequeued_data = data_queue.head;
    data_queue.head = dequeued_data->next;
    if (data_queue.head == NULL)
    {
        data_queue.tail = NULL;
    }
    data_queue.queue_size--;
    mtx_unlock(&data_queue.data_queue_lock);
    void *data = dequeued_data->data;
    free(dequeued_data);
    return data;
}

bool current_thread_should_sleep() // TODO: ensure no multiple entries in thread queue.
{
    bool queue_is_empty = data_queue.queue_size == 0;
    bool waiting_threads_exist = thread_queue.waiting_count > 0;
    bool current_is_oldest_thread = thrd_equal(thrd_current(), thread_queue.head->id);
    return queue_is_empty || (waiting_threads_exist && current_is_oldest_thread);
}

void thread_enqueue()
{
    struct ThreadElement *new_thread_element = create_thread_element();
    mtx_lock(&thread_queue.thread_queue_lock);
    add_element_to_thread_queue();
    mtx_unlock(&thread_queue.thread_queue_lock);
}

void thread_dequeue()
{
    mtx_lock(&thread_queue.thread_queue_lock);
    cnd_destroy(&thread_queue.head->cnd_thread);
    struct ThreadElement *dequeued_thread = thread_queue.head;
    thread_queue.head = thread_queue.head->next;
    free(dequeued_thread);
    if (thread_queue.head == NULL)
    {
        thread_queue.tail = NULL;
    }
    thread_queue.waiting_count--;
    mtx_unlock(&thread_queue.thread_queue_lock);
}

void add_element_to_thread_queue()
{
}

struct ThreadElement *create_thread_element()
{
    struct ThreadElement *thread_element = (struct ThreadElement *)malloc(sizeof(struct ThreadElement));
    thread_element->id = thrd_current();
    thread_element->next = NULL;
    cnd_init(&thread_element->cnd_thread);
}

bool tryDequeue(void **element)
{
    mtx_lock(&data_queue.data_queue_lock);
    if (data_queue.queue_size == 0)
    {
        mtx_unlock(&data_queue.data_queue_lock);
        return false;
    }
    // TODO: DRY
    struct DataElement *dequeued_data = data_queue.head;
    data_queue.head = dequeued_data->next;
    if (data_queue.head == NULL)
    {
        data_queue.tail = NULL;
    }
    data_queue.queue_size--;
    mtx_unlock(&data_queue.data_queue_lock);
    *element = (void *)dequeued_data->data;
    free(dequeued_data);
    return true;
}

size_t size(void)
{
    return data_queue.queue_size;
}

size_t waiting(void)
{
    // TODO: add locking here(?)
    return thread_queue.waiting_count;
}

size_t visited(void)
{
    return data_queue.visited_count;
}