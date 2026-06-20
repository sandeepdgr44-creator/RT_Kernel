#include <stdint.h>
#include "kernel.h"

/*================ CONFIG =================*/

TCB *delay_list = 0;

ready_queue_t ready_list[MAX_PRIORITY];

uint32_t ready_bitmap = 0;

TCB tcb_array[MAX_TASKS];

uint32_t stacks[MAX_TASKS][STACK_SIZE];

TCB *current_task;

uint32_t task_count = 0;

volatile uint32_t tick_count = 0;


/*================ READY INSERT =================*/

void ready_insert(TCB *t)
{
    uint8_t p = t->priority;

    ready_queue_t *q = &ready_list[p];

    if(q->head == 0)
    {
        q->head = q->tail = t;

        t->next = 0;
        t->prev = 0;

        ready_bitmap |= (1 << p);
    }
    else
    {
        q->tail->next = t;

        t->prev = q->tail;

        t->next = 0;

        q->tail = t;
    }
}


/*================ READY REMOVE =================*/

void ready_remove(TCB *t)
{
    uint8_t p = t->priority;

    ready_queue_t *q = &ready_list[p];

    if(t->prev)
        t->prev->next = t->next;

    if(t->next)
        t->next->prev = t->prev;

    if(q->head == t)
        q->head = t->next;

    if(q->tail == t)
        q->tail = t->prev;

    if(q->head == 0)
        ready_bitmap &= ~(1 << p);
}


/*================ DELAY INSERT =================*/

void delay_insert(TCB *t, uint32_t ticks)
{
    uint32_t wake = tick_count + ticks;

    t->delay = wake;
    t->next = 0;

    if(delay_list == 0)
    {
        delay_list = t;
        return;
    }

    TCB *iter = delay_list;

    if(wake < iter->delay)
    {
        t->next = delay_list;
        delay_list = t;
        return;
    }

    while(iter->next && iter->next->delay < wake)
    {
        iter = iter->next;
    }

    t->next = iter->next;
    iter->next = t;
}


/*================ PRIORITY SEARCH =================*/

uint8_t highest_priority(void)
{
    for(uint8_t i = 0; i < MAX_PRIORITY; i++)
    {
        if(ready_bitmap & (1 << i))
            return i;
    }

    return 255;
}


/*================ SCHEDULER =================*/

TCB* schedule(void)
{
    uint8_t p = highest_priority();

    if(p == 255)
        return current_task;

    ready_queue_t *q = &ready_list[p];

    TCB *next = q->head;

    if(next && next->next)
    {
        q->head = next->next;

        q->tail->next = next;

        next->prev = q->tail;

        next->next = 0;

        q->tail = next;
    }

    return q->head;
}


/*================ STACK INIT =================*/

void stack_init(uint32_t id, void (*task)(void *), void *param)
{
    for(int i = 0; i < STACK_SIZE; i++)
        stacks[id][i] = STACK_PATTERN;

    uint32_t *sp;

    sp = &stacks[id][STACK_SIZE];

    *(--sp) = 0x01000000;          // xPSR
    *(--sp) = (uint32_t)task;      // PC
    *(--sp) = 0xFFFFFFFD;          // LR

    *(--sp) = 0;                   // R12
    *(--sp) = 0;                   // R3
    *(--sp) = 0;                   // R2
    *(--sp) = 0;                   // R1
    *(--sp) = (uint32_t)param;     // R0

    for(int i = 0; i < 8; i++)
        *(--sp) = 0;               // R4-R11

    tcb_array[id].sp = sp;

    tcb_array[id].stack_start = &stacks[id][0];

    tcb_array[id].stack_end = &stacks[id][STACK_SIZE];
}


/*================ STACK CHECK =================*/

void stack_check(TCB *t)
{
    if(t->stack_start[0] != STACK_PATTERN)
    {
        while(1);
    }
}


/*================ TASK CREATE =================*/

int task_create(void (*task)(void *), uint8_t priority, void *param)
{
    if(task_count >= MAX_TASKS)
        return -1;

    uint32_t id = task_count;

    stack_init(id, task, param);

    tcb_array[id].priority = priority;

    tcb_array[id].state = TASK_READY;

    ready_insert(&tcb_array[id]);

    task_count++;

    return 0;
}


/*================ DELAY =================*/

void os_delay(uint32_t ticks)
{
    __disable_irq();

    current_task->state = TASK_BLOCKED;

    ready_remove(current_task);

    delay_insert(current_task, ticks);

    __enable_irq();

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


/*================ PENDSV =================*/

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(

        "MRS r0, PSP \n"
        "STMDB r0!, {r4-r11} \n"

        "LDR r1, =current_task \n"
        "LDR r2, [r1] \n"
        "STR r0, [r2] \n"

        "PUSH {lr} \n"
        "BL schedule \n"
        "POP {lr} \n"

        "STR r0, [r1] \n"

        "LDR r0, [r0] \n"
        "LDMIA r0!, {r4-r11} \n"

        "MSR PSP, r0 \n"

        "BX lr"
    );
}


/*================ SYSTICK =================*/

void SysTick_Handler(void)
{
    tick_count++;

    while(delay_list && delay_list->delay <= tick_count)
    {
        TCB *t = delay_list;

        delay_list = delay_list->next;

        t->state = TASK_READY;

        ready_insert(t);
    }

    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


/*================ RUNTIME UPDATE =================*/

void runtime_update()
{
    if(current_task)
        current_task->runtime++;
}


/*================ TASK CONTROL =================*/

void os_task_suspend(TCB *t)
{
    __disable_irq();

    ready_remove(t);

    t->state = TASK_SUSPENDED;

    __enable_irq();
}

void os_task_resume(TCB *t)
{
    __disable_irq();

    t->state = TASK_READY;

    ready_insert(t);

    __enable_irq();
}

void os_task_delete(TCB *t)
{
    __disable_irq();

    ready_remove(t);

    t->state = TASK_DELETED;

    __enable_irq();
}


/*================ YIELD =================*/

void os_yield_from_isr(void)
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void os_yield(void)
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}


/*================ IDLE TASK =================*/

void os_watchdog_kick(void)
{
    IWDG->KR = 0xAAAA;
}

void idle_task(void *p)
{
    while(1)
    {
        os_watchdog_kick();
    }
}


/*================ NEXT DELAY =================*/

uint32_t next_delay(void)
{
    if(delay_list == 0)
        return 0xFFFFFFFF;

    return delay_list->delay - tick_count;
}


/*================ START FIRST TASK =================*/

void start_first_task(void)
{
    __asm volatile(

        "LDR r0, =current_task \n"
        "LDR r0, [r0] \n"
        "LDR r0, [r0] \n"

        "MSR PSP, r0 \n"

        "MOV r0,#2 \n"
        "MSR CONTROL,r0 \n"
        "ISB \n"

        "POP {r4-r11} \n"
        "POP {r0-r3} \n"
        "POP {r12} \n"
        "POP {lr} \n"
         "POP {pc}"
    );
}


/*================ OS START =================*/

void os_start(void)
{
    task_create(idle_task, MAX_PRIORITY-1, 0);

    current_task = schedule();

    SysTick_Config(16000000 / TICK_RATE_HZ);

    NVIC_SetPriority(PendSV_IRQn, 255);

    start_first_task();
}
