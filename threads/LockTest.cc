#include "copyright.h"
#include "system.h"
#include "synch.h"

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