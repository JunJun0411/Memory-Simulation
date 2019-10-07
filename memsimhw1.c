// 
// Virual Memory Simulator Homework
// One-level page table system with FIFO and LRU
// Two-level page table system with LRU
// Inverted page table with a hashing system 
// Submission Year: 2019
// Student Name: 박준형
// Student Number: B511074
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PAGESIZEBITS 12			// page size = 4Kbytes
#define VIRTUALADDRBITS 32		// virtual address space size = 4Gbytes
const unsigned PageSize = 1 << PAGESIZEBITS;


struct pageTableEntry {
	int level;				// page table level (1 or 2)
	char valid;
	struct pageTableEntry *secondLevelPageTable;	// valid if this entry is for the first level page table (level = 1)
	int frameNumber;				// valid if this entry is for the second level page table (level = 2)
};

struct framePage {
	int number;			// frame number
	int pid;			// Process id that owns the frame
	int virtualPageNumber;		// virtual page number using the frame
	struct framePage *lruLeft;	// for LRU circular doubly linked list
	struct framePage *lruRight; 	// for LRU circular doubly linked list
};

struct invertedPageTableEntry {
	int pid;				// process id
	int virtualPageNumber;			// virtual page number
	int frameNumber;			// frame number allocated
	struct invertedPageTableEntry *next;
};

struct procEntry {
	char *traceName;			// the memory trace name
	int pid;				// process (trace) id
	int ntraces;				// the number of memory traces
	int num2ndLevelPageTable;		// The 2nd level page created(allocated);
	int numIHTConflictAccess; 		// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;			// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;			// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;			// The number of page faults
	int numPageHit;				// The number of page hits
	struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
};

struct framePage *oldestFrame, *newestFrame; // the oldest frame pointer
int firstLevelBits, phyMemSizeBits, numProcess;
int s_flag = 0;
int nFrame;

void initPhyMem(struct framePage *phyMem, int nFrame) {
	int i;
	for(i = 0; i < nFrame; i++) {
		phyMem[i].number = i;
		phyMem[i].pid = -1;
		phyMem[i].virtualPageNumber = -1;
		phyMem[i].lruLeft = &phyMem[(i-1+nFrame) % nFrame];
		phyMem[i].lruRight = &phyMem[(i+1+nFrame) % nFrame];
	}

	oldestFrame = &phyMem[0];

}
int delete(struct framePage *phyMem, int nFrame, int num){
	int i;
	struct framePage *page;
	page = oldestFrame;
	for(i=0; i<nFrame;i++){
		if(phyMem->number != num) phyMem = phyMem->lruRight
		// Found
		else 
	}
	// Not Found
	return -1;
}

void oneLevelVMSim(struct procEntry *procTable, struct framePage *phyMemFrames, char FIFOorLRU) {
	int i;
	unsigned Vaddr, Paddr, idx, j;
	char rw;
	char act = 0;
	// -s option print statement
		
	for (i = 0, j = 0; EOF!=fscanf(procTable[i].tracefp, "%x %c", &Vaddr, &rw); i = (i + 1) % numProcess) {			
		// write to pageTable
		Paddr = (Vaddr % PageSize);
		idx = (Vaddr / PageSize);
		
		// pageTable Search
		// PageFault(miss)
		if (procTable[i].firstLevelPageTable[idx].valid == 0) {
			// pageWrite
			procTable[i].firstLevelPageTable[idx].valid = 1;
			procTable[i].firstLevelPageTable[idx].frameNumber = j;
			procTable[i].numPageFault++;
			// 변경되는 PageTableEntry의 vaildbit = 0 으로 셋팅
			procTable[i].firstLevelPageTable[phyMemFrames[j].virtualPageNumber].valid = 0;
			// write to phyMemFrameTable
			
			phyMemFrames[j].pid = i;
			phyMemFrames[j].virtualPageNumber = idx;
			
			Paddr += j * PageSize;
			
			if(!act) {
				j++;
				// FIFOorLRU action
				if(j==nFrame) act = 1;
			}
			else{
				// FIFO 
				if(!FIFOorLRU)	j %= nFrame;
				// LRU old오른쪽이 old
				else {
					newestFrame = &oldestFrame
					oldestFrame = &phyMemFrames[oldestFrame.lruRight.number];
					j = oldestFrame.number; 
				}
			}	
		}
		// Hit
		else if(procTable[i].firstLevelPageTable[idx].valid == 1) {
			Paddr += procTable[i].firstLevelPageTable[idx].frameNumber * PageSize;
			procTable[i].numPageHit++;
			// phyMemList update
			int Num = procTable[i].firstLevelPageTable[idx].frameNumber;
			delete()	
		}
		procTable[i].ntraces++;
		
		// -s
		if(s_flag) printf("One-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces, Vaddr, Paddr);
	}

	// 출력	
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

int main(int argc, char *argv[]) {
	int i;

	// -s option
	if (argv[1][0] == '-') {
		s_flag++;
	}

	if (argc < s_flag + 5) {
		printf("Usage : %s [-s] firstLevelBits PhysicalMemorySizeBits TraceFileNames\n", argv[0]); exit(1);
	}

	// 4번째 인자
	phyMemSizeBits = atoi(argv[s_flag + 3]);
	if (phyMemSizeBits < PAGESIZEBITS) {
		printf("PhysicalMemorySizeBits %d should be larger than PageSizeBits %d\n", phyMemSizeBits, PAGESIZEBITS); exit(1);
	}

	// 3번째 인자
	firstLevelBits = atoi(argv[s_flag + 2]);
	if (VIRTUALADDRBITS - PAGESIZEBITS - firstLevelBits <= 0) {
		printf("firstLevelBits %d is too Big for the 2nd level page system\n", firstLevelBits); exit(1);
	}

	// 프로세스의 갯수
	numProcess = argc - s_flag - 4;
	struct procEntry *procTable;
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess );
	struct pageTableEntry *page;
	page = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << (VIRTUALADDRBITS - PAGESIZEBITS)));
	// initialize procTable for memory simulations
	for (i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n", i, argv[i + s_flag + 4]);
		procTable[i].tracefp = fopen(argv[i + s_flag + 4], "rt");
		procTable[i].pid = i;
		procTable[i].ntraces = 0;
		procTable[i].firstLevelPageTable = page;
		
	}

	// Frame 갯수
	nFrame = (1 << (phyMemSizeBits - PAGESIZEBITS)); assert(nFrame > 0);

	printf("\nNum of Frames %d Physical Memory Size %ld bytes\n", nFrame, (1L << phyMemSizeBits));

	struct framePage *phyMemFrames;
	phyMemFrames = (struct framePage *)malloc(sizeof(struct framePage) * nFrame);
	// initialize procTable for the simulation
	for (i = 0; i < numProcess; i++) {
		// initialize procTable fields
		// rewind tracefiles
		rewind(procTable[i].tracefp);
	}
		
		// oneLevel일 경우
	if(argv[s_flag + 1][0] == '0'){
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		// call oneLevelVMSim() with FIFO
		oneLevelVMSim(procTable, phyMemFrames, 0);

		// initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// call oneLevelVMSim() with LRU
		//oneLevelVMSim(procTable, phyMemFrames, 1);
	}	
		
	return(0);
}
