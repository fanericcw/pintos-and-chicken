#ifndef SWAP_H
#define SWAP_H

#include <stdint.h>

#define SECTOR_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

void swap_init (void);
void swap_in (uint32_t swap_idx, void *user_virt_addr);
uint32_t swap_out (void *user_virt_addr);

#endif /* vm/swap.h */