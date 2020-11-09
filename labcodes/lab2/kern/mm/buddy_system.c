#include <buddy_system.h>
#include <list.h>
#include <pmm.h>
#include <string.h>

#define MAX_LEVEL 12
free_area_t free_area[MAX_LEVEL + 1];

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
        //cprintf("level is: %d\n", level);
        for (int i = 0; i < temp / (1 << level); i++) {
            p->property = (1 << level);
            SetPageProperty(p);
            list_add_before(&(free_area[level].free_list), &(p->page_link));
            //cprintf("page entry: %08llx\n", p->page_link);
            p += (1 << level);
            free_area[level].nr_free++;   
        }
        //cprintf("number of blocks: %d\n", free_area[level].nr_free);
        temp = temp % (1 << level);
        level--;
    }
    assert(n == bds_nr_free_pages());
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
}

static void
bds_my_merge() {
    int level = 0;
    while (level < MAX_LEVEL) {
        list_entry_t* le = list_next(&(free_area[level].free_list));
        list_entry_t* bfle = list_prev(le);
        while (le != &(free_area[level].free_list)) {
            struct Page* ple = le2page(le, page_link);
            struct Page* pbf = le2page(bfle, page_link);
            bfle = list_next(bfle);
            le = list_next(le);
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
}

static struct Page*
bds_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > bds_nr_free_pages()) return NULL;
    int level = 0;
    while ((1 << level) < n) { level++; }
    if (level > MAX_LEVEL) return NULL;
    cprintf("level is: %d\n", level);
    // get this page and leave buddy system
    list_entry_t* le = list_next(&(free_area[level].free_list));
    struct Page* page = le2page(le, page_link);
    if (page != NULL) {
        SetPageReserved(page);
        // deal with partial work
        bds_my_partial(page + n, page->property - n, level - 1);
        bds_my_merge();
        ClearPageReserved(page);
        ClearPageProperty(page);
        list_del(&(page->page_link));
        free_area[level].nr_free--;
    }
    return page;
}

static void
bds_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
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