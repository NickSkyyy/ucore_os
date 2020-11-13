#include <pmm.h>
#include <list.h>
#include <string.h>
#include <default_pmm.h>
#include <buddysys_pmm.h>

#define lson(i) ((i)<<1)
#define rson(i) ((i)<<1|1)
#define fath(i) ((i)>>1)

#define is_2power(x) (!((x)&((x)-1)))
#define max(a,b) ((a)>(b)?(a):(b))

#define UINT32_SHR_OR(a,n)      ((a)|((a)>>(n)))
#define UINT32_MASK(a)          (UINT32_SHR_OR(UINT32_SHR_OR(UINT32_SHR_OR(UINT32_SHR_OR(UINT32_SHR_OR(a,1),2),4),8),16))    
#define UINT32_REMAINDER(a)     ((a)&(UINT32_MASK(a)>>1))
#define UINT32_ROUND_DOWN(a)    (UINT32_REMAINDER(a)?((a)-UINT32_REMAINDER(a)):(a))//小于a的最大的2^k

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

#define NODE_MAX_NUM 80000

struct node{
	unsigned int size;
	unsigned int longest;//important mark in buddy sys
}buddytree[NODE_MAX_NUM];

free_area_t free_area;

struct allocrecord{
	struct Page* base;
	int offset;
	size_t nr;
}rec[NODE_MAX_NUM];//array stores offset
int nr_block;//block num has been allocated

int fixsize(int n){
    if(n==0) return 1;
    else return (UINT32_ROUND_DOWN(n)<<1);
}

static void
buddysys_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

void buddysys_new(int size){
	unsigned int node_size;
	nr_block=0;
	if(size<1||!is_2power(size)) return;

	buddytree[1].size=size;
	node_size=size*2;

	for(int i=1;i<(size<<1);i++){
		if(is_2power(i)) node_size>>=1;
		buddytree[i].longest=node_size;
	}
}

static void
buddysys_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 1;
        SetPageProperty(p);
        set_page_ref(p, 0);
        list_add_before(&free_list,&(p->page_link));
    }
    nr_free += n;
    buddysys_new(UINT32_ROUND_DOWN(n));
}

int buddysys_alloc(struct node* self,int size){
	unsigned index=1;
	unsigned node_size;
	unsigned offset=0;

	if((self+1)==NULL) return -1;
	if(size<=0) size=1;
	else if(!is_2power(size)) size=fixsize(size);

	if(self[index].longest<size) return -1;
	for(node_size=self[1].size;node_size!=size;node_size>>=1){
		if(self[lson(index)].longest>=size){
			if(self[rson(index)].longest>=size){
				index=self[lson(index)].longest<=self[rson(index)].longest?lson(index):rson(index);
			}
			else index=lson(index);
		}
		else index=rson(index);
	}

	self[index].longest=0;
	offset=index*node_size-self[1].size;

	while(index){
		index=fath(index);
        if(index==0) break;
		self[index].longest=max(self[lson(index)].longest,self[rson(index)].longest);
	}
	return offset;
}

static struct Page *
buddysys_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL;
    struct Page *p;
    list_entry_t *le = &free_list,*len;
    rec[nr_block].offset=buddysys_alloc(buddytree,n);

    //find the head page of this block via offset
    for(int i=0;i<rec[nr_block].offset+1;i++) {le=list_next(le);}
    page=le2page(le,page_link);
	int allocpages;
	if(!is_2power(n)) allocpages=fixsize(n);
	else allocpages=n;

    rec[nr_block].base=page;//a pointer to head page
    rec[nr_block].nr=allocpages;//page num to allocate
    nr_block++;//blocks have been allocated

    for(int i=0;i<allocpages;i++){
    	len=list_next(le);
    	p=le2page(le,page_link);
    	ClearPageProperty(p);
    	le=len;
    }

    nr_free-=allocpages;
    page->property=n;

    return page;
}

