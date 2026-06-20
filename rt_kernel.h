#ifndef KERNEL_H
#define KERNEL_H

#include "stm32f4xx.h"
#include <stdint.h>


/*================ CONFIG =================*/

#define MAX_TASKS       6
#define MAX_PRIORITY    8
#define STACK_SIZE      128

#define TICK_RATE_HZ    1000

#define STACK_PATTERN   0xA5A5A5A5

/*================ TASK STATES =================*/

typedef enum
{
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED,
    TASK_DELETED

} task_state_t;

/*================ TCB =================*/

typedef struct tcb
{
    uint32_t *sp;

    uint32_t *stack_start;
    uint32_t *stack_end;

    struct tcb *next;
    struct tcb *prev;

    uint32_t delay;
    uint32_t runtime;

    uint8_t priority;
    uint8_t state;

    uint32_t notify_value;
    uint8_t notify_state;

} TCB;

/*================ READY QUEUE =================*/

typedef struct
{
    TCB *head;
    TCB *tail;

} ready_queue_t;

/*================ GLOBAL VARIABLES =================*/


/*================ SCHEDULER =================*/

uint8_t highest_priority(void);

TCB* schedule(void);

/*================ TASK MANAGEMENT =================*/

int task_create(
    void (*task)(void *),
    uint8_t priority,
    void *param
);

void os_task_suspend(TCB *t);

void os_task_resume(TCB *t);

void os_task_delete(TCB *t);

/*================ TASK CONTROL =================*/

void os_delay(uint32_t ticks);

void os_yield(void);

void os_yield_from_isr(void);

/*================ STACK =================*/

void stack_init(uint32_t id, void (*task)(void *), void *param);

void stack_check(TCB *t);

/*================ READY LIST =================*/

void ready_insert(TCB *t);

void ready_remove(TCB *t);

/*================ DELAY LIST =================*/

void delay_insert(TCB *t, uint32_t ticks);

uint32_t next_delay(void);

/*================ RUNTIME =================*/

void runtime_update(void);

/*================ OS CONTROL =================*/

void os_start(void);

void start_first_task(void);

/*================ IDLE =================*/

void idle_task(void *p);
void os_tickless_idle(void);

void os_watchdog_kick(void);

/*================ INTERRUPTS =================*/

void PendSV_Handler(void);

void SysTick_Handler(void);

#endif
