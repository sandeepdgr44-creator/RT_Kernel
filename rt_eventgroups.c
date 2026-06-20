#include "event_group.h"
extern TCB *current_task;
void event_group_init(EventGroup_t *eg)
{
    eg->bits = 0;
    eg->wait_list = 0;
}

EventBits_t event_group_set_bits(EventGroup_t *eg, EventBits_t bits)
{
    __disable_irq();

    eg->bits |= bits;

    TCB *t = eg->wait_list;
    TCB *prev = 0;

    while(t)
    {
        TCB *next = t->next;

        /* wake task */
        t->state = TASK_READY;
        ready_insert(t);

        if(prev)
            prev->next = next;
        else
            eg->wait_list = next;

        t = next;
    }

    __enable_irq();

    os_yield();

    return eg->bits;
}

EventBits_t event_group_clear_bits(EventGroup_t *eg, EventBits_t bits)
{
    __disable_irq();

    eg->bits &= ~bits;

    __enable_irq();

    return eg->bits;
}

EventBits_t event_group_wait_bits(
        EventGroup_t *eg,
        EventBits_t bits_to_wait,
        uint8_t clear_on_exit,
        uint8_t wait_for_all,
        uint32_t ticks)
{
    __disable_irq();

    EventBits_t bits = eg->bits;

    uint8_t condition;

    if(wait_for_all)
        condition = ((bits & bits_to_wait) == bits_to_wait);
    else
        condition = (bits & bits_to_wait);

    if(!condition)
    {
        current_task->state = TASK_BLOCKED;

        ready_remove(current_task);

        current_task->next = eg->wait_list;

        eg->wait_list = current_task;

        if(ticks)
            delay_insert(current_task, ticks);

        __enable_irq();

        os_yield();

        bits = eg->bits;
    }
    else
    {
        __enable_irq();
    }

    if(clear_on_exit)
        eg->bits &= ~bits_to_wait;

    return bits;
}
