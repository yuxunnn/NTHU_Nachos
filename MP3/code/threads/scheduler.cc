// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

// MP3
int 
L1Compare(Thread *t1, Thread *t2) {
    // smaller burst time first
    double t1ApproxRemainTime = t1->getApproxRemainTime();
    double t2ApproxRemainTime = t2->getApproxRemainTime();

    if (t1ApproxRemainTime > t2ApproxRemainTime){
        return 1;
    }
    else if (t1ApproxRemainTime < t2ApproxRemainTime){
        return -1;
    }
    else {
        return 0;
    }
}

// MP3
int 
L2Compare(Thread *t1, Thread *t2) {
    // higher priority first
    int t1Priority = t1->getPriority();
    int t2Priority = t2->getPriority();

    if (t1Priority < t2Priority) {
        return 1;
    }
    else if (t1Priority > t2Priority) {
        return -1;
    }
    else {
        return 0;
    }
}


Scheduler::Scheduler()
{ 
    readyList = new List<Thread *>; 
    // MP3
    L1 = new SortedList<Thread *>(L1Compare);
    L2 = new SortedList<Thread *>(L2Compare);
    L3 = new List<Thread *>;

    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
    // MP3
    delete L1;
    delete L2;
    delete L3;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);

    // readyList->Append(thread);
    // MP3
    int threadPriority = thread->getPriority();

    thread->setStartWaitingTime(kernel->stats->totalTicks);

    if (threadPriority > 149) {
        // out of range
        DEBUG(z, "Thread priority is out of range");
    }
    else if (threadPriority >= 100) {
        // L1 (100 - 149)
        L1->Insert(thread);
        thread->InsertedIntoQueue(1);
    }
    else if (threadPriority >= 50) {
        // L2 (50 - 99)
        L2->Insert(thread);
        thread->InsertedIntoQueue(2);
    }
    else if (threadPriority >= 0) {
        // L3 (0 - 49)
        L3->Append(thread);
        thread->InsertedIntoQueue(3);
    }
    else {
        // out of range
        DEBUG(z, "Thread priority is out of range");
    }
    
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    // if (readyList->IsEmpty()) {
	// 	return NULL;
    // } else {
    // 	return readyList->RemoveFront();
    // }
    // MP3
    if (!L1->IsEmpty()) {
        Thread *currThread = kernel->currentThread;
        Thread *nextThread = L1->RemoveFront();

        nextThread->setCpuStartTime(kernel->stats->totalTicks);
        nextThread->setTotalWaitingTime(0);
        nextThread->setAccuTicks(0);

        nextThread->RemovedFromQueue();
        currThread->ContextSwitch(nextThread->getID());

        return nextThread;
    }
    else if (!L2->IsEmpty()) {
        Thread *currThread = kernel->currentThread;
        Thread *nextThread = L2->RemoveFront();

        nextThread->setCpuStartTime(kernel->stats->totalTicks);
        nextThread->setTotalWaitingTime(0);
        nextThread->setAccuTicks(0);

        nextThread->RemovedFromQueue();
        currThread->ContextSwitch(nextThread->getID());

        return nextThread;
    }
    else if (!L3->IsEmpty()) {
        Thread *currThread = kernel->currentThread;
        Thread *nextThread = L3->RemoveFront();

        nextThread->setCpuStartTime(kernel->stats->totalTicks);
        nextThread->setTotalWaitingTime(0);
        nextThread->setAccuTicks(0);

        nextThread->RemovedFromQueue();
        currThread->ContextSwitch(nextThread->getID());

        return nextThread;
    }
    else {
        return NULL;
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
        ASSERT(toBeDestroyed == NULL);
	    toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

// MP3
int
Scheduler::Aging()
{
    Thread *thread;
    ListIterator<Thread *> *iterator;

    if (!L1->IsEmpty()) {
        iterator = new ListIterator<Thread *>(L1);
        for (; !iterator->IsDone(); iterator->Next()) {
            thread = iterator->Item();

            // Increase total waiting time
            bool doAging = thread->IncreaseTotalWaitingTime();
            thread->setStartWaitingTime(kernel->stats->totalTicks);
            
            // Check whether over 1500
            if (doAging) {
                thread->ChangePriority();
            }

        }
    }

    if (!L2->IsEmpty()) {
        iterator = new ListIterator<Thread *>(L2);
        for (; !iterator->IsDone(); iterator->Next()) {
            thread = iterator->Item();

            // Increase total waiting time
            bool doAging = thread->IncreaseTotalWaitingTime();
            thread->setStartWaitingTime(kernel->stats->totalTicks);
            
            // Check whether over 1500
            if (doAging) {
                thread->ChangePriority();
            }

            if (thread->getPriority() > 99) {
                L2->Remove(thread);
                L1->Insert(thread);

                thread->RemovedFromQueue();
                thread->InsertedIntoQueue(1);
            }
        }
    }

    if (!L3->IsEmpty()) {
        iterator = new ListIterator<Thread *>(L3);
        for (; !iterator->IsDone(); iterator->Next()) {
            thread = iterator->Item();
            
            // Increase total waiting time
            bool doAging = thread->IncreaseTotalWaitingTime();
            thread->setStartWaitingTime(kernel->stats->totalTicks);

            // Check whether over 1500
            if (doAging) {
                thread->ChangePriority();
            }
            
            if (thread->getPriority() > 49) {
                L3->Remove(thread);
                L2->Insert(thread);

                thread->RemovedFromQueue();
                thread->InsertedIntoQueue(2);
            }
        }
    }    
}

// MP3
bool
Scheduler::CheckPreemptive()
{
    Thread *currThread = kernel->currentThread;
    int currThreadLevel = currThread->getQueueLevel();

    if (currThreadLevel == 1) {
        if (!L1->IsEmpty()) {
            if (L1Compare(currThread, L1->Front()) == 1) {
                return TRUE;
            }
        }
    }
    else if (currThreadLevel == 2) {
        if (!L1->IsEmpty()) {
            return TRUE;
        }
    }
    else {
        if (!L1->IsEmpty()){
            return TRUE;
        }
        if (!L2->IsEmpty()) {
            return TRUE;
        }
        if (!L3->IsEmpty() && currThread->getAccuTicks() >= 100) {
            return TRUE;
        }
    }

    return FALSE;
}

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	    toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}