static void
buddysys_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    unsigned int node_size,index;
    unsigned int lson_longest,rson_longest;
    struct node* self=buddytree;

    list_entry_t *le=list_next(&free_list);
    
    //to find out which block match this para: head page "base"
    int match=0;
    for(match=0;match<nr_block;match++){
    	if(rec[match].base==base) break;
    }

    int offset=rec[match].offset;
    for(int i=0;i<offset;i++){
    	le=list_next(le);
    }

    int freepages;
    if(!is_2power(n)) freepages=fixsize(n);
    else freepages=n;

    assert((self+1)!=NULL);
    assert(offset>=0);
    assert(offset<(self+1)->size);

    node_size = 1;
    index = (offset + self[1].size);
    //substitute code_size=1 into "offset=index*node_size-self->size"
    nr_free+=freepages;

    //some question remaining
    //why node_size equals to 1? disobey wuwenbin
    struct Page* p;
    self[index].longest=freepages;
    for(int i=0;i<freepages;i++){
    	p=le2page(le,page_link);
    	p->flags=0;
    	p->property=1;
    	SetPageProperty(p);
    	le=list_next(le);
    }

    while(index){
    	index=fath(index);
        if(index==0) break;
    	node_size<<=1;

    	lson_longest=self[lson(index)].longest;
    	rson_longest=self[rson(index)].longest;

    	if(lson_longest+rson_longest==node_size){
    		self[index].longest=node_size;
    	}
    	else self[index].longest=max(lson_longest,rson_longest);
    }

    //delete record rec[match] from array rec
    for(int i=match;i<nr_block-1;i++){
    	rec[i]=rec[i+1];
    }
    nr_block--;
}

static size_t
buddysys_nr_free_pages(void) {
    return nr_free;
}

static void
buddysys_check(void) {
    struct Page *p0, *A,*B,*C,*D;
    p0 = A = B = C = D = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((A = alloc_page()) != NULL);
    assert((B = alloc_page()) != NULL);

    cprintf("alloc single page to p0,A,B:\n");
    cprintf("p0,A,B: %p %p %p\n",p0,A,B);
    cprintf("A-p0,B-A: %p %p\n",A-p0,B-A);

    assert(p0 != A && p0 != B && A != B);
    assert(page_ref(p0) == 0 && page_ref(A) == 0 && page_ref(B) == 0);

    cprintf("free single page to p0,A,B\n");

    free_page(p0);
    free_page(A);
    free_page(B);

    // =====500(512)A====|=====500(512)B===== 
    cprintf("allocate 500 pages to A and B:\n");
    A=alloc_pages(500);
    cprintf("now free page num is: %d\n",nr_free);
    B=alloc_pages(500);
    cprintf("now free page num is: %d\n",nr_free);
    cprintf("A %p\n",A);
    cprintf("B %p\n",B);

    cprintf("now free 250 pages to A, 500 pages to B, 250 pages to A+250:\n");
    free_pages(A,250);
    // ===FREE===|==(256)A==|=====500(512)B===== 
    free_pages(B,500);
    // ===FREE===|==(256)A==|=======FREE======== 
    free_pages(A+250,250);
    // ==============FREE=======================
    cprintf("if process right, the pages shall all br free!\n");
    
    p0=alloc_pages(1024);
    //  ================p0======================
    cprintf("allocate 1024 pages to p0, because all pages are free, the pointer p0 must equal to A!\n");
    cprintf("p0 %p\n",p0);
    assert(p0 == A);

    // cprintf("allocate 70 pages to A, 35 pages to B, A+128 shall equal to B!\n");
    // A=alloc_pages(70); 
    // B=alloc_pages(35);
    // assert(A+128==B);//检查是否相邻
    // //  ===70(128)A===|==35(64)B==|===64FREE===|=====================FREE=================
    // cprintf("A %p\n",A);
    // cprintf("B %p\n",B);

    // cprintf("allocate 80 pages to C, A+256 shall equal to C!\n");
    // C=alloc_pages(80);
    // assert(A+256==C);//检查C有没有和A重叠
    // //  ===70(128)A===|==35(64)B==|===64FREE===|=========80(128)C==========|=========free==========
    // cprintf("C %p\n",C);
    // free_pages(A,70);//释放A
    // //  =====FREE=====|==35(64)B==|===64FREE===|=========80(128)C==========|=========free==========
    // cprintf("B %p\n",B);
    // D=alloc_pages(60);
    // //  =====FREE=====|==35(64)B==|===60(64)D==|=========80(128)C==========|=========free==========
    // cprintf("D %p\n",D);
    // assert(B+64==D);//检查B，D是否相邻
    // free_pages(B,35);
    // cprintf("D %p\n",D);
    // free_pages(D,60);
    // cprintf("C %p\n",C);
    // free_pages(C,80);
    // //  =====FREE=====|====FREE===|====FREE====|=========FREE==========|=========free==========
    free_pages(p0,1000);//全部释放
    //  ========================================FREE===========================================
}

const struct pmm_manager buddysys_pmm_manager = {
    .name = "buddysys_pmm_manager",
    .init = buddysys_init,
    .init_memmap = buddysys_init_memmap,
    .alloc_pages = buddysys_alloc_pages,
    .free_pages = buddysys_free_pages,
    .nr_free_pages = buddysys_nr_free_pages,
    .check = buddysys_check,
};

