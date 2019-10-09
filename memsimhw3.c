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
	int firstVirtualPageNumber;
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

struct framePage *oldestFrame; // the oldest frame pointer
struct framePage *newestFrame; // the newest frame pointer
int firstLevelBits, twoLevelBits, phyMemSizeBits, numProcess;
int s_flag = 0;
int nFrame;

void initPhyMem(struct framePage *phyMem, int nFrame) {
	int i;
	for(i = 0; i < nFrame; i++) {
		phyMem[i].number = i;
		phyMem[i].pid = -1;
		phyMem[i].virtualPageNumber = -1;
		phyMem[i].firstVirtualPageNumber = -1;
		phyMem[i].lruLeft = &phyMem[(i-1+nFrame) % nFrame];
		phyMem[i].lruRight = &phyMem[(i+1+nFrame) % nFrame];
	}

	oldestFrame = &phyMem[0];
	newestFrame = &phyMem[0];

}
void initprocTable(struct procEntry *procTable, int numProcess, int bits){
	int i;
	// initialize procTable for the simulation
	for (i = 0; i < numProcess; i++) {
		// initialize procTable fields
		// rewind tracefiles
		rewind(procTable[i].tracefp);
		procTable[i].ntraces = 0;
		procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << bits));
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;
		procTable[i].num2ndLevelPageTable = 0;
	}
}

void LRU(struct framePage *phyMemFrames, int fnum){
	int lnum, rnum, newF;
	lnum = phyMemFrames[fnum].lruLeft->number;
	rnum = phyMemFrames[fnum].lruRight->number;
	newF = newestFrame->number;

	// Hit Frame 링크에서 삭제
	phyMemFrames[lnum].lruRight = &phyMemFrames[rnum];
	phyMemFrames[rnum].lruLeft = &phyMemFrames[lnum];
	// Hit Frame 선두로
	phyMemFrames[fnum].lruRight = &phyMemFrames[newF];
	phyMemFrames[fnum].lruLeft = &phyMemFrames[newestFrame->lruLeft->number];
	phyMemFrames[newF].lruLeft = &phyMemFrames[fnum];
	phyMemFrames[phyMemFrames[fnum].lruLeft->number].lruRight = &phyMemFrames[fnum];
}

