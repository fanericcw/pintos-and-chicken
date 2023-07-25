#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/list.h"
#include <stdint.h>

/* State of spt */
#define ZERO 0
#define FRAME 1
#define SWAP 2

struct spte
{
    void *user_virt_addr;
    uint8_t state;
    struct list_elem elem;
};

void spt_destroy (struct list *);
bool spte_set_page (struct list *, void *);

#endif /* vm/page.h */