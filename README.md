운영체제 프로그래밍
=================

## < Virtual Memory Management Simulator >
이번 프로그래밍은 수업 시간에 배운 Virtual Memory Systems에서 one-level, two-level
Page Table 과 Inverted Page Table system을 구현하여 시뮬레이션 해 보는 것입니다. 제공
되는 mtraces 디렉토리에 실제 프로그램 수행 중 접근한 메모리의 주소(Virtual address)를 순
차적으로 모아 놓은 memory trace가 있습니다. 각각의 trace file에 들어 있는 memory trace
포맷은 다음과 같습니다. 
<pre><code>0041f7a0 R
13f5e2c0 R
05e78900 R
004758a0 R
31348900 W
</code></pre>
   
앞의 8문자는 접근된 메모리의 주소를 16진수로 나타낸 것이고(32bits) 그 뒤의 R 또는 W 해당
메모리 주소에 Read를 하는지 Write를 하는지 각각을 나타냅니다. 이 trace는 다음 코드
(fscanf())를 사용하여 읽어 들이면 됩니다. (본 문제에서 R/W 는 중요하지 않습니다.)
<pre><code>unsigned addr;
 char rw;
 ...
 fscanf(file,"%x %c",&addr,&rw);
</code></pre>
Virtual Memory System simulator에서 virtual address 크기는 32bits (4Gbytes)로 나타나고
page의 사이즈는 12bits (4Kbytes)로 가정 합니다.

## 수행 예
<pre><code>$ ./memsim –s 1 10 32 ../mtraces/gcc.trace ../mtraces/bzip.trace
process 0 opening ../mtraces/gcc.trace
process 1 opening ../mtraces/bzip.trace
Num of Frames 1048576 Physical Memory Size 4294967296 bytes ============================================================= The Two-Level Page Table Memory Simulation Starts ..... ============================================================= Two-Level procID 0 traceNumber 1 virtual addr 2f8773d8 pysical addr 3d8
Two-Level procID 1 traceNumber 1 virtual addr 6645b58 pysical addr 1b58
Two-Level procID 0 traceNumber 2 virtual addr 3d729358 pysical addr 2358
Two-Level procID 1 traceNumber 2 virtual addr 6645b58 pysical addr 1b58
......
...... Two-Level procID 0 traceNumber 999999 virtual addr 2f8773e0 pysical addr 3ae3e0
Two-Level procID 1 traceNumber 999999 virtual addr 6645ba0 pysical addr 723ba0
Two-Level procID 0 traceNumber 1000000 virtual addr 3d729358 pysical addr 24358
Two-Level procID 1 traceNumber 1000000 virtual addr 5fe5180 pysical addr 2eb180
**** ../mtraces/gcc.trace ***** Proc 0 Num of traces 1000000
Proc 0 Num of second level page tables allocated 164
Proc 0 Num of Page Faults 2852
Proc 0 Num of Page Hit 997148
**** ../mtraces/bzip.trace ***** Proc 1 Num of traces 1000000
Proc 1 Num of second level page tables allocated 39
Proc 1 Num of Page Faults 317
Proc 1 Num of Page Hit 999683
</code></pre>

## 그 밖에 수행 예:

<pre><code>memsim –s 0 10 20 ../mtraces/gcc.trace ../mtraces/bzip.trace ../mtraces/random0.trace
memsim –s 1 10 32 ../mtraces/bzip.trace
memsim –s 2 10 32 ../mtraces/gcc.trace ../mtraces/bzip.trace
memsim –s 3 10 24 ../mtraces/gcc.trace ../mtraces/bzip.trace ../mtraces/bzip.trace
memsim 3 10 18 ../mtraces/bzip.trace ../mtraces/gcc.trace ../mtraces/sixpack.trace
../mtraces/swim.trace ../mtraces/random0.trace ../mtraces/random2.trace
memsim 3 10 22 ../mtraces/bzip.trace ../mtraces/gcc.trace ../mtraces/sixpack.trace
../mtraces/swim.trace ../mtraces/random0.trace ../mtraces/random2.trace
memsim 3 7 19 ../mtraces/bzip.trace ../mtraces/gcc.trace ../mtraces/sixpack.trace ../mtraces/swim.trace 
</code></pre>
