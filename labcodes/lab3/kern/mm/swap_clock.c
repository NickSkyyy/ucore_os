#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_clock.h>
#include <list.h>

/*
 * manage all swappable pages, so we can
 * link these pages into pra_list_head according the time order.
 */
list_entry_t pra_list_head;

static int
_clk_tick_event(struct mm_struct *mm)
{
    pte_t* ptep = NULL;
    list_entry_t *start, *next;
    start = (list_entry_t* )mm->sm_priv;
    next = list_next(start);
    cprintf("after positive swap\n");
    while(next != start){
        struct Page* p = le2page(next, pra_page_link);
        ptep = get_pte(mm->pgdir, p->pra_vaddr, 0);//get_pte(mm->pgdir, page2(p), 0);
        assert((*ptep & PTE_P) != 0);
        *ptep = *ptep & (~PTE_A);   // accessed bit is set to 0
        cprintf("addr is: 0x%llx\n", p->pra_vaddr);
        cprintf("accessed: %d, dirty: %d\n", 
        ((*ptep & PTE_A) == 0 ? 0 : 1),
        ((*ptep & PTE_D) == 0 ? 0 : 1));
        tlb_invalidate(mm->pgdir, p->pra_vaddr);
        //cprintf("after %d\n", *ptep);
        next = list_next(next); //move to next page
    }
    //CODE HERE
    return 0; 
}

/*
_* _clk_init_mm: init pra_list_head and let  mm->sm_priv point to 
 * the addr of pra_list_head. Now, From the memory control struct 
 * mm_struct, we can access FIFO PRA
 */
static int 
_clk_init_mm(struct mm_struct * mm)
{
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    cprintf(" mm->sm_priv %x in fifo_init_mm\n",mm->sm_priv);
    return 0;
}

static int
_clk_map_swappable(struct mm_struct* mm, uintptr_t addr, struct Page* page, int swap_in)
{
    list_entry_t *head = (list_entry_t*) mm->sm_priv;
    list_entry_t * entry = &(page->pra_page_link);

    assert(entry != NULL && head!=NULL);
    //pte_t * ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
    //    assert((*ptep & PTE_P) != 0);
    //link the most recent arrival page at the back of queue
    list_add_before(head, entry);
    return 0;
}

static int
_clk_swap_out_victim(struct mm_struct* mm, struct Page ** ptr_page, int in_tick){
    pte_t* ptep = NULL;
    list_entry_t *start, *next, *le1, *le2, *le3, *le4;
    assert(in_tick==0);             //make sure won't meet tick
    start = (list_entry_t* )(mm->sm_priv);
    next = list_next(start);
    struct Page* page_l1 = NULL, *page_l2 = NULL, 
                *page_l3 = NULL, *page_l4 = NULL;
    //cprintf("ckpt1\n");
    while(next != start){
        struct Page* p = le2page(next, pra_page_link);
        ptep = get_pte(mm->pgdir, p->pra_vaddr, 0);
        //ptep = get_pte(mm->pgdir, page2pa(p), 0);
        assert(((*ptep) & PTE_P) != 0);
      //  cprintf("get_pte sucess\n");
        bool accessed = (((*ptep) & PTE_A) != 0);   // accessed bit
        bool dirty = (((*ptep) & PTE_D) != 0);   // accessed bit
        cprintf("access %d, dirty %d \n", accessed, dirty);
        cprintf("%p, %d\n", p, p->flags);
        if (!accessed && !dirty && page_l1 == NULL)
        {
            page_l1 = le2page(next, pra_page_link);
            le1 = next;
            break;
        }
        else if (!accessed && dirty && page_l2 == NULL)
        {
            page_l2 = le2page(next, pra_page_link);
            le2 = next;
        }
        else if (accessed && !dirty && page_l3 == NULL)
        {              
            page_l3 = le2page(next, pra_page_link);
            le3 = next;
        }
        else if (accessed && dirty && page_l4 == NULL)
        {                
            page_l4 = le2page(next, pra_page_link);
            le4 = next;
        }
      //  cprintf("ckpt2\n");
        next = list_next(next); //move to next page
    }
    //cprintf("swapping out\n");
    if (page_l1){
        //uintptr_t v = page_l1->pra_vaddr;
        //cprintf("%p\n",v); 
        *ptr_page = (struct Page* )page_l1;
        //cprintf("ok1\n");
        //cprintf("%p\n", page_l1);
        list_del(le1);
        return 0;
    }
    //cprintf("no l1\n");
    if (page_l2){
        uintptr_t v = page_l2->pra_vaddr;
        //cprintf("%d\n",v);
        *ptr_page = (struct Page* )page_l2;

        list_del(&(page_l2->pra_page_link));
        return 0;
    }
    if (page_l3){
      //  cprintf("%p\n",page_l3->pra_vaddr);
        *ptr_page = page_l3;
        list_del(&(page_l3->pra_page_link));
        return 0;
    }
    if (page_l4){
        //cprintf("%p\n",page_l4->pra_vaddr);
        *ptr_page = page_l4;
        list_del(&(page_l4->pra_page_link));
        // // deal with all dirty
        // start = (list_entry_t* )(mm->sm_priv);
        // next = list_next(start);
        // while (next != start) {
        //     struct Page* p = le2page(next, pra_page_link);
        //     ptep = get_pte(mm->pgdir, p->pra_vaddr, 0);
        //     *ptep = *ptep & (~PTE_A);
        //     next = list_next(next);
        // }
        return 0;
    }else{
        cprintf("something wrong when victim\n");
        *ptr_page = NULL;
        return 1;
    }
}


static int
_clk_check_swap(void){
    extern struct mm_struct *check_mm_struct;
    cprintf("write Virt Page c in clk_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==4);
    cprintf("write Virt Page a in clk_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==4);
    cprintf("write Virt Page d in clk_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==4);
    cprintf("write Virt Page b in clk_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==4);
    cprintf("write Virt Page e in clk_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==5);
    _clk_tick_event(check_mm_struct);

    // b c d e(first: b, Acc/D: 0 1)
    cprintf("write Virt Page b in fifo_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==5);
    cprintf("write Virt Page a in fifo_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==6);
    cprintf("write Virt Page b in fifo_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    assert(pgfault_num==6);
    cprintf("write Virt Page c in fifo_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    assert(pgfault_num==7);
    _clk_tick_event(check_mm_struct);
    cprintf("write Virt Page d in fifo_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    assert(pgfault_num==8);
    cprintf("write Virt Page e in fifo_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    assert(pgfault_num==8);
    cprintf("write Virt Page a in fifo_check_swap\n");
    assert(*(unsigned char *)0x1000 == 0x0a);
    *(unsigned char *)0x1000 = 0x0a;
    assert(pgfault_num==8);
    return 0;
}

static int
_clk_init(void)
{
    return 0;
}

static int
_clk_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

struct swap_manager swap_manager_clk = 
{
    .name               = "extend clock swap manager",
    .test_mm            = NULL,
    .init               = &_clk_init,
    .init_mm            = &_clk_init_mm,
    .tick_event         = &_clk_tick_event,
    .map_swappable      = &_clk_map_swappable,
    .set_unswappable    = &_clk_set_unswappable,
    .swap_out_victim    = & _clk_swap_out_victim,
    .check_swap         = & _clk_check_swap
};