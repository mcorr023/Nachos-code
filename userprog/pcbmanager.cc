

#include "pcbmanager.h"


PCBManager::PCBManager(int maxProcesses) {

    bitmap = new BitMap(maxProcesses);
    pcbs = new PCB*[maxProcesses];
    pcbManagerLock = new Lock("pcbManagerLock");
    //printf("pcbs.size() = %d\n", sizeof(pcbs));

    for(int i = 0; i < maxProcesses; i++) {
        pcbs[i] = NULL;
    }

}


PCBManager::~PCBManager() {

    delete bitmap;

    delete pcbs;

}


PCB* PCBManager::AllocatePCB() {

    // Aquire pcbManagerLock

    pcbManagerLock->Acquire();

    int pid = bitmap->Find();

    // Release pcbManagerLock
    pcbManagerLock->Release();
   

    ASSERT(pid != -1);

    pcbs[pid] = new PCB(pid);

    return pcbs[pid];

}


int PCBManager::DeallocatePCB(PCB* pcb) {

    // Check is pcb is valid -- check pcbs for pcb->pid
    ASSERT(pcb->pid != -1);

    // Aquire pcbManagerLock
    pcbManagerLock->Acquire();

    bitmap->Clear(pcb->pid);

    // Release pcbManagerLock
    pcbManagerLock->Release();

    int pID = pcb->pid;
    delete pcbs[pcb->pid];

    pcbs[pID] = NULL;

}

PCB* PCBManager::GetPCB(int pid) {
    return pcbs[pid];
}