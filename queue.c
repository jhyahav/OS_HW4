#include "queue.h"
#include <threads.h>
#include <stdatomic.h>
#include <stdlib.h>

struct ThreadQueue
{
    struct ThreadElement *head;
    struct ThreadElement *tail;
    mtx_t thread_queue_lock; // FIXME: get rid of this (carefully)
    atomic_ulong waiting_count;
};

struct ThreadElement // TODO: add tracker for which data element corresponds to thread for dequeue
{
    thrd_t id;
    struct ThreadElement *next;
    // Each thread has its own condition variable so we can signal it independently.
    cnd_t cnd_thread;
    bool terminated;
};

// We implement the queue as a linked list, saving its head and tail.
struct DataQueue
{
    struct DataElement *head;
    struct DataElement *tail;
    atomic_ulong queue_size;
    atomic_ulong visited_count;
    mtx_t data_queue_lock;
};

struct DataElement
{
    struct DataElement *next;
    void *data;
};

/*
    We keep track of the threads in order of sleep time in order to
    always signal the oldest one and thus maintain the FIFO order between them.
*/
static struct ThreadQueue thread_queue;
static struct DataQueue data_queue;
static thrd_t terminator;

void free_all_data_elements(void);
void destroy_thread_queue(void);
struct DataElement *create_element(void *data);
void add_element_to_data_queue(struct DataElement *new_element);
void add_element_to_empty_data_queue(struct DataElement *new_element);
void add_element_to_nonempty_data_queue(struct DataElement *new_element);
bool current_thread_should_sleep(void);
void thread_enqueue(void);
void thread_dequeue(void);
void add_element_to_thread_queue(struct ThreadElement *new_element);
void add_element_to_empty_thread_queue(struct ThreadElement *new_element);
void add_element_to_nonempty_thread_queue(struct ThreadElement *new_element);
struct ThreadElement *create_thread_element(void);

void initQueue(void)
{
    data_queue.head = NULL;
    thread_queue.head = NULL;
    data_queue.tail = NULL;
    thread_queue.tail = NULL;
    data_queue.queue_size = 0;
    thread_queue.waiting_count = 0;
    data_queue.visited_count = 0;
    mtx_init(&data_queue.data_queue_lock, mtx_plain);
    mtx_init(&thread_queue.thread_queue_lock, mtx_plain);
}

void destroyQueue(void)
{
    free_all_data_elements();
    destroy_thread_queue();
}

void free_all_data_elements(void)
{
    mtx_lock(&data_queue.data_queue_lock);
    struct DataElement *prev_head;
    while (data_queue.head != NULL)
    {
        prev_head = data_queue.head;
        data_queue.head = prev_head->next;
        free(prev_head);
    }
    // Resetting the fields is not strictly necessary, but just for good measure.
    data_queue.tail = NULL;
    data_queue.visited_count = 0;
    data_queue.queue_size = 0;
    mtx_unlock(&data_queue.data_queue_lock);
    mtx_destroy(&data_queue.data_queue_lock);
}

void destroy_thread_queue(void)
{
    mtx_lock(&thread_queue.thread_queue_lock);
    terminator = thrd_current();
    struct ThreadElement *prev_head;
    while (thread_queue.head != NULL)
    {
        prev_head = thread_queue.head;
        thread_queue.head = prev_head->next;
        prev_head->terminated = true;
        cnd_signal(&prev_head->cnd_thread);
        free(prev_head); // FIXME: signal doesn't block, free() should be called by the thread itself!
    }
    thread_queue.tail = NULL;
    thread_queue.waiting_count = 0;
    mtx_unlock(&thread_queue.thread_queue_lock);
    mtx_destroy(&thread_queue.thread_queue_lock);
}

void enqueue(void *element_data)
{
    // cnd_t *last_cnd = NULL;
    struct DataElement *new_element = create_element(element_data);
    mtx_lock(&data_queue.data_queue_lock);
    add_element_to_data_queue(new_element);
    mtx_unlock(&data_queue.data_queue_lock);

    // fine-grained data helps us wake up the specific thread we need. adding IDs should help with this
    // after adding ids, we should be able to transform this while loop into an if statement
    while (data_queue.queue_size > 0 && thread_queue.waiting_count > 0) //&& last_cnd != &thread_queue.head->cnd_thread)
    {
        printf("%d\n", !!thread_queue.head);
        // printf("%lx\n", &thread_queue.head->cnd_thread);
        cnd_signal(&thread_queue.head->cnd_thread);
        // TODO: thread_dequeue should be called here!!!! FIXME: important
        // last_cnd = &thread_queue.head->cnd_thread;
    }
    // mtx_unlock(&thread_queue.thread_queue_lock);
}

