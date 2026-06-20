#ifndef EVENT_GROUP_H
#define EVENT_GROUP_H

#include "kernel.h"

typedef uint32_t EventBits_t;

typedef struct
{
    EventBits_t bits;
    TCB *wait_list;

} EventGroup_t;

/* create */
void event_group_init(EventGroup_t *eg);

/* set bits */
EventBits_t event_group_set_bits(EventGroup_t *eg, EventBits_t bits);

/* clear bits */
EventBits_t event_group_clear_bits(EventGroup_t *eg, EventBits_t bits);

/* wait bits */
EventBits_t event_group_wait_bits(
        EventGroup_t *eg,
        EventBits_t bits_to_wait,
        uint8_t clear_on_exit,
        uint8_t wait_for_all,
        uint32_t ticks_to_wait);

#endif
