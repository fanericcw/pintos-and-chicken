#ifndef SWAP_H
#define SWAP_H

#include <stdint.h>

void swap_init (void);
void swap_in (void *user_virt_addr, void *kernel_virt_addr);
uint32_t swap_out (void *user_virt_addr);

#endif /* vm/swap.h */