struct DataElement *create_element(void *data)
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
    data_queue.queue_size++;
}

void add_element_to_nonempty_data_queue(struct DataElement *new_element)
{
    data_queue.tail->next = new_element;
    data_queue.tail = new_element;
    data_queue.queue_size++;
}

void *dequeue(void)
{
    mtx_lock(&data_queue.data_queue_lock);
    // This loop blocks as required
    while (current_thread_should_sleep())
    {
        thread_enqueue();
        struct ThreadElement *current = thread_queue.tail;
        cnd_wait(&current->cnd_thread, &data_queue.data_queue_lock);
        if (current->terminated)
        {
            // Prevents runaway threads upon destruction
            thrd_join(terminator, NULL);
        }
        thread_dequeue();
    }

    struct DataElement *dequeued_data = data_queue.head;
    data_queue.head = dequeued_data->next;
    if (data_queue.head == NULL)
    {
        data_queue.tail = NULL;
    }
    data_queue.queue_size--;
    data_queue.visited_count++;
    mtx_unlock(&data_queue.data_queue_lock);
    void *data = dequeued_data->data;
    free(dequeued_data);
    return data;
}

bool current_thread_should_sleep(void) // TODO: ensure no multiple entries in thread queue.
{
    /*
        TODO: after assigning each thread queue element its corresponding data element id/index,
        what we need to do is instead of relying on counts, etc, check if the id/index the current thread needs
        is at the head of the thread queue and go to sleep if not
    */
    bool queue_is_empty = data_queue.queue_size == 0;
    mtx_lock(&thread_queue.thread_queue_lock);
    bool waiting_threads_exist = thread_queue.waiting_count > 0 && thread_queue.head; // FIXME: is this necessary?
    mtx_unlock(&thread_queue.thread_queue_lock);
    // bool current_is_oldest_thread = thrd_equal(thrd_current(), thread_queue.head->id);
    return queue_is_empty; //|| (waiting_threads_exist && !current_is_oldest_thread);
    // if number of waiting threads is greater than number of elements in data queue, can dequeue
}

void thread_enqueue(void)
{
    struct ThreadElement *new_thread_element = create_thread_element();
    mtx_lock(&thread_queue.thread_queue_lock);
    add_element_to_thread_queue(new_thread_element);
    mtx_unlock(&thread_queue.thread_queue_lock);
}

void thread_dequeue(void)
{
    mtx_lock(&thread_queue.thread_queue_lock);
    // cnd_destroy(&thread_queue.head->cnd_thread); FIXME: could this be our culprit?
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

void add_element_to_thread_queue(struct ThreadElement *new_element)
{
    thread_queue.waiting_count == 0 ? add_element_to_empty_thread_queue(new_element) : add_element_to_nonempty_thread_queue(new_element);
}

void add_element_to_empty_thread_queue(struct ThreadElement *new_element)
{
    thread_queue.head = new_element;
    thread_queue.tail = new_element;
    thread_queue.waiting_count++;
}

void add_element_to_nonempty_thread_queue(struct ThreadElement *new_element)
{
    thread_queue.tail->next = new_element;
    thread_queue.tail = new_element;
    thread_queue.waiting_count++;
}

struct ThreadElement *create_thread_element(void)
{
    struct ThreadElement *thread_element = (struct ThreadElement *)malloc(sizeof(struct ThreadElement));
    thread_element->id = thrd_current();
    thread_element->next = NULL;
    thread_element->terminated = false;
    cnd_init(&thread_element->cnd_thread);
    return thread_element;
}

bool tryDequeue(void **element)
{
    mtx_lock(&data_queue.data_queue_lock);
    while (data_queue.queue_size == 0 || data_queue.head == NULL)
    {
        mtx_unlock(&data_queue.data_queue_lock);
        return false;
    }
    struct DataElement *dequeued_data = data_queue.head;
    data_queue.head = dequeued_data->next;
    if (data_queue.head == NULL)
    {
        data_queue.tail = NULL;
    }
    data_queue.queue_size--;
    data_queue.visited_count++;
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
    return thread_queue.waiting_count;
}

size_t visited(void)
{
    return data_queue.visited_count;
}