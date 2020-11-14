#include <buddy_system.h>
#include <list.h>
#include <pmm.h>
#include <string.h>

#define MAX_LEVEL 12
free_area_t free_area[MAX_LEVEL + 1];

static void
bds_selfcheck(void) {
    cprintf("self-check begin.\n");
    for (int i = 0; i <= MAX_LEVEL; i++) {
        list_entry_t* le = list_next(&(free_area[i].free_list));
        while (le != &(free_area[i].free_list)) {
            cprintf("%d - %llx\n", i, le);
            le = list_next(le);
        }
    }
    cprintf("self-check end.\n");
}

static size_t
bds_nr_free_pages(void) {
    size_t sum = 0;
    for (int i = 0; i <= MAX_LEVEL; i++)
        sum += free_area[i].nr_free * (1 << i);
    return sum;
}

static void
bds_init(void) {
    for (int i = 0; i <= MAX_LEVEL; i++) {
        list_init(&(free_area[i].free_list));
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
    p = base;
    size_t temp = n;        // uesd to calculate 
    int level = MAX_LEVEL;  // 12 as default
    while (level >= 0) {
        for (int i = 0; i < temp / (1 << level); i++) {
            struct Page* page = p;
            page->property = (1 << level);
            SetPageProperty(page);
            list_add_before(&(free_area[level].free_list), &(page->page_link));
            p += (1 << level);         
            free_area[level].nr_free++;   
        }
        temp = temp % (1 << level);
        level--;
    }
    assert(n == bds_nr_free_pages());
    bds_selfcheck();
}

static void
bds_my_partial(struct Page *base, size_t n, int level) {
    if (level < 0) return;
    size_t temp = n;
    while (level >= 0) {
        for (int i = 0; i < temp / (1 << level); i++) {
            base->property = (1 << level);
            SetPageProperty(base);
            // add pages in order
            struct Page* p = NULL;
            list_entry_t* le = list_next(&(free_area[level].free_list));
            list_entry_t* bfle = list_prev(le);
            while (le != &(free_area[level].free_list)) {
                p = le2page(le, page_link);
                if (base + base->property < le) break;
                bfle = bfle->next;
                le = le->next;
            }
            list_add(bfle, &(base->page_link));
            base += (1 << level);
            free_area[level].nr_free++;
        }
        temp = temp % (1 << level);
        level--;
    }
    cprintf("alloc_page check: \n");
    for (int i = MAX_LEVEL; i >= 0; i--) {
        list_entry_t* le = list_next(&(free_area[i].free_list));
        while (le != &(free_area[i].free_list)) {
            struct Page* page = le2page(le, page_link);
            cprintf("%d - %llx\n", i, page->page_link);
            le = list_next(le);
        }
    }
}

static void
bds_my_merge(int level) {
    cprintf("before merge.\n");
    bds_selfcheck();
    while (level < MAX_LEVEL) {
        if (free_area[level].nr_free <= 1) {
            level++;
            continue;
        }
        list_entry_t* le = list_next(&(free_area[level].free_list));
        list_entry_t* bfle = list_prev(le);
        while (le != &(free_area[level].free_list)) {
            bfle = list_next(bfle);
            le = list_next(le);
            struct Page* ple = le2page(le, page_link);
            struct Page* pbf = le2page(bfle, page_link); 
            cprintf("bfle addr is: %llx\n", pbf->page_link);
            cprintf("le addr is: %llx\n", ple->page_link);
            if (pbf + pbf->property == ple) {            
                bfle = list_next(bfle);
                le = list_next(le);
                pbf->property = pbf->property << 1;
                ClearPageProperty(ple);
                list_del(&(pbf->page_link));
                list_del(&(ple->page_link));
                bds_my_partial(pbf, pbf->property, level + 1);             
                free_area[level].nr_free -= 2;              
                continue;
            } 
        }
        level++;
    }
    //bds_selfcheck();
}

static struct Page*
bds_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > bds_nr_free_pages()) return NULL;
    int level = 0;
    while ((1 << level) < n) { level++; }
    while (level <= MAX_LEVEL && free_area[level].nr_free == 0) { level++; }
    if (level > MAX_LEVEL) return NULL;
    // get this page and leave buddy system
    list_entry_t* le = list_next(&(free_area[level].free_list));
    struct Page* page = le2page(le, page_link);
    if (page != NULL) {
        SetPageReserved(page);
        // deal with partial work
        bds_my_partial(page + n, page->property - n, level - 1);
        ClearPageReserved(page);
        ClearPageProperty(page);
        list_del(&(page->page_link));
        free_area[level].nr_free--;
        bds_my_merge(0);
    }
    cprintf("after allocate & merge\n");
    bds_selfcheck();
    return page;
}

static void
bds_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page* p = base;
    for (; p != base + n; p++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
    // free pages
    base->property = n;
    SetPageProperty(base);
    int level = 0;
    while ((1 << level) != n) { level++; }
    bds_my_partial(base, n, level);
    bds_selfcheck();
    free_area[level].nr_free++;
    bds_my_merge(level); 
    bds_selfcheck();
}

static void
bds_check(void) {
    int count = 0, total = 0;
    for (int i = 0; i <= MAX_LEVEL; i++) {
        list_entry_t* free_list = &(free_area[i].free_list);
        list_entry_t* le = free_list;
        while ((le = list_next(le)) != free_list) {
            struct Page* p = le2page(le, page_link);
            assert(PageProperty(p));
            count++;
            total += p->property;
        }
    }
    assert(total == bds_nr_free_pages());

    // basic check
    struct Page *p0, *p1, *p2;
    p0 = p1 =p2 = NULL;
    cprintf("p0\n");
    assert((p0 = alloc_page()) != NULL);
    cprintf("p1\n");
    assert((p1 = alloc_page()) != NULL);
    cprintf("p2\n");
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p1 != p2 && p2 != p0);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);
    cprintf("first part of check successfully.\n");

    free_area_t temp_list[MAX_LEVEL + 1];
    for (int i = 0; i <= MAX_LEVEL; i++) {
        temp_list[i] = free_area[i];
        list_init(&(free_area[i].free_list));
        assert(list_empty(&(free_area[i])));
        free_area[i].nr_free = 0;
    }
    assert(alloc_page() == NULL);
    cprintf("clean successfully.\n");
    cprintf("p0\n");
    free_page(p0);
    cprintf("p1\n");
    free_page(p1);
    cprintf("p2\n");
    free_page(p2);
    total = 0;
    for (int i = 0; i <= MAX_LEVEL; i++) 
        total += free_area[i].nr_free;
    //assert(total == 3);

    //assert((p0 = alloc_page()) != NULL);
    //assert((p1 = alloc_page()) != NULL);
    //assert((p2 = alloc_page()) != NULL);
    //assert(alloc_page() == NULL);
}

const struct pmm_manager buddy_system = {
    .name = "buddy_system",
    .init = bds_init,
    .init_memmap = bds_inti_memmap,
    .alloc_pages = bds_alloc_pages,
    .free_pages = bds_free_pages,
    .nr_free_pages = bds_nr_free_pages,
    .check = bds_check,
};