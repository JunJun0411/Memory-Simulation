// Operate System 운영 체제
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
const unsigned PageSize = 1 << PAGESIZEBITS;	// page size = 4Kbytes


struct pageTableEntry {
	int level;										// page table level (1 or 2)
	int valid;
	struct pageTableEntry *secondLevelPageTable;	// valid if this entry is for the first level page table (level = 1)
	int frameNumber;								// valid if this entry is for the second level page table (level = 2)
};

struct framePage {
	int number;						// frame number
	int pid;						// Process id that owns the frame
	int virtualPageNumber;			// virtual page number using the frame
	int firstVirtualPageNumber;
	struct framePage *lruLeft;		// for LRU circular doubly linked list
	struct framePage *lruRight; 	// for LRU circular doubly linked list
};

struct invertedPageTableEntry {
	int pid;						// process id
	int virtualPageNumber;			// virtual page number
	int frameNumber;				// frame number allocated(index)
	struct invertedPageTableEntry *next;
};

struct procEntry {
	char *traceName;				// the memory trace name
	int pid;						// process (trace) id
	int ntraces;					// the number of memory traces
	int num2ndLevelPageTable;		// The 2nd level page created(allocated);
	int numIHTConflictAccess; 		// The number of Inverted Hash Table Conflict Accesses
	int numIHTNULLAccess;			// The number of Empty Inverted Hash Table Accesses
	int numIHTNonNULLAcess;			// The number of Non Empty Inverted Hash Table Accesses
	int numPageFault;				// The number of page faults
	int numPageHit;					// The number of page hits
	struct pageTableEntry *firstLevelPageTable;
	FILE *tracefp;
};

struct framePage *oldestFrame;	// the oldest frame pointer
struct framePage *newestFrame;	// the newest frame pointer
int firstLevelBits, twoLevelBits, phyMemSizeBits, numProcess;
int s_flag = 0;
int nFrame;

