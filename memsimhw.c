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

struct procEntry *procTable;
struct framePage *oldestFrame; // the oldest frame pointer
int firstLevelBits, phyMemSizeBits, numProcess;
int s_flag = 0;

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

void oneLevelVMSim(struct procEntry *procTable, struct framePage *phyMemFrames, char FIFOorLRU) {
	
	// -s option print statement
	if(s){
	for()
	printf("One-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces,Vaddr,Paddr );
	}
	
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void twoLevelVMSim(struct procEntry *procTable, struct framePage *phyMemFrames) {
	
	// -s option print statement
	printf("Two-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces,Vaddr,Paddr);
		
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
	}
}

void invertedPageVMSim(struct procEntry *procTable, struct framePage *phyMemFrames, int nFrame) {
	
	// -s option print statement
	printf("IHT procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces,Vaddr,Paddr);
		
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n",i,procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n",i,procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
}

int main(int argc, char *argv[]) {
	int i;

	// -s 옵션이 있는 경우
	if (argv[1] == "-s") {
		s_flag++;
	}

	if (argc < s_flag + 5) {
	     printf("Usage : %s [-s] firstLevelBits PhysicalMemorySizeBits TraceFileNames\n",argv[0]); exit(1);
	}
	
	// 4번째 인자
	phyMemSizeBits = argv[s_flag + 3];
	if (phyMemSizeBits < PAGESIZEBITS) {
		printf("PhysicalMemorySizeBits %d should be larger than PageSizeBits %d\n",phyMemSizeBits,PAGESIZEBITS); exit(1);
	}
	
	// 3번째 인자
	firstLevelBits = argv[s_flag + 2];
	if (VIRTUALADDRBITS - PAGESIZEBITS - firstLevelBits <= 0 ) {
		printf("firstLevelBits %d is too Big for the 2nd level page system\n",firstLevelBits); exit(1);
	}
	
	// 프로세스의 갯수
	numProcess = argc - s_flag - 4;
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);
	// initialize procTable for memory simulations
	for(i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n",i,argv[i+s_flag+3]);
		procTable[i].tracefp = fopen(argv[i+s_flag+3], "rt");
		procTable[i].pid = i;
	}

	int nFrame = (1<<(phyMemSizeBits-PAGESIZEBITS)); assert(nFrame>0);

	printf("\nNum of Frames %d Physical Memory Size %ld bytes\n",nFrame, (1L<<phyMemSizeBits));
	
	// initialize procTable for the simulation
	for(i = 0; i < numProcess; i++) {
		// initialize procTable fields
		// rewind tracefiles
		rewind(procTable[i].tracefp);
		
		// oneLevel일 경우
		if(argv[s_flag + 1] == 0){
			printf("=============================================================\n");
			printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
			printf("=============================================================\n");
			// call oneLevelVMSim() with FIFO
			oneLevelVMSim(procTable[i], phyMemFrames, 'F', i);	
		
			// initialize procTable for the simulation
			printf("=============================================================\n");
			printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
			printf("=============================================================\n");
			// call oneLevelVMSim() with LRU
		}	
	}
	// twoLevel일 경우
	else if(argv[s_flag + 1] == 1){
		// initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// call twoLevelVMSim()
	}

	// inverted의 경우
	else{
		// initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		// call invertedPageVMsim()
	}
	return(0);
}
