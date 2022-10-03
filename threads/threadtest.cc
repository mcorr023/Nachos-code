// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;
int numThreadsActive;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
#ifdef HW1_SEMAPHORES

int SharedVariable;
Semaphore *sem = new Semaphore("semaphore", 1);
void SimpleThread(int which) {
    int num, val;
    for(num = 0; num < 5; num++) {

        //entry section
        sem->P();

        val = SharedVariable;
        printf("*** thread %d sees value %d\n", which, val);
        currentThread->Yield();
        SharedVariable = val+1;

        //exit section
        sem->V();

        currentThread->Yield();
        }
        
        //Decrement numThreadsActive
        numThreadsActive--;
        //Check if numthreadsActive = 0; yield self while not.
        while(numThreadsActive != 0){
            currentThread->Yield();
        }

    val = SharedVariable;
    printf("Thread %d sees final value %d\n", which, val);
}

#else

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}
#endif

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

#ifdef HW1_SEMAPHORES

void
ThreadTest2(int n) {
    DEBUG('t', "Entering SimpleTest");
    Thread *t;
    numThreadsActive = n;
    printf("NumthreadsActive = %d\n", numThreadsActive);

    for(int i=1; i<n; i++)
    {
        t = new Thread("forked thread");
        t->Fork(SimpleThread,i);
    }
    SimpleThread(0);
}

#else 

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

#endif 

#ifdef HW1_LOCKS
Lock * Testl = new Lock ("MyLock");
int Counter=0;
void LockT(int number)
{
    	int num, val;
 	
	for (num = 0; num < 3; num++){
		Testl ->Acquire();
		
		val = Counter;
		printf("*** thread %d sees counter value %d\n", number, val);
		currentThread->Yield();
		
		Counter++; 
        			
		Testl->Release();
		
		currentThread->Yield();
		
	 }
	
	while (Counter<12)
		currentThread->Yield();
	
	val = Counter;

    printf("Thread %d sees final value %d\n", number, val);
}

void LockTest()
{
	for (int i = 1; i<=3; i++)
	{
    		Thread *Testt = new Thread("forked thread");

    		Testt ->Fork(LockT, i);
	}
    LockT(0);
}

#endif 

