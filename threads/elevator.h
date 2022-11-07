
#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "copyright.h"
#include "synch.h"

void Elevator(int numFloors);
void ArrivingGoingFromTo(int atFloor, int toFloor);

typedef struct Person {
    int id;
    int atFloor;
    int toFloor;
} Person;


class ELEVATOR {

public:
    ELEVATOR(int numFloors);
    ~ELEVATOR();
    void hailElevator(Person *p);
    void start(int numFloors);
    int totalPersonsWaiting(int numFloors);
    

private:
    int currentFloor;
    Condition **entering;
    Condition **leaving;
    Condition *personArrived;
    int *personsWaiting;
    int occupancy;
    int maxOccupancy;
    Lock *elevatorLock;
    bool directionUp;

};

#endif