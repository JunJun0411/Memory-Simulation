#!/usr/bin/python3
 
import os, sys, subprocess, random, itertools
from subprocess import PIPE, Popen
import time

 
trace_path = "../mtraces/"
trace_list = [trace_path + trace for trace in os.listdir(trace_path)]
 
trace_args = []
for i in range(1, 4):
    trace_args += list(map(list, itertools.product(trace_list, repeat=i)))
 
subprocess.call("gcc -o memsimhw memsimhw.c -O2 -W -Wall", shell=True)
 
i = 1
f = open("errorline.txt", 'a');
while True:
    simType = 3#random.randint(0, 3)
    firstLvBits = random.randint(0, 21)
    phyMem = random.randint(12, 32)
    traces = random.choice(trace_args)

    args = [str(simType), str(firstLvBits), str(phyMem)] + traces
  
    if (phyMem - firstLvBits) < 12 : 
        continue 

    print(i, args, "STARTED")

    try:
        answer = Popen(["./memsim"] + args, stdout=PIPE).communicate()[0].strip()
    except BaseException as e:
        print(e)
        continue
 
    s = time.time()
    mine = Popen(["./memsimhw"] + args, stdout=PIPE).communicate()[0].strip()
    e = time.time()
    answerf = "\n".join([line.strip() for line in answer.split('\n') if "Proc" in line])
    minef = "\n".join([line.strip() for line in mine.split('\n') if "Proc" in line])
	    

    if answerf == minef and (e - s) < 600: 
        print(i,[firstLvBits, phyMem]+traces, "PASSED", e - s)
    else:
        print(answer)
        print(mine)
        print(" ".join(args))
   	f.write(" ".join(args));
	f.write("\n");
    
    i += 1
