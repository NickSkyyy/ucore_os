#include <buddy_system.h>
#include <list.h>
#include <pmm.h>
#include <string.h>

const int MAX_LEVEL = 12;
free_area_t free_area[MAX_LEVEL + 1];

static void
bds_init(void) {
    for (int i = 0; i <= MAX_LEVEL; i++) {
        list_init(free_area + i);
        free_area[i].nr_free = 0;
    }
}

static void
bds_inti_memmap(struct Page *base, size_t n) {
    // check info correct
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    // save as buddy    
}