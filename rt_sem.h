#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include "kernel.h"


typedef struct
{
    uint8_t locked;

    TCB *owner;

    TCB *wait_list;

}mutex_t;

typedef struct
{
    int32_t count;

    TCB *wait_list;

}semaphore_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

void sem_init(semaphore_t *s, int32_t init);
void sem_take(semaphore_t *s);
void sem_give(semaphore_t *s);
void sem_give_from_isr(semaphore_t *s);



#endif
