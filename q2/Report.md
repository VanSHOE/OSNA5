# Compile command

`gcc q1.c ../colors.c -lpthread`

# Global Mutex Used

## ingrLock

Used for maintaing atomicity across modification of the total number of ingredients in the kitchen.

## printLock

Ensures that only printf is running at a time to prevent mixing of outputs

## timeMutex

Ensures that the curTime variable that stores time for all threads is not being modified simultaneously.

# Global Condition Variables

## timeCond

Broadcasted on every tick

# Global Semaphores

## driveQueue

Used to ensure that only x cars can be in the drive queue at a time.

## ovensQueue

Used to ensure that only x ovens can be in use at a time.

# Functions and Algorithm

## chefFunc

This function is what each chef's thread runs. It is responsible for waiting, allocating ovens, and cooking the food.
Each thread is created initially and waits on the timeCond until its arrival time.
It then waits on its wakeUp semaphore which will be run by an order when something is assigned to the chef.
It then checks for ingredients, and if it has all the ingredients, it waits on the ovensQueue semaphore to allocate an oven.
When done, it releases the oven and signals the order that it is done and goes back to waiting on the wakeUp semaphore.

In any of this, if its exit time comes before it is done, it exits.

## orderFunc

This function is what each order's thread runs. It is responsible for waiting, allocating chefs, and assigning orders to chefs.
Each thread is created by the customer when needed. It instantly starts going through pizzas and assigning them to chefs.
Before this it checks if enough ingredients exist for any of its pizzas, if not, it instantly marks itself as rejected and notifies its customer. It only assigns an order to the chef if it is possible for that chef to start that order at that instant and finish it as well.
It signals the customer accordingly on whether its order was done, partially done or rejected.

## customerFunc

This function is what each customer's thread runs. It is responsible for waiting, allocating orders, and waiting for orders to be done.
Each thread is created initially and waits on wakeUp. It is woken up on the tick when its arrival time comes.
It waits on the driveQueue semaphore to allocate a spot in the drive queue. It waits to be rejected or told to move to the pickupSpot.

## timerThread

This function is what the timer thread runs. It is responsible for updating the global variable `curTime` and broadcasting on `timeCond` every second. It ensures that there is only one global view of time.

## customerCompare

This function is used to compare two customers based on their arrival time. If same, then based on their indices.

## chefCompare

This function is used to compare two chefs based on their arrival time. If same, then based on their indices.

## main

This function initializes all the global variables and semaphores. It then creates the timer thread and the chef threads. It then reads the input and creates the customer threads. It then waits for all the threads to finish and then exits.

# Questions

## The pick-up spot now has a stipulated amount of pizzas it can hold. If the pizzas are full, chefs route the pizzas to a secondary storage. How would you handle such a situation?

I would use a queue for the secondary storage. The chefs would push the pizzas to the queue and as the pick-up slots get free, new pizzas would be popped from the queue and put in the pick-up slots. It is just an additional queue. Another thread that handles this movement can be created which checks when there is a free slot in the pickup spot and it moves the front of the queue to the pickup spot.

## Each incomplete order affects the ratings of the restaurant. Given the past histories of orders, how would you re-design your simulation to have lesser incomplete orders? Note that the rating of the restaurant is not affected if the order is rejected instantaneously on arrival.

Since currently I am allowing orders even if they can be made partially, I can just remove that. Now, even if one pizza can't be made, the order is rejected. This would reduce the number of incomplete orders.

## Ingredients can be replenished on calling the nearest supermarket. How would your drive-thru rejection / acceptance change based on this?

This depends on the specifics of how the supermarket works. If I can just ask for as many ingredients as I want, then there is no need to ever reject a customer because of ingredient shortage. If a chef is still there after ingredients come, the order can be prepared and hence can be accepted. Otherwise it will be rejected.
And while making some parts of the pizza, a chef can put an order so that the supermarket can deliver the ingredients. This would reduce the number of rejected orders and it would be efficient as minimal to no time was wasted waiting for it.
