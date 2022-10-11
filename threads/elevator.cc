#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"


int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");


ELEVATOR *e;


void ELEVATOR::start(int numFloors) {
    
    while(1) {
        // A. Wait until hailed
        while (totalPersonsWaiting(numFloors)== 0 && occupancy == 0){
            elevatorLock->Acquire();
            personArrived->Wait(elevatorLock);
            elevatorLock->Release();
        }

        // B. While there are active persons, loop doing the following
        while (occupancy > 0 || totalPersonsWaiting(numFloors) > 0){
        //      0. Acquire elevatorLock
            elevatorLock->Acquire();
        //      1. Signal persons inside elevator to get off (leaving->broadcast(elevatorLock))
            leaving[currentFloor]->Broadcast(elevatorLock);
            elevatorLock->Release();
            currentThread->Yield();
            elevatorLock->Acquire();
        //      2. Signal persons atFloor to get in, one at a time, checking occupancyLimit each time
            while(occupancy <= maxOccupancy){
                entering[currentFloor]->Signal(elevatorLock);
                elevatorLock->Release();
                currentThread->Yield();
                elevatorLock->Acquire();
            }
        //      2.5 Release elevatorLock

            elevatorLock->Release();
        //      3. Spin for some time
                for(int j =0 ; j< 1000000; j++) {
                    currentThread->Yield();
                }
        //      4. Go to next floor
            
            if(directionUp){
                currentFloor++;
            }
            else{
                currentFloor--;
            }
            printf("Elevator arrives on floor %d", currentFloor);
        
        }
        
    }
}

void ElevatorThread(int numFloors) {

    e = new ELEVATOR(numFloors);
    printf("Elevator with %d floors was created!\n", numFloors);

    e->start(numFloors);

}

ELEVATOR::ELEVATOR(int numFloors) {
    currentFloor = 1;
    occupancy = 0;
    maxOccupancy = 5;
    directionUp = TRUE;

     // Initialize entering
    entering = new Condition*[numFloors];
    for (int i = 0; i < numFloors; i++) {
        entering[i] = new Condition("Entering " + i);
    }
    personsWaiting = new int[numFloors];
    elevatorLock = new Lock("ElevatorLock");

    personArrived = new Condition ("personArrived");

    // Initialize leaving
    leaving = new Condition*[numFloors];
    for (int i = 0; i < numFloors; i++) {
        leaving[i] = new Condition("Leaving " + i);
    }
}

int ELEVATOR::totalPersonsWaiting(int numFloors){
    int totalPersons = 0;
    for (int i = 0; i < numFloors; i++){
        totalPersons += personsWaiting[i];

    }
    return totalPersons;

}


void Elevator(int numFloors) {
    // Create Elevator Thread
    Thread *t = new Thread("Elevator");
    t->Fork(ElevatorThread, numFloors);
}


void ELEVATOR::hailElevator(Person *p) {
    // 1. Increment waiting persons atFloor
    
    personsWaiting[p->atFloor]++;
    if(p->atFloor < p->toFloor){
        directionUp = FALSE;
    }
    // 1.5 Acquire elevatorLock;
    elevatorLock->Acquire();
    // 2. Hail Elevator
    personArrived->Signal(elevatorLock);
    
    // 3. Wait for elevator to arrive atFloor [entering[p->atFloor]->wait(elevatorLock)]
    entering[p->atFloor]->Wait(elevatorLock);
    // 5. Get into elevator
    printf("Person %d got into the elevator.\n", p->id);
    // 6. Decrement persons waiting atFloor [personsWaiting[atFloor]++]
    personsWaiting[p->atFloor]++;
    // 7. Increment persons inside elevator [occupancy++]
    occupancy++;
    // 8. Wait for elevator to reach toFloor [leaving[p->toFloor]->wait(elevatorLock)]
    leaving[p->toFloor]->Wait(elevatorLock);
    // 9. Get out of the elevator
    printf("Person %d got out of the elevator.\n", p->id);
    // 10. Decrement persons inside elevator
    occupancy--;
    // 11. Release elevatorLock;
    elevatorLock->Release();
}

void PersonThread(int person) {

    Person *p = (Person *)person;

    printf("Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);

    e->hailElevator(p);

}

int getNextPersonID() {
    int personID = nextPersonID;
    personIDLock->Acquire();
    nextPersonID = nextPersonID + 1;
    personIDLock->Release();
    return personID;
}


void ArrivingGoingFromTo(int atFloor, int toFloor) {


    // Create Person struct
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;

    // Creates Person Thread
    Thread *t = new Thread("Person " + p->id);
    t->Fork(PersonThread, (int)p);

    

}