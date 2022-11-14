- [Compile command](#compile-command)
- [Global Mutex Used](#global-mutex-used)
  - [couldntWashMutex](#couldntwashmutex)
  - [printLock](#printlock)
  - [timeMutex](#timemutex)
- [Global Condition Variables](#global-condition-variables)
  - [timeCond](#timecond)
- [Extra Functions](#extra-functions)
  - [studentIn](#studentin)
  - [cmpfunc](#cmpfunc)
  - [timerThread](#timerthread)
- [Algorithm](#algorithm)
- [Extra Info](#extra-info)

# Compile command

`gcc q1.c ../colors.c -lpthread`

# Global Mutex Used

## couldntWashMutex

Used for maintaing atomicity across modification of the total number of students who could not complete their washing

## printLock

Ensures that only printf is running at a time to prevent mixing of outputs

## timeMutex

Ensures that the curTime variable that stores time for all threads is not being modified simultaneously.

# Global Condition Variables

## timeCond

Broadcasted on every tick

# Extra Functions

## studentIn

This function is what each student's thread runs. It is responsible for waiting, allocating, and washing the clothes.
On failure it updates the global variables `couldntWash` and `totalSecondsWasted`.

## cmpfunc

Used to compare two student objects. It is used in qsort in `main`. It places those students on the left who must come first by FCFS.

## timerThread

This function is what the timer thread runs. It is responsible for updating the global variable `curTime` and broadcasting on `timeCond` every second. It ensures that there is only one global view of time.

# Algorithm

The algorithm is pretty straight forward.
All student threads are created at once since thread creation can take undefined time.
All student threads then wait for wakeUp semaphore which is posted on when their time to arrive has come.
They then do a timed wait on the washing machines, leaving if timed out, or washing if given a washing machine before the time runs out.
Washing is simulated using a loop and to eliminate busy waiting, by making the variable sleep in between ticks.
The student leaves and leaves the washing machines for others by posting on the washing machine semaphore.
These threads update `couldntWash` and `totalSecondsWasted` if they could not be allocated a washing machine.

A separate timer thread exists that increments curTime every second and broadcasts on `timeCond`

Main function starts off with init-ing all the mutex and condition variables and takes in the required input.
The students are then sorted on based of their arrival time (if equal on their indices)
Then it creates all their threads passing them their information.
Then it keeps iterating over the list and waking up any student thread that needs to run.

# Extra Info

Bonus was completed, time is printed.
Same coloring scheme as required.