void oneLevelVMSim(struct procEntry *procTable, struct framePage *phyMemFrames, char FIFOorLRU) {
	int i, fnum, bpid, bidx;
	unsigned Vaddr, Paddr, idx;
	char rw;
	char fullFrame = 0;
	for (i = 0 ; EOF!=fscanf(procTable[i].tracefp, "%x %c", &Vaddr, &rw); i = (i + 1) % numProcess) {			
		
		Paddr = (Vaddr % PageSize);
		idx = (Vaddr / PageSize);
		
		// Miss
		if(procTable[i].firstLevelPageTable[idx].valid == 0) {
			// Write in pageTable
			procTable[i].firstLevelPageTable[idx].valid = 1;
			procTable[i].firstLevelPageTable[idx].frameNumber = newestFrame->number;
			// PageFault
			procTable[i].numPageFault++;
			
			// 모든 Frame 사용되는 중이며 LRU의 경우 원래 차지중이던 프로세스의 PageEntry의 vaild bit을 0으로 변경
			if (fullFrame) {
				bpid = phyMemFrames[newestFrame->number].pid;
				bidx = phyMemFrames[newestFrame->number].virtualPageNumber;
				procTable[bpid].firstLevelPageTable[bidx].valid = 0;
			}

			// Write in phyMemFrameTable
			phyMemFrames[newestFrame->number].pid = i;
			phyMemFrames[newestFrame->number].virtualPageNumber = idx;

			// 모든 Frame 사용되는 중이며 LRU의 경우
			if (FIFOorLRU && fullFrame) oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
			if (newestFrame->number == nFrame - 1) fullFrame = 1;

			// physical Frame Number
			Paddr += newestFrame->number * PageSize;
			
			
			newestFrame = &phyMemFrames[newestFrame->lruRight->number];
			// 모든 Frame 사용되는 중이며 LRU의경우에서 oldestFrame 교체해야하는 경우
			if(FIFOorLRU && fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
		}

		// Hit
		else {
			// Hit Frame의 number
			fnum = procTable[i].firstLevelPageTable[idx].frameNumber;
			
			Paddr += fnum * PageSize;
			procTable[i].numPageHit++;
			
			// LRU
			if (FIFOorLRU) {
				// oldestFrame과 Hit Frame이 같은경우
				if(oldestFrame->number == fnum) {
					oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
					// phyMem 가득찬 경우
					if(fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
					else LRU(phyMemFrames, fnum);
					
				}
				else LRU(phyMemFrames, fnum);
			}
		}
		
		procTable[i].ntraces++;
		
		// -s option print statement
		if(s_flag) printf("One-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces, Vaddr, Paddr);
	}

	// 출력	
	for(i=0; i < numProcess; i++) {
		printf("**** %s *****\n",procTable[i].traceName);
		printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		free(procTable[i].firstLevelPageTable);
	}
}

void twoLevelVMSim(struct procEntry *procTable, struct framePage *phyMemFrames) {
   	int i, j, fnum, bpid, bfidx, btidx;
	unsigned Vaddr, Paddr, idxF, idxT;
	char rw;
    	char fullFrame = 0;

	for (i = 0 ; EOF!=fscanf(procTable[i].tracefp, "%x %c", &Vaddr, &rw); i = (i + 1) % numProcess) {
		// offset
		Paddr = (Vaddr % PageSize);
		// twoLevelIndex
		idxT = ((Vaddr % (1 << (PAGESIZEBITS + twoLevelBits))) / PageSize);
		// oneLevelIndex
		idxF = (Vaddr / (1 << (PAGESIZEBITS + twoLevelBits)));
		
		// Miss 1LevelPage.vaild=0 인경우 2LevelPage.vaild=0인경우
		if (procTable[i].firstLevelPageTable[idxF].valid == 0 || procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].valid==0) { 
			// Page Fault
			procTable[i].numPageFault++;

			// firstLevelPage가 없는 경우 할당
			if (procTable[i].firstLevelPageTable[idxF].valid == 0) {
				procTable[i].firstLevelPageTable[idxF].valid = 1;
				procTable[i].firstLevelPageTable[idxF].level = 1;
				procTable[i].firstLevelPageTable[idxF].secondLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << twoLevelBits));
				procTable[i].num2ndLevelPageTable++;
			}
			// Write in secondLevelPage
			procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].valid = 1;
			procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].level = 2;
			procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].frameNumber = newestFrame->number;

			// 대체될 경우 phyMemFrame에서 page-out 되어 pageTable의 vaild bit을 0으로 셋팅
			if(fullFrame) {
				bpid = phyMemFrames[newestFrame->number].pid;
				btidx = phyMemFrames[newestFrame->number].virtualPageNumber;
				bfidx = phyMemFrames[newestFrame->number].firstVirtualPageNumber;
				procTable[bpid].firstLevelPageTable[bfidx].secondLevelPageTable[btidx].valid = 0;
			}

			// Write in phyMemFrameTable
			phyMemFrames[newestFrame->number].pid = i;
			phyMemFrames[newestFrame->number].virtualPageNumber = idxT;
			phyMemFrames[newestFrame->number].firstVirtualPageNumber = idxF;

			// 모든 Mem사용된경우 oldestFrame의 포인터를 오른쪽으로 이동
			if (fullFrame) oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
			if (newestFrame->number == nFrame - 1) fullFrame = 1;

			// physical Frame Number
			Paddr += newestFrame->number * PageSize;

			// 다음 교체해야할 newestFrame을 오른쪽 포인터으로 넘김
			newestFrame = &phyMemFrames[newestFrame->lruRight->number];

			// oldestFrame을 교체해야하는 경우
			if (fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
		}
		
		// Hit
		else {
			// Hit Frame의 number
			fnum = procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].frameNumber;
			Paddr += fnum * PageSize;
			procTable[i].numPageHit++;

			// oldestFrame과 Hit Frame이 같은경우
			if (oldestFrame->number == fnum) {
				oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
				// phyMem 가득찬 경우
				if (fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
				else LRU(phyMemFrames, fnum);

			}
			else LRU(phyMemFrames, fnum);
		}

		procTable[i].ntraces++;

		// -s option print statement
		if(s_flag) printf("Two-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces, Vaddr, Paddr); 
	}
        for(i=0; i < numProcess; i++) {
                printf("**** %s *****\n",procTable[i].traceName);
                printf("Proc %d Num of traces %d\n",i,procTable[i].ntraces);
                printf("Proc %d Num of second level page tables allocated %d\n",i,procTable[i].num2ndLevelPageTable);
                printf("Proc %d Num of Page Faults %d\n",i,procTable[i].numPageFault);
                printf("Proc %d Num of Page Hit %d\n",i,procTable[i].numPageHit);
                assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		
		free(procTable[i].firstLevelPageTable); 
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

	twoLevelBits = VIRTUALADDRBITS - PAGESIZEBITS - firstLevelBits;

	// 프로세스의 갯수
	numProcess = argc - s_flag - 4;
	struct procEntry *procTable;
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess );
	// initialize procTable for memory simulations
	for (i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n", i, procTable[i].traceName = argv[i + s_flag + 4]);
		procTable[i].tracefp = fopen(argv[i + s_flag + 4], "r");
		procTable[i].traceName = argv[i + s_flag + 4];
		procTable[i].pid = i;
		
	}

	// Frame 갯수
	nFrame = (1 << (phyMemSizeBits - PAGESIZEBITS)); assert(nFrame > 0);

	printf("\nNum of Frames %d Physical Memory Size %ld bytes\n", nFrame, (1L << phyMemSizeBits));

	struct framePage *phyMemFrames;
	phyMemFrames = (struct framePage *)malloc(sizeof(struct framePage) * nFrame);
		
		// oneLevel일 경우
	if(argv[s_flag + 1][0] == '0'){
                // initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess, VIRTUALADDRBITS - PAGESIZEBITS);
		// call oneLevelVMSim() with FIFO
		oneLevelVMSim(procTable, phyMemFrames, 0);

		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess, VIRTUALADDRBITS - PAGESIZEBITS);
		// call oneLevelVMSim() with LRU
		oneLevelVMSim(procTable, phyMemFrames, 1);

	}	
	 // twoLevel일 경우
        else if(argv[s_flag + 1][0] == '1'){
                // initialize procTable for the simulation
                printf("=============================================================\n");
                printf("The Two-Level Page Table Memory Simulation Starts .....\n");
                printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess, firstLevelBits);
                // call twoLevelVMSim()
                twoLevelVMSim(procTable, phyMemFrames);
        }

		
	return(0);
}