void initPageTableEntry(struct pageTableEntry * pte, int num){
	int i;
	for(i=0; i<num; i++){
		pte[i].level=0;
		pte[i].valid=0;
		pte[i].frameNumber=-1;
		pte[i].secondLevelPageTable = NULL;
	}
}
 
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
void initprocTable(struct procEntry *procTable, int numProcess){
	int i;
	// initialize procTable for the simulation
	for (i = 0; i < numProcess; i++) {
		// initialize procTable fields
		// rewind tracefiles
		rewind(procTable[i].tracefp);
		procTable[i].pid = i;
		procTable[i].ntraces = 0;
		procTable[i].num2ndLevelPageTable = 0;
		procTable[i].numIHTConflictAccess = 0;
		procTable[i].numIHTNonNULLAcess = 0;
		procTable[i].numIHTNULLAccess = 0;
		procTable[i].numPageFault = 0;
		procTable[i].numPageHit = 0;
	}
}
void initinvertedPageTableEntry(struct invertedPageTableEntry *inverted, int nFrame) {
	int i;
	for (i = 0; i < nFrame; i++) {
		inverted[i].pid = -1;
		inverted[i].virtualPageNumber = -1;
		inverted[i].frameNumber = -1;
		inverted[i].next = NULL;
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
	
	// firstLevelPageTable 메모리 할당
	for(i=0; i<numProcess; i++){
		 procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << (VIRTUALADDRBITS - PAGESIZEBITS)));
		 initPageTableEntry(procTable[i].firstLevelPageTable, (1 << (VIRTUALADDRBITS - PAGESIZEBITS) ));
	}
	for (i = 0; EOF != fscanf(procTable[i].tracefp, "%x %c", &Vaddr, &rw); i = (i + 1) % numProcess) {

		// offset
		Paddr = (Vaddr & 0x00000FFF);
		// VPN
		idx = ((Vaddr & 0xFFFFF000) >> PAGESIZEBITS);

		// Miss
		if (procTable[i].firstLevelPageTable[idx].valid == 0) {
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
			if (FIFOorLRU && fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
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
				if (oldestFrame->number == fnum) {
					oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
					// phyMem 가득찬 경우
					if (fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
					else LRU(phyMemFrames, fnum);

				}
				else LRU(phyMemFrames, fnum);
			}
		}

		procTable[i].ntraces++;

		// -s option print statement
		if (s_flag) printf("One-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces, Vaddr, Paddr);
		}

	// 출력	
	for (i = 0; i < numProcess; i++) {
		printf("**** %s *****\n", procTable[i].traceName);
		printf("Proc %d Num of traces %d\n", i, procTable[i].ntraces);
		printf("Proc %d Num of Page Faults %d\n", i, procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n", i, procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		free(procTable[i].firstLevelPageTable);
	}
}

void twoLevelVMSim(struct procEntry *procTable, struct framePage *phyMemFrames) {
	int i, j, fnum, bpid, bfidx, btidx;
	unsigned Vaddr, Paddr, idxF, idxT;
	char rw;
	char fullFrame = 0;
	// oneLevelPageTable 할당
	for(i=0; i<numProcess; i++){
                 procTable[i].firstLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << firstLevelBits));
		 initPageTableEntry(procTable[i].firstLevelPageTable, (1 << firstLevelBits) );
        }


	for (i = 0; EOF != fscanf(procTable[i].tracefp, "%x %c", &Vaddr, &rw); i = (i + 1) % numProcess) {
		// offset
		Paddr = (Vaddr & 0x00000FFF);
		// twoLevelIndex
		idxT = (((Vaddr & 0xFFFFF000) << firstLevelBits) >> (PAGESIZEBITS + firstLevelBits));
		// oneLevelIndex
		idxF = ((Vaddr & 0xFFFFF000) >> (PAGESIZEBITS + twoLevelBits));

		// Miss 1LevelPage.vaild=0 인경우 2LevelPage.vaild=0인경우
		if (procTable[i].firstLevelPageTable[idxF].valid == 0 || procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].valid == 0) {
			// Page Fault
			procTable[i].numPageFault++;

			// firstLevelPage가 없는 경우 할당
			if (procTable[i].firstLevelPageTable[idxF].valid == 0) {
				procTable[i].firstLevelPageTable[idxF].valid = 1;
				procTable[i].firstLevelPageTable[idxF].level = 1;
				procTable[i].firstLevelPageTable[idxF].secondLevelPageTable = (struct pageTableEntry *)malloc(sizeof(struct pageTableEntry) * (1 << twoLevelBits));
				initPageTableEntry(procTable[i].firstLevelPageTable[idxF].secondLevelPageTable, (1 << twoLevelBits) );
				
				procTable[i].num2ndLevelPageTable++;
			}
			// Write in secondLevelPage
			procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].valid = 1;
			procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].level = 2;
			procTable[i].firstLevelPageTable[idxF].secondLevelPageTable[idxT].frameNumber = newestFrame->number;

			// 대체될 경우 phyMemFrame에서 page-out 되어 pageTable의 vaild bit을 0으로 셋팅
			if (fullFrame) {
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
		if (s_flag) printf("Two-Level procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces, Vaddr, Paddr);
	}

	for (i = 0; i < numProcess; i++) {
		printf("**** %s *****\n", procTable[i].traceName);
		printf("Proc %d Num of traces %d\n", i, procTable[i].ntraces);
		printf("Proc %d Num of second level page tables allocated %d\n", i, procTable[i].num2ndLevelPageTable);
		printf("Proc %d Num of Page Faults %d\n", i, procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n", i, procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		for (j = 0; j < (1 << firstLevelBits); j++){ 
			if(procTable[i].firstLevelPageTable[j].valid){
				free(procTable[i].firstLevelPageTable[j].secondLevelPageTable);
			}
		}
		free(procTable[i].firstLevelPageTable);
	}
}
struct invertedPageTableEntry* CreateNode(int id, int vpn, int newNum ,struct invertedPageTableEntry *hashIPT, char check) { 
	struct invertedPageTableEntry * NewNode = (struct invertedPageTableEntry *)malloc(sizeof(struct invertedPageTableEntry));
	NewNode->pid = id; 
	NewNode->virtualPageNumber = vpn;
	NewNode->frameNumber = newNum;
	if(check) {
		NewNode->next = hashIPT->next;
	}
	else NewNode->next = NULL; 
	hashIPT->next = &*NewNode;

