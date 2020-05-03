운영체제 프로그래밍 과제  
=================

##< Virtual Memory Management Simulator >
이번 프로그래밍 과제 수업 시간에 배운 Virtual Memory Systems에서 one-level, two-level
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
   
