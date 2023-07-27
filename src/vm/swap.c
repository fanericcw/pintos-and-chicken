#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include <stdint.h>

struct block *swap_block;
struct bitmap *swap_map;

void
swap_init (void)
{
    swap_block = block_get_role (BLOCK_SWAP);
    if (swap_block == NULL)
        PANIC ("Swap block device not found");
    swap_map = bitmap_create (block_size (swap_block) / (PGSIZE / BLOCK_SECTOR_SIZE));
    if (swap_map == NULL)
        PANIC ("Swap bitmap allocation failed");
    bitmap_set_all (swap_map, true);
}

void
swap_in (uint32_t swap_idx, void *user_virt_addr)
{   
    for (uint32_t i = 0; i < SECTOR_PER_PAGE; i++)
    {
        block_read (swap_block, swap_idx * SECTOR_PER_PAGE + i, 
                                user_virt_addr + (i * BLOCK_SECTOR_SIZE));
    }
    bitmap_flip (swap_map, swap_idx);
}

uint32_t
swap_out (void *user_virt_addr)
{
    if (!is_user_vaddr (user_virt_addr))
        return SIZE_MAX;
    
    uint32_t swap_idx = bitmap_scan_and_flip (swap_map, 0, 1, true);
    if (swap_idx == BITMAP_ERROR)
        PANIC ("Swap partition is full");
    
    for (size_t i = 0; i < (PGSIZE / BLOCK_SECTOR_SIZE); i++)
    {
        block_write (swap_block, swap_idx * SECTOR_PER_PAGE + i, 
            user_virt_addr + (i * BLOCK_SECTOR_SIZE));
    }
    return swap_idx;
}