#include "queue.h"
#include <string.h>
extern TCB *current_task;
void queue_init(Queue_t *q, uint8_t *buffer,
                uint32_t length,
                uint32_t item_size)
{
    q->buffer = buffer;
    q->length = length;
    q->item_size = item_size;

    q->head = 0;
    q->tail = 0;
    q->count = 0;

    q->wait_send = 0;
    q->wait_recv = 0;
}

int queue_send(Queue_t *q, void *item, uint32_t ticks)
{
    __disable_irq();

    if(q->count == q->length)
    {
        current_task->state = TASK_BLOCKED;
        ready_remove(current_task);

        current_task->next = q->wait_send;
        q->wait_send = current_task;

        if(ticks)
            delay_insert(current_task, ticks);

        __enable_irq();

        os_yield();

        return -  1;
    }

    memcpy(&q->buffer[q->head * q->item_size],
           item,
           q->item_size);

    q->head = (q->head + 1) % q->length;

    q->count++;

    if(q->wait_recv)
    {
        TCB *t = q->wait_recv;
        q->wait_recv = t->next;

        t->state = TASK_READY;
        ready_insert(t);
    }

    __enable_irq();

    return 0;
}

int queue_receive(Queue_t *q, void *item, uint32_t ticks)
{
    __disable_irq();

    if(q->count == 0)
    {
        current_task->state = TASK_BLOCKED;
        ready_remove(current_task);

        current_task->next = q->wait_recv;
        q->wait_recv = current_task;

        if(ticks)
            delay_insert(current_task, ticks);

        __enable_irq();

        os_yield();

        return -1;
    }

    memcpy(item,
           &q->buffer[q->tail * q->item_size],
           q->item_size);

    q->tail = (q->tail + 1) % q->length;

    q->count--;

    if(q->wait_send)
    {
        TCB *t = q->wait_send;
        q->wait_send = t->next;

        t->state = TASK_READY;
        ready_insert(t);
    }

    __enable_irq();

    return 0;
}
