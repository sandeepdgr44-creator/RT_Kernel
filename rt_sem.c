#include "semaphore.h"



extern TCB *current_task;
void mutex_init(mutex_t *m)
{
    m->locked = 0;
    m->owner = 0;
    m->wait_list = 0;
}

void mutex_lock(mutex_t *m)
{

__disable_irq();

if(m->locked == 0)
{
    m->locked = 1;
    m->owner = current_task;

    __enable_irq();
    return;
}

/* mutex busy */

current_task->state = TASK_BLOCKED;

ready_remove(current_task);

/* add to wait list */

current_task->next = m->wait_list;
m->wait_list = current_task;

__enable_irq();

/* context switch */

os_yield();

}
void mutex_unlock(mutex_t *m)
{

__disable_irq();

if(m->owner != current_task)
{
    __enable_irq();
    return;
}

/* waiting task ? */

if(m->wait_list)
{
    TCB *t = m->wait_list;

    m->wait_list = t->next;

    t->state = TASK_READY;

    ready_insert(t);

    m->owner = t;
}
else
{
    m->locked = 0;
    m->owner = 0;
}

__enable_irq();

}




void sem_init(semaphore_t *s, int32_t init)
{
    s->count = init;
    s->wait_list = 0;
}
void sem_take(semaphore_t *s)
{

__disable_irq();

if(s->count > 0)
{
    s->count--;

    __enable_irq();
    return;
}

/* block task */

current_task->state = TASK_BLOCKED;

ready_remove(current_task);

/* add to wait list */

current_task->next = s->wait_list;

s->wait_list = current_task;

__enable_irq();

os_yield();

}
void sem_give(semaphore_t *s)
{

__disable_irq();

if(s->wait_list)
{
    TCB *t = s->wait_list;

    s->wait_list = t->next;

    t->state = TASK_READY;

    ready_insert(t);
}
else
{
    s->count++;
}

__enable_irq();

}

void sem_give_from_isr(semaphore_t *s)
{

if(s->wait_list)
{
    TCB *t = s->wait_list;

    s->wait_list = t->next;

    t->state = TASK_READY;

    ready_insert(t);

    /* request context switch */

    os_yield_from_isr();
}
else
{
    s->count++;
}

}
