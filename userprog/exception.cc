// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------


void doExit(int status) {
    
    printf("System Call: [%d] invoked Exit\n", currentThread->space->pcb->pid);
    
    // Manage PCB memory As a parent process
    PCB* pcb = currentThread->space->pcb;

    // Delete exited children and set parent null for non-exited ones
    pcb->DeleteExitedChildrenSetParentNull();

    // Manage PCB memory As a child process
    printf ("Process [%d] exits with [%d]\n", currentThread->space->pcb->pid, status);
    if(pcb->parent == NULL) pcbManager->DeallocatePCB(pcb);


    // Delete address space only after use is completed
    delete currentThread->space;

    // Finish current thread only after all the cleanup is done
    // because currentThread marks itself to be destroyed (by a different thread)
    // and then puts itself to sleep -- thus anything after this statement will not be executed!
    currentThread->Finish();

}

void incrementPC() {
    int counter;
    counter = machine->ReadRegister(PCReg);
    counter += 4;
    machine->WriteRegister(PrevPCReg, counter-4);
    machine->WriteRegister(PCReg, counter);
    machine->WriteRegister(NextPCReg, counter+4);
}


void childFunction(int pid) {

    // 1. Restore the state of registers
    currentThread->RestoreUserState();

    // 2. Restore the page table for child
    currentThread->space->RestoreState();

    //PCReg == machine->ReadRegister(PCReg);
    //machine->WriteRegister(pid,  PCReg, currentThread->space->GetNumPages());
    machine->Run();

}

int doFork(int functionAddr) {

    unsigned int oldPC, oldPrevPC, oldNextPC;
    unsigned int index;

    printf("System Call: [%d] invoked Fork.\n", currentThread->space->pcb->pid);

    // 1. Check if sufficient memory exists to create new process
    // if check fails, return -1
    
    if (!(currentThread->space->GetNumPages() <= mm->GetFreePageCount())){
        machine->WriteRegister(2, -1);
        return -1;
    }

    // 2. SaveUserState for the parent thread
    currentThread->SaveUserState();

    // 3. Create a new address space for child by copying parent address space
    //parent = currentThread->space;
    
    AddrSpace *childAddrSpace = new AddrSpace(currentThread->space);

    // 4. Create a new thread for the child and set its addrSpace
    Thread *childThread = new Thread("childThread");
    childThread->space = childAddrSpace;

    // 5. Create a PCB for the child and connect it all up
    PCB *childpcb = pcbManager->AllocatePCB();
    childpcb->thread = childThread;
    childAddrSpace->pcb = childpcb;
    
    // set parent for child pcb
    childpcb->parent = currentThread->space->pcb;
    // add child for parent pcb
    currentThread->space->pcb->AddChild(childpcb);
    
    mmLock->Acquire();
    // 6. Set up machine registers for child and save it to child thread
    oldPC = machine->ReadRegister(PCReg);
    oldPrevPC = machine->ReadRegister(PrevPCReg);
    oldNextPC = machine->ReadRegister(NextPCReg);
    index = oldPC;
    machine->WriteRegister(PCReg, machine->ReadRegister(4)); 
    machine->WriteRegister(PrevPCReg, machine->ReadRegister(4)-4); 
    machine->WriteRegister(NextPCReg, machine->ReadRegister(4)+4); 

    childThread->SaveUserState(); 
    
    // 7. Restore register state of parent user-level process

    // 8. Call thread->fork on Child
    childThread->Fork(childFunction, 1);
    printf("Process [%d] Fork: start at address [0x%x] with [%d] pages memory\n", currentThread->space->pcb->pid, machine->ReadRegister(4), currentThread->space->GetNumPages());
   
    machine->WriteRegister(PCReg, oldPC); 
    machine->WriteRegister(PrevPCReg, oldPrevPC); 
    machine->WriteRegister(NextPCReg, oldNextPC); 

    currentThread->RestoreUserState();
    machine->WriteRegister(2, childpcb->pid);

    mmLock->Release();
    return childpcb->pid;
}