	return NewNode; 
}
void invertedPageVMSim(struct procEntry *procTable, struct framePage *phyMemFrames, int nFrame) {
	int i, fnum, idx;
	unsigned Vaddr, Paddr, hashIdx;
	char rw;
	char fullFrame = 0;


	// invertedPageTable 생성
	struct invertedPageTableEntry *iPT;
	iPT = (struct invertedPageTableEntry *)malloc(sizeof(struct invertedPageTableEntry) * nFrame);
	// invertedPageTable 초기화
	initinvertedPageTableEntry(iPT, nFrame);

	
	for (i = 0; EOF != fscanf(procTable[i].tracefp, "%x %c", &Vaddr, &rw); i = (i + 1) % numProcess) {
		// offset
		Paddr = (Vaddr & 0x00000FFF);
		// VPN
		idx = ((Vaddr & 0xFFFFF000) >> PAGESIZEBITS);
		// 해싱 후 Index
		hashIdx = (i + idx) % nFrame;

		// Empty entry일 경우
		if (iPT[hashIdx].next == NULL) {
			procTable[i].numIHTNULLAccess++;
			procTable[i].numPageFault++;

			CreateNode(i, idx, newestFrame->number, &iPT[hashIdx], 0);

			// page-out : 대체될 경우 phyMemFrame에 hashTable에서 할당하고 있는 entry 삭제
			if (fullFrame) {
				int bpid, bidx, hidx;
				bpid = phyMemFrames[newestFrame->number].pid;
				bidx = phyMemFrames[newestFrame->number].virtualPageNumber;
				hidx = (bpid + bidx) % nFrame;
				struct invertedPageTableEntry *nIPT;	// 삭제할 entry
				struct invertedPageTableEntry *bIPT;	// nIPT를 링크하고 있는 entry
				nIPT = iPT[hidx].next;
				bIPT = &iPT[hidx];
				// Search
				while (nIPT->pid != bpid || nIPT->virtualPageNumber != bidx) {
					if(nIPT->next == NULL) break;
					bIPT = nIPT;
					nIPT = nIPT->next;
                        	}

				// 못 찾은경우 error
				if (nIPT->pid != bpid || nIPT->virtualPageNumber != bidx) {
					printf("HashTable에 삭제할 entry가 존재하지 않습니다empty \n");
					exit(1);
				}
				// 찾은 entry의 next가 있는 경우 
				bIPT->next = nIPT->next;
			}


			// Write in phyMemFrameTable			
			phyMemFrames[newestFrame->number].pid = i;
			phyMemFrames[newestFrame->number].virtualPageNumber = idx;
			phyMemFrames[newestFrame->number].number = newestFrame->number;
			// 모든 Frame들이 사용되고 있는 경우 oldestFrame의 포인터를 오른쪽으로 이동
			if (fullFrame) oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
			if (newestFrame->number == nFrame - 1) fullFrame = 1;

			// physical Frame Number
			Paddr += newestFrame->number * PageSize;

			// 다음 교체해야할 newestFrame을 오른쪽 포인터으로 넘김
			newestFrame = &phyMemFrames[newestFrame->lruRight->number];

			// oldestFrame을 교체해야하는 경우
			if (fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];

		}

		// Collision 
		else {
			procTable[i].numIHTNonNULLAcess++;
			struct invertedPageTableEntry *cIPT;	// 찾을 entry
			cIPT = iPT[hashIdx].next; 

			procTable[i].numIHTConflictAccess++;

			while (1){
				if((cIPT->next == NULL) || ((cIPT->pid == i) && (cIPT->virtualPageNumber == idx))) break;
				cIPT = cIPT->next;
				procTable[i].numIHTConflictAccess++;
			}

			// entry에서 찾은 경우 pageHit
			if ((cIPT->pid == i) && (cIPT->virtualPageNumber == idx)) {
			//	printf("HIT %d\n" , hashIdx);
				procTable[i].numPageHit++;
				fnum = cIPT->frameNumber;
				Paddr += fnum * PageSize;

				// oldestFrame과 Hit Frame이 같은경우
				if (oldestFrame->number == fnum) {
					oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
					// phyMem 가득찬 경우
					if (fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
					else LRU(phyMemFrames, fnum);
				}
				else LRU(phyMemFrames, fnum);
			}

			// 못 찾은 경우 pageFault
			else {
				procTable[i].numPageFault++;

				CreateNode(i, idx, newestFrame->number, &iPT[hashIdx], 1);

				// page-out : 대체될 경우 phyMemFrame에 hashTable에서 할당하고 있는 entry 삭제
				if (fullFrame) {
					int bpid, bidx, hidx;
					bpid = phyMemFrames[newestFrame->number].pid;
					bidx = phyMemFrames[newestFrame->number].virtualPageNumber;
					hidx = (bpid+bidx)%nFrame;
					struct invertedPageTableEntry *n2IPT;	// 삭제할 entry
					struct invertedPageTableEntry *b2IPT;	// n2IPT를 링크하고 있는 entry
					n2IPT = iPT[hidx].next;
					b2IPT = &iPT[hidx];
		
					// Search
					while (n2IPT->pid != bpid || n2IPT->virtualPageNumber != bidx) {
						if(n2IPT->next == NULL) break;
						b2IPT = n2IPT;
						n2IPT = n2IPT->next;
					}
					// 못 찾은경우 error
					if (n2IPT->pid != bpid || n2IPT->virtualPageNumber != bidx) {
						printf("HashTable에 삭제할 entry가 존재하지 않습니다 \n");
					}
					b2IPT->next = n2IPT->next;
					
					
				}

				// Write in phyMemFrameTable			
				phyMemFrames[newestFrame->number].pid = i;
				phyMemFrames[newestFrame->number].virtualPageNumber = idx;
				phyMemFrames[newestFrame->number].number = newestFrame->number;

				// 모든 Frame들이 사용되고 있는 경우 oldestFrame의 포인터를 오른쪽으로 이동
				if (fullFrame) oldestFrame = &phyMemFrames[oldestFrame->lruRight->number];
				if (newestFrame->number == nFrame - 1) fullFrame = 1;

				// physical Frame Number
				Paddr += newestFrame->number * PageSize;

				// 다음 교체해야할 newestFrame을 오른쪽 포인터으로 넘김
				newestFrame = &phyMemFrames[newestFrame->lruRight->number];

				// oldestFrame을 교체해야하는 경우
				if (fullFrame) newestFrame = &phyMemFrames[oldestFrame->number];
				
			}
		}
		procTable[i].ntraces++;

		// -s option print statement
		if (s_flag) printf("IHT procID %d traceNumber %d virtual addr %x physical addr %x\n", i, procTable[i].ntraces, Vaddr, Paddr);
	}
	for (i = 0; i < numProcess; i++) {
		printf("**** %s *****\n", procTable[i].traceName);
		printf("Proc %d Num of traces %d\n", i, procTable[i].ntraces);
		printf("Proc %d Num of Inverted Hash Table Access Conflicts %d\n", i, procTable[i].numIHTConflictAccess);
		printf("Proc %d Num of Empty Inverted Hash Table Access %d\n", i, procTable[i].numIHTNULLAccess);
		printf("Proc %d Num of Non-Empty Inverted Hash Table Access %d\n", i, procTable[i].numIHTNonNULLAcess);
		printf("Proc %d Num of Page Faults %d\n", i, procTable[i].numPageFault);
		printf("Proc %d Num of Page Hit %d\n", i, procTable[i].numPageHit);
		assert(procTable[i].numPageHit + procTable[i].numPageFault == procTable[i].ntraces);
		assert(procTable[i].numIHTNULLAccess + procTable[i].numIHTNonNULLAcess == procTable[i].ntraces);
	}
	free(iPT);
	
}
int main(int argc, char *argv[]) {
	int i;

	// -s option
	if (argv[1][0] == '-' && argv[1][1] == 's') {
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
	procTable = (struct procEntry *)malloc(sizeof(struct procEntry) * numProcess);
	// initialize procTable for memory simulations
	for (i = 0; i < numProcess; i++) {
		// opening a tracefile for the process
		printf("process %d opening %s\n", i, procTable[i].traceName = argv[i + s_flag + 4]);
		procTable[i].tracefp = fopen(argv[i + s_flag + 4], "r");
		procTable[i].traceName = argv[i + s_flag + 4];
	}

	// Frame 갯수
	nFrame = (1 << (phyMemSizeBits - PAGESIZEBITS)); assert(nFrame > 0);

	printf("\nNum of Frames %d Physical Memory Size %ld bytes\n", nFrame, (1L << phyMemSizeBits));

	struct framePage *phyMemFrames;
	phyMemFrames = (struct framePage *)malloc(sizeof(struct framePage) * nFrame);
	
	// oneLevel일 경우
	if (argv[s_flag + 1][0] == '0' || argv[s_flag + 1][0] >= '3') {
		// initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The One-Level Page Table with FIFO Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess);
		// call oneLevelVMSim() with FIFO
		oneLevelVMSim(procTable, phyMemFrames, 0);

		printf("=============================================================\n");
		printf("The One-Level Page Table with LRU Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess);
		// call oneLevelVMSim() with LRU
		oneLevelVMSim(procTable, phyMemFrames, 1);
	}
	// twoLevel일 경우
	if (argv[s_flag + 1][0] == '1' || argv[s_flag + 1][0] >= '3'){
		// initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The Two-Level Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess);
		// call twoLevelVMSim()
		twoLevelVMSim(procTable, phyMemFrames);
	}
	// inverted의 경우
	if (argv[s_flag + 1][0] == '2' || argv[s_flag + 1][0] >= '3') {

		// initialize procTable for the simulation
		printf("=============================================================\n");
		printf("The Inverted Page Table Memory Simulation Starts .....\n");
		printf("=============================================================\n");
		initPhyMem(phyMemFrames, nFrame);
		initprocTable(procTable, numProcess);
		// call invertedPageVMsim()
		invertedPageVMSim(procTable, phyMemFrames, nFrame);
	}

	return(0);
}
