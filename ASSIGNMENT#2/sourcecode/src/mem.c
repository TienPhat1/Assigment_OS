
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
	#include <stdio.h>

	static BYTE _ram[RAM_SIZE]; //Array size ram is RAM_SIZE = 1 ^ (ADDRESS_SIZE)


	static struct {
		uint32_t proc;	// ID of process currently uses this page
		int index;	// Index of the page in the list of pages allocated
				// to the process.
		int next;	// The next page in the list. -1 if it is the last
				// page.
	} _mem_stat[NUM_PAGES];

	static pthread_mutex_t mem_lock;
	static pthread_mutex_t ram_lock;
	/*Handle asynchronous*/


	void init_mem(void) {
		memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
		memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
		pthread_mutex_init(&mem_lock, NULL);
	}

	/* get offset of the virtual address */
	static addr_t get_offset(addr_t addr) {							//GET 10 bit last of the address
		return addr & ~((~0U) << OFFSET_LEN);
	}

	/* [addr(21bit)] = [segment(5bit)]_[page(5bit)]_[offset(10bit)]
		=>[segment(5bit)] = addr >> ([page(5bit)]+[offset(10bit)])
	*/

	/* get the first layer index */ 
	static addr_t get_first_lv(addr_t addr) {
		return addr >> (OFFSET_LEN + PAGE_LEN);
	}

	/* [addr(21bit)] = [segment(5bit)]_[page(5bit)]_[offset(10bit)]
		=>[segment(5bit)] = [addr(20bit)] >> ([page(5bit)]+[offset(10bit)])
		=>[segment(5bit)]+[page(5bit)] = [addr(20bit)] >> [offset(10bit)]
		=>[page(5bit)] = [segment(5bit)]+[page(5bit)] - [segment(5bit)]
	*/

	/* get the second layer index */ //get page index
	static addr_t get_second_lv(addr_t addr) {
		return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
	}

	/* Search for page table table from the a segment table */
	static struct page_table_t * get_page_table(
			addr_t index, 	// Segment level index
			struct seg_table_t * seg_table) { // first level table
		
		/*
		* TODO: Given the Segment index [index], you must go through each
		* row of the segment table [seg_table] and check if the v_index
		* field of the row is equal to the index
		*
		* */

		int i;																								/**/
		for (i = 0; i < seg_table->size; i++) {																/**/
			// Enter your code here																			/**/
			if(seg_table->table[i].v_index == index){														/**/
				return seg_table->table[i].pages;															/**/
			}
		}
		return NULL;

	}
/*
	static int remove_page_table(addr_t index , struct seg_table_t * seg_table){

		if(seg_table == NULL) return 1;

		for(int i = 0; i < seg_table->size ; i++)
			{
				if(seg_table->table[i].v_index == index){
					int indx = --seg_table->size;
					seg_table->table[i] = seg_table->table[indx];
					seg_table->table[i].v_index = 0;
					free(seg_table->table[indx].pages);
					return 1;
				}
			}
		return 0;
	}
*/
static int remove_page_table(addr_t v_segment, struct seg_table_t * seg_table) {
	if (seg_table == NULL) return 0;
	int i;
	for (i = 0; i < seg_table->size; i++) {
		if (seg_table->table[i].v_index == v_segment) {
			int idx = seg_table->size-1;
			seg_table->table[i] = seg_table->table[idx];
			seg_table->table[idx].v_index = 0;
			free(seg_table->table[idx].pages);
			seg_table->size--;
			return 1;
		}
	}
	return 0;
}
	/* Translate virtual address to physical address. If [virtual_addr] is valid,
	* return 1 and write its physical counterpart to [physical_addr].
	* Otherwise, return 0 */
	static int translate(
			addr_t virtual_addr, 	// Given virtual address
			addr_t * physical_addr, // Physical address to be returned
			struct pcb_t * proc) {  // Process uses given virtual address

		/* Offset of the virtual address */
		addr_t offset = get_offset(virtual_addr);
		/* The first layer index */
		addr_t first_lv = get_first_lv(virtual_addr);
		/* The second layer index */
		addr_t second_lv = get_second_lv(virtual_addr);
		
		/* Search in the first level */
		struct page_table_t * page_table = NULL;
		page_table = get_page_table(first_lv, proc->seg_table);
		if (page_table == NULL) {
			return 0;
		}

		int i;
		for (i = 0; i < page_table->size; i++) {
			if (page_table->table[i].v_index == second_lv) {
				/* TODO: Concatenate the offset of the virtual addess
				* to [p_index] field of page_table->table[i] to 
				* produce the correct physical address and save it to
				* [*physical_addr]  */
				addr_t p_index = page_table->table[i].p_index;
				*physical_addr = (p_index << OFFSET_LEN) | offset;							/**/
				return 1;
			}
		}
		return 0;	
	} 
	//Checking amount memory to allocted for process
