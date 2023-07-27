#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "lib/kernel/list.h"
#include <stdint.h>

/* State of spt */
#define ZERO 0
#define FRAME 1
#define SWAP 2

/* Max Stack size */
#define MAX_STACK 8 * 1024 * 1024

struct spte
{
    void *user_virt_addr;
    uint8_t state;
    struct list_elem elem;
};

void spt_destroy (struct list *);
bool spte_set_page (struct list *, void *);
struct spte *page_lookup(void *);
bool vaddr_is_valid(void *);
bool load_page (struct list *, uint32_t, void *);

#endif /* vm/page.h */