#include <stdio.h>
#include <stdlib.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

// Destroys entire SPT for a thread
void spt_destroy (struct list *spt) {
  ASSERT(spt != NULL)
  struct thread *cur = thread_current();

  struct list_elem *e;
  for (e = list_begin(spt); e != list_end(spt);
  e = e) 
  {
    struct spte *entry = list_entry(e, struct spte, elem);
    free_frame(pagedir_get_page(cur->pagedir, entry->user_virt_addr));
		pagedir_clear_page(cur->pagedir, entry->user_virt_addr);
  }
}

bool
spte_set_page (struct list *spt, void *upage)
{
  struct spte *spte;
  spte = (struct spte *) malloc(sizeof(struct spte));
  bool in_list = false;  // upage already inserted?

  spte->user_virt_addr = upage;
  spte->state = FRAME;

  struct list_elem *e;
  for (e = list_begin(spt); e != list_end(spt); e = list_next(e))
  {
    struct spte *entry = list_entry(e, struct spte, elem);
    if (entry->user_virt_addr == upage) {
      in_list = true;
      break;
    }
  }

  if (!in_list) {
    list_push_back(spt, &spte->elem);
    // successfully inserted into the supplemental page table.
    return true;
  }
  else {
    // failed. there is already an entry.
    free (spte);
    return false;
  }
}

struct spte *page_lookup(void *user_virt_addr) {
  struct list *spt = &thread_current()->spt;
  struct list_elem *e;
  for (e = list_begin(spt); e != list_end(spt); e = list_next(e)) {
    struct spte *entry = list_entry(e, struct spte, elem);
    if (entry->user_virt_addr == user_virt_addr)
      return entry;
  }
  return NULL;    // vaddr not found as page
}

// Checks whether vaddr is valid
bool vaddr_is_valid(void * vaddr)
{
  if (page_lookup(vaddr) == NULL)
    return false;
  return true;
}

bool 
load_page (struct list *spt, uint32_t pd, void *user_virt_addr)
{   
  // Not valid user vaddr
  if (!vaddr_is_valid (user_virt_addr))
    return false;
  struct spte *spte = page_lookup(user_virt_addr);
  // Address not in spt
  if (spte == NULL)
    return false;
 
  void *kpage = allocate_frame(PAL_USER, user_virt_addr);
  // Failed to create a single page
  if (kpage == NULL)
    return false;
  
  switch (spte->state)
  {
    case ZERO:
        memset (kpage, 0, PGSIZE);
        break;
    case FRAME:
        /* Do nothing */
        break;
    case SWAP:
        PANIC("Swapping not yet implemente");
        break;
    
    default:
        break;
  }

  if(!pagedir_set_page (thread_current()->pagedir, user_virt_addr, kpage, true)) 
  {
      free_frame (kpage);
      return false;
  }
  return true;
} 