int check_amount_memmory(struct pcb_t * proc , uint32_t num_pages){											
	uint32_t number_mem_free = 0;
											
	//check physical memory																		
	for(int i = 0 ; i < NUM_PAGES ; i++){																		
		if(_mem_stat[i].proc == 0){																			        														/**/
			if(++number_mem_free == num_pages)																	
				break;																					
		}																									
	}
	if(number_mem_free < num_pages) return 0;																		
	//check virtual memory																					
	if(proc->bp + num_pages*PAGE_SIZE >= RAM_SIZE) return 0;													
	return 1;																								
}

void alloc_ret_mem(addr_t ret_mem, uint32_t num_pages, struct pcb_t * proc){								
		int count_pages = 0;	//count pages																				
		int last_alloc_pages = -1;			//get [next] = -1 at last page																	
		for(int i = 0 ; i< NUM_PAGES ; i++)																			
		{																													
			if(_mem_stat[i].proc != 0) continue; //page have used												
			_mem_stat[i].proc = proc->pid;			// the page is used by process																
			_mem_stat[i].index = count_pages;		//index list of allocated pages															
			if(last_alloc_pages > -1){																			
				_mem_stat[last_alloc_pages].next = i;		//update last page													
			}																									
			last_alloc_pages = i;			//update last page																
			//find and creat virtual page_table																					
			addr_t virtual_address = ret_mem + count_pages*PAGE_SIZE;											
			addr_t segment_addr = get_first_lv(virtual_address);												
			addr_t page_addr = get_second_lv(virtual_address);
			struct page_table_t* v_page_table = get_page_table(segment_addr, proc->seg_table);
			if(v_page_table == NULL){
				int indx = proc->seg_table->size++;
				proc->seg_table->table[indx].v_index = segment_addr;
				v_page_table 
				= proc->seg_table->table[indx].pages 
				= (struct page_table_t*)malloc(sizeof(struct page_table_t));
			}
			int idx = v_page_table->size++;
			v_page_table->table[idx].v_index = page_addr;
			v_page_table->table[idx].p_index = i;
			if(++count_pages == num_pages){
				_mem_stat[i].next = -1; break;
			}

		}
	}


addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
		pthread_mutex_lock(&mem_lock);
		addr_t ret_mem = 0;
		/* TODO: Allocate [size] byte in the memory for the
		* process [proc] and save the address of the first
		* byte in the allocated memory region to [ret_mem].
		* */

		uint32_t num_pages = ((size % PAGE_SIZE) == 0) ? size / PAGE_SIZE :
			size / PAGE_SIZE + 1; // Number of pages we will use
		int mem_avail = 0; // We could allocate new memory region or not?

		/* First we must check if the amount of free memory in
		* virtual address space and physical address space is
		* large enough to represent the amount of required 
		* memory. If so, set 1 to [mem_avail].
		* Hint: check [proc] bit in each page of _mem_stat
		* to know whether this page has been used by a process.
		* For virtual memory space, check bp (break pointer).
		* */
		mem_avail = check_amount_memmory(proc,num_pages);															/**/
		if (mem_avail) {
			/* We could allocate new memory region to the process */
			ret_mem = proc->bp;	//start allocate from break pointer in virtual memory
			proc->bp += num_pages * PAGE_SIZE; // allocate memory to process in virtual memory
			/* Update status of physical pages which will be allocated
			* to [proc] in _mem_stat. Tasks to do:
			* 	- Update [proc], [index], and [next] field
			* 	- Add entries to segment table page tables of [proc]
			* 	  to ensure accesses to allocated memory slot is
			* 	  valid. */
			alloc_ret_mem(ret_mem,num_pages,proc);
		}
		if (1) {
			puts("-----------==Allocation==------------");
			dump();
		}
		pthread_mutex_unlock(&mem_lock);
		return ret_mem;
	}

int free_mem(addr_t address, struct pcb_t * proc) {
	/**
	 * TODO: Release memory region allocated by [proc]. 
	 * The first byte of this region is indicated by [address]. 
	 * Tasks to do:
	 * 	+ Set flag [proc] of physical page use by the memory block 
	 * 	+ back to zero to indicate that it is free.
	 * 	+ Remove unused entries in segment table and page tables of the process [proc].
	 * 	+ Remember to use lock to protect the memory from other processes.
	 */
	pthread_mutex_lock(&mem_lock);
	
	addr_t v_address = address;	// virtual address to free in process
	addr_t p_address = 0;		// physical address to free in memory
	
	// Find physical page in memory
	if (!translate(v_address, &p_address, proc)) return 1;
	
	// Clear physical page in memory
	addr_t p_segment_page_index = p_address >> OFFSET_LEN; 
	int num_pages = 0; // number of pages in list
	int i;
	for (i=p_segment_page_index; i!=-1; i=_mem_stat[i].next) {
		num_pages++;
		_mem_stat[i].proc = 0; // clear physical memory
	}
	
	// Clear virtual page in process
	for (i = 0; i < num_pages; i++) {
		addr_t v_addr = v_address + i * PAGE_SIZE;
		addr_t v_segment = get_first_lv(v_addr);
		addr_t v_page = get_second_lv(v_addr);
		// printf("- pid = %d ::: v_segment = %d, v_page = %d\n", proc->pid, v_segment, v_page);
		struct page_table_t * page_table = get_page_table(v_segment, proc->seg_table);
		if (page_table == NULL) {
			puts("Error");
			continue;
		}
		int j;
		for (j = 0; j < page_table->size; j++) {
			if (page_table->table[j].v_index == v_page) {
				int last = --page_table->size;
				page_table->table[j] = page_table->table[last];
				break;
			}
		}
		if (page_table->size == 0) {
			remove_page_table(v_segment, proc->seg_table);
		}
	}
	// Update break pointer
	proc->bp = proc->bp - num_pages*PAGE_SIZE;
	if (1) {
		puts("-----------==Deallocation==-----------");
		dump();
	}
	pthread_mutex_unlock(&mem_lock);
	return 0;
}

	int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
		addr_t physical_addr;
		if (translate(address, &physical_addr, proc)) {
			pthread_mutex_lock(&ram_lock);
			*data = _ram[physical_addr]; //read data from physical memory which have address_addr
			pthread_mutex_unlock(&ram_lock);
			return 0;
		}else{
			return 1;
		}
	}

	int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
		addr_t physical_addr;
		if (translate(address, &physical_addr, proc)) {
			pthread_mutex_lock(&ram_lock);
			_ram[physical_addr] = data;
			pthread_mutex_unlock(&ram_lock);
			return 0;
		}else{
			return 1;
		}
	}

	void dump(void) {
		int i;
		for (i = 0; i < NUM_PAGES; i++) { 	//seach all pages
			if (_mem_stat[i].proc != 0) {	//find pages is used
				printf("%03d: ", i);		//print i()
				printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
					i << OFFSET_LEN, 	// địa chỉ đầu page
					((i + 1) << OFFSET_LEN) - 1, // địa chỉ cuối page
					_mem_stat[i].proc,	//in id process 
					_mem_stat[i].index,	//
					_mem_stat[i].next
				);
				int j;
				for (	j = i << OFFSET_LEN;
					j < ((i+1) << OFFSET_LEN) - 1;
					j++) {
					
					if (_ram[j] != 0) {
						printf("\t%05x: %02x\n", j, _ram[j]);
					}
						
				}
			}
		}
	}




