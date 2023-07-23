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
}

void
swap_in (void *user_virt_addr, void *kernel_virt_addr)
{
    if (!is_kernel_vaddr (kernel_virt_addr) || !is_user_vaddr (user_virt_addr))
        return;
    
    for (uint32_t i = 0; i < (PGSIZE / BLOCK_SECTOR_SIZE); i++)
    {
        block_read (swap_block, bitmap_scan_and_flip (swap_map, 0, 1, false), 
                                kernel_virt_addr + (i * BLOCK_SECTOR_SIZE));
    }
    bitmap_set (swap_map, user_virt_addr, true);
}

uint32_t
swap_out (void *user_virt_addr)
{
    if (!is_user_vaddr (user_virt_addr))
        return -1;
    
    uint32_t swap_index = bitmap_scan_and_flip (swap_map, 0, 1, false);
    if (swap_index == BITMAP_ERROR)
        PANIC ("Swap partition is full");
    
    for (size_t i = 0; i < (PGSIZE / BLOCK_SECTOR_SIZE); i++)
    {
        block_write (swap_block, swap_index, user_virt_addr + 
                                        (i * BLOCK_SECTOR_SIZE));
    }
    return swap_index;
}