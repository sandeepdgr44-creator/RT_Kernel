#ifndef     RT_QUEUE_H
#define     RT_QUEUE_H



/* queue structure */

typedef struct
{
    uint8_t *buffer;

    uint32_t item_size;
    uint32_t length;

    uint32_t head;
    uint32_t tail;
    uint32_t count;

    TCB *wait_send;
    TCB *wait_recv;

} Queue_t;

/* function prototypes APIs*/
void queue_init(Queue_t *q, uint8_t *buffer,
                uint32_t length,
                uint32_t item_size);

int queue_send(Queue_t *q, void *item, uint32_t ticks);

int queue_receive(Queue_t *q, void *item, uint32_t ticks);

#endif          /*end of rt_queue.h*/




#endif