int doExec(char* filename) {
    printf("System Call: [%d] invoked Exec\n", currentThread->space->pcb->pid);

    // Use progtest.cc:StartProcess() as a guide

    // 1. Open the file and check validity
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        incrementPC();
        machine->WriteRegister(2,-1);
        return -1;
    }
    // 6. Delete current address space
    PCB* pcb = currentThread->space->pcb;
    delete currentThread->space;

    // 2. Create new address space
    space = new AddrSpace(executable);

    // 3. Check if Addrspace creation was successful
    if(space->valid != true) {
        printf("Could not create AddrSpace\n");
        incrementPC();
        return -1;
    }

    // Steps 4 and 5 may not be necessary!!

    // 4. Create a new PCB for the new addrspace
    // ?. Can you reuse existing pcb?
    
    // Initialize parent
    space->pcb = pcb;

    // 5. Set the thread for the new pcb
    pcb->thread = currentThread;

    // 7. SEt the addrspace for currentThread
    currentThread->space = space;

    // 8.     delete executable;			// close file
    delete executable;

    // 9. Initialize registers for new addrspace
    space->InitRegisters();		// set the initial register values

    // 10. Initialize the page table
    space->RestoreState();		// load page table register

    // 11. Run the machine now that all is set up
    printf("Exec Program: [%d] loading [%s]\n", currentThread->space->pcb->pid, filename);
    machine->Run();			// jump to the user progam
    ASSERT(FALSE); // Execution nevere reaches here

    return 0;
}


int doJoin(int pid) {


    PCB* joinPCB = pcbManager->GetPCB(currentThread->space->pcb->pid);
    if (joinPCB == NULL) {
    // 1. Check if this is a valid pid and return -1 if not
        return -1;
    }

    // 2. Check if pid is a child of current process
    PCB* pcb = currentThread->space->pcb;
    if (pcb != joinPCB->parent) {
        return -1;
    }

    // 3. Yield until joinPCB has not exited

    while(!joinPCB->HasExited()) {
        currentThread->Yield();
    }

    // 4. Store status and delete joinPCB
    int status = joinPCB->exitStatus;
    delete joinPCB;

    return status;

}


int doKill (int pid) {

    // 1. Check if the pid is valid and if not, return -1
    PCB* joinPCB = pcbManager->GetPCB(pid);
    if (pcb == NULL) return -1;

    // 2. IF pid is self, then just exit the process
    if (pcb == currentThread->space->pcb) {
           doExit(0);
           return 0;
    }

    // 3. Valid kill, pid exists and not self, do cleanup similar to Exit
    // However, change references from currentThread to the target thread
    // pcb->thread is the target thread

    // 4. Set thread to be destroyed.
    scheduler->RemoveThread(pcb->thread);

    // 5. return 0 for success!
}



void doYield() {
    printf("System Call: [%d] invoked Yield.\n", currentThread->space->pcb->pid);
    currentThread->Yield();
}


// This implementation (discussed in one of the videos) is broken!
// Try and figure out why.
char* readString1(int virtAddr) {

    unsigned int pageNumber = virtAddr / 128;
    unsigned int pageOffset = virtAddr % 128;
    unsigned int frameNumber = machine->pageTable[pageNumber].physicalPage;
    unsigned int physicalAddr = frameNumber*128 + pageOffset;

    char *string = &(machine->mainMemory[physicalAddr]);

    return string;

}


// This implementation is correct!
// perform MMU translation to access physical memory
char* readString(int virtualAddr) {
    int i = 0;
    char* str = new char[256];
    unsigned int physicalAddr = currentThread->space->Translate(virtualAddr);

    // Need to get one byte at a time since the string may straddle multiple pages that are not guaranteed to be contiguous in the physicalAddr space
    bcopy(&(machine->mainMemory[physicalAddr]),&str[i],1);
    while(str[i] != '\0' && i != 256-1)
    {
        virtualAddr++;
        i++;
        physicalAddr = currentThread->space->Translate(virtualAddr);
        bcopy(&(machine->mainMemory[physicalAddr]),&str[i],1);
    }
    if(i == 256-1 && str[i] != '\0')
    {
        str[i] = '\0';
    }

    return str;
}

void doCreate(char* fileName)
{
    printf("Syscall Call: [%d] invoked Create.\n", currentThread->space->pcb->pid);
    fileSystem->Create(fileName, 0);
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    } else  if ((which == SyscallException) && (type == SC_Exit)) {
        // Implement Exit system call
        doExit(machine->ReadRegister(4));
    
    } else if ((which == SyscallException) && (type == SC_Fork)) {
        int ret = doFork(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    } else if ((which == SyscallException) && (type == SC_Exec)) {
        int virtAddr = machine->ReadRegister(4);
        char* fileName = readString(virtAddr);
        int ret = doExec(fileName);
        machine->WriteRegister(2, ret);
        incrementPC();
    } else if ((which == SyscallException) && (type == SC_Join)) {
        int ret = doJoin(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    } else if ((which == SyscallException) && (type == SC_Kill)) {
        int ret = doKill(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    } else if ((which == SyscallException) && (type == SC_Yield)) {
        doYield();
        incrementPC();
    } else if((which == SyscallException) && (type == SC_Create)) {
        int virtAddr = machine->ReadRegister(4);
        char* fileName = readString(virtAddr);
        doCreate(fileName);
        incrementPC();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
