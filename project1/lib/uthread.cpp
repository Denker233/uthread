#include "uthread.h"
#include "TCB.h"
#include <cassert>
#include <deque>
#include <signal.h>
#include <sys/time.h>
#include <mutex>
#include <condition_variable>

using namespace std;



// Finished queue entry type
typedef struct finished_queue_entry
{
	TCB *tcb;	  // Pointer to TCB
	void *result; // Pointer to thread result (output)
} finished_queue_entry_t;

// Join queue entry type
typedef struct join_queue_entry
{
	TCB *tcb;			 // Pointer to TCB
	int waiting_for_tid; // TID this thread is waiting on
} join_queue_entry_t;



// You will need to maintain structures to track the state of threads
// - uthread library functions refer to threads by their TID so you will want
//   to be able to access a TCB given a thread ID
// - Threads move between different states in their lifetime (READY, BLOCK,
//   FINISH). You will want to maintain separate "queues" (doesn't have to
//   be that data structure) to move TCBs between different thread queues.
//   Starter code for a ready queue is provided to you
// - Separate join and finished "queues" can also help when supporting joining.
//   Example join and finished queue entry types are provided above

TCB* running_thread;
TCB* threads[MAX_THREAD_NUM]={nullptr};
int next_thread_index=0;
ucontext_t* main_thread_context;
struct sigaction sa;
static bool interrupts_enabled = true;
struct itimerval timer;


int find_next_index(){
	for(int i = 0; i < MAX_THREAD_NUM;i++){
		if(threads[i]==nullptr){
			cout<<"each index inside find next index"<<i<<endl;
			return i;
		}
	}
	return -1;
}



// Queues
static deque<TCB *> ready_queue;
static deque<finished_queue_entry_t *> finished_queue;
static deque<join_queue_entry_t *> join_queue;


// Interrupt Management --------------------------------------------------------

// Start a countdown timer to fire an interrupt
static void startInterruptTimer(int quantum_usecs)
{	
	printf("inside start interrupt\n");
	// TODO
	timer.it_value.tv_sec = 0;
   	timer.it_value.tv_usec = quantum_usecs;
   	timer.it_interval.tv_sec = 0;
   	timer.it_interval.tv_usec = quantum_usecs;
	setitimer(ITIMER_VIRTUAL, &timer, nullptr);
}

//disable and enable interrupt functions are from code example on Canvas
static void disableInterrupts()
{	
	printf("inside disable interrupt\n");
    assert(interrupts_enabled);
    sigprocmask(SIG_BLOCK,&sa.sa_mask, NULL);
    interrupts_enabled = false;
}

static void enableInterrupts()
{	
	printf("inside able interrupt\n");
    assert(!interrupts_enabled);
    interrupts_enabled = true;
    sigprocmask(SIG_UNBLOCK,&sa.sa_mask, NULL);
}

// Queue Management ------------------------------------------------------------

void addToFinishQueue(finished_queue_entry_t *entry)
{
	finished_queue.push_back(entry);
}
void addToJoinQueue(join_queue_entry_t *entry)
{
	join_queue.push_back(entry);
}

finished_queue_entry_t *popFromFinishedQueue()
{
	assert(!finished_queue.empty());

	finished_queue_entry_t *finish_queue_head = finished_queue.front();
	finished_queue.pop_front();
	return finish_queue_head;
}

int removeFromFinishedQueue(int tid)
{
	for (deque<finished_queue_entry_t *>::iterator iter = finished_queue.begin(); iter != finished_queue.end(); ++iter)
	{
		if (tid == (*iter)->tcb->getId())
		{
			finished_queue.erase(iter);
			return 0;
		}
	}

	// Thread not found
	return -1;
}

join_queue_entry_t *popFromJoinQueue()
{
	assert(!join_queue.empty());

	join_queue_entry_t *join_queue_head = join_queue.front();
	join_queue.pop_front();
	return join_queue_head;
}

int removeFromJoinQueue(int tid)
{
	for (deque<join_queue_entry_t *>::iterator iter = join_queue.begin(); iter != join_queue.end(); ++iter)
	{
		if (tid == (*iter)->tcb->getId())
		{
			join_queue.erase(iter);
			return 0;
		}
	}

	// Thread not found
	return -1;
}


// Add TCB to the back of the ready queue
void addToReadyQueue(TCB *tcb)
{
	ready_queue.push_back(tcb);
}

// Removes and returns the first TCB on the ready queue
// NOTE: Assumes at least one thread on the ready queue
TCB *popFromReadyQueue()
{
	assert(!ready_queue.empty());

	TCB *ready_queue_head = ready_queue.front();
	ready_queue.pop_front();
	return ready_queue_head;
}

// Removes the thread specified by the TID provided from the ready queue
// Returns 0 on success, and -1 on failure (thread not in ready queue)
int removeFromReadyQueue(int tid)
{
	for (deque<TCB *>::iterator iter = ready_queue.begin(); iter != ready_queue.end(); ++iter)
	{
		if (tid == (*iter)->getId())
		{
			ready_queue.erase(iter);
			return 0;
		}
	}

	// Thread not found
	return -1;
}

// Helper functions ------------------------------------------------------------
int find_if_terminated(int tid){
	for (deque<finished_queue_entry_t *>::iterator iter = finished_queue.begin(); iter != finished_queue.end(); ++iter)
	{
		if (tid == (*iter)->tcb->getId())
		{
			return 1;
		}
	}
	return -1;
}

// Switch to the next ready thread
static void switchThreads(int placeholder)
{	disableInterrupts();
	// TODO
	printf("inside swithc context\n");
	volatile int main_flag = 0, flag=0;
	printf("in else of switch threads\n");
	ucontext_t* r_context = running_thread->getContext();
	int ret_val = getcontext(r_context);
	if (flag == 1) {
		printf("flag==1 in else of switch threads\n");
		if(running_thread->getId()==0){
			printf("main thread exitting\n");
		}
		return;
	}
	flag = 1;
	printf("first time in else of switch after setting flag\n");
	if (running_thread->getState()==RUNNING||running_thread->getState()==READY){
		running_thread->setState(READY);
		addToReadyQueue(running_thread);
	}
	// if(running_thread->getState()==FINISHED&&running_thread->getId()!=0){//from yield
	// 	cout<<"Delete id: "<<running_thread->getId()<<endl;
	// 	delete threads[running_thread->getId()];
	// 	threads[running_thread->getId()] = nullptr;
	// }
	
	TCB* next_thread =popFromReadyQueue();
	cout<<"Next thread in else"<<next_thread->getId()<<endl;
	running_thread = next_thread;
	running_thread->setState(RUNNING);
	enableInterrupts();
	setcontext(next_thread->getContext());// 
	
	

}

// Library functions -----------------------------------------------------------

// The function comments provide an (incomplete) summary of what each library
// function must do

// Starting point for thread. Calls top-level thread function
void stub(void *(*start_routine)(void *), void *arg)
{
	// TODO
	void* result=start_routine(arg);
	uthread_exit(result);
}


int uthread_init(int quantum_usecs)
{
	// Initialize any data structures
	// Setup timer interrupt and handler
	// Create a thread for the caller (main) thread
	sa.sa_handler = switchThreads;
	sa.sa_flags=0;
	if (sigemptyset(&sa.sa_mask) < -1)
    {
        cout << "ERROR: Failed to empty to set" << endl;
        exit(1);
    }

	sigaction(SIGVTALRM, &sa, nullptr);

	startInterruptTimer(quantum_usecs);
  	

	// no thread at  queue then switch back to main thread
	TCB* main_thread = new TCB(0,RUNNING);
	running_thread = main_thread;
	threads[0]=main_thread;

	
	return 0;
}

int uthread_create(void *(*start_routine)(void *), void *arg)
{	
	disableInterrupts();
	// Create a new thread and add it to the ready queue
	next_thread_index = find_next_index();
	State state = READY;
	TCB* tcb =new TCB(next_thread_index,start_routine,arg,state);
	threads[next_thread_index]=tcb;
	cout<<"thread index"<<next_thread_index<<endl;
	ucontext_t* context = tcb->getContext();
    makecontext((context),(void (*)(void))(stub),2,start_routine,arg);
	addToReadyQueue(tcb);
	printf("end of thread_create\n");
	cout<<"the state of all thread"<<threads[next_thread_index]->getState()<<endl;
	enableInterrupts();
    return next_thread_index;
}

int uthread_join(int tid, void **retval)
{
	// If the thread specified by tid is already terminated, just return
	// If the thread specified by tid is still running, block until it terminates
	// Set *retval to be the result of thread if retval != nullptr
	cout<<"running thread "<<running_thread->getId()<< "and waiting for "<<tid<<endl;
	for(finished_queue_entry_t* entry:finished_queue){
		cout<<entry->tcb->getId()<<endl;
		if (entry->tcb->getId()==tid){
			if(retval!=nullptr){
				*retval=entry->result;
				cout<<"Delete id: "<<tid<<endl;
			// delete threads[tid];
			// threads[tid] = nullptr;
				removeFromFinishedQueue(tid);
				delete threads[tid];
				threads[tid] = nullptr;
			}
			// cout<<"Delete id: "<<tid<<endl;
			// // delete threads[tid];
			// // threads[tid] = nullptr;
			// removeFromFinishedQueue(tid);
			// delete threads[tid];
			// threads[tid] = nullptr;
			return 1;
		}
	}
	printf("inside join\n");
	cout<<"the id of the join thread "<<threads[tid]->getId()<<endl;
	cout<<"the state of the join thread "<<threads[tid]->getState()<<endl;
	if(threads[tid]->getState()==RUNNING||threads[tid]->getState()==READY){
		printf("in the if of join\n");
		join_queue_entry_t* join_entry =new join_queue_entry_t;
		join_entry->tcb = running_thread;
		join_entry->waiting_for_tid = tid;
		addToJoinQueue(join_entry);
		running_thread->setState(BLOCK);
		while(threads[tid]->getState()!=FINISHED){
			continue;
		}
		printf("after the while loop in join\n");
		if(retval!=nullptr){
			printf("inside the retval!=nullptr\n");
			for(finished_queue_entry_t* entry:finished_queue){
				if (entry->tcb->getId()==tid){
					cout<<"Result in the join"<<entry->result<<endl;
					*retval=entry->result;
				}
			}
		}	
		//clean up
		cout<<"Delete id: "<<tid<<endl;
		delete threads[tid];
		threads[tid] = nullptr;
		return 1;

		
	}
	return -1;
}

int uthread_yield()
{
	// TODO
	// setitimer(ITIMER_VIRTUAL, &timer, nullptr);
	switchThreads(1);
	return 1;
}

void uthread_exit(void *retval)
{
	// If this is the main thread, exit the program
	// Move any threads joined on this thread back to the ready queue
	// Move this thread to the finished queue
	//if other thread is waiting for me then alert
	cout<<"The result in exit is: "<<retval;
	if(threads[0]->getState()==RUNNING){
		printf("main thread exiting!!!\n");
		exit(0);
	}
	for(join_queue_entry_t* join_entry:join_queue){
		if(join_entry->waiting_for_tid==running_thread->getId()){
			uthread_resume(join_entry->tcb->getId());
			cout<<"Resumed tid"<<join_entry->tcb->getId()<<endl;
		}
	}
	cout<<"running thread id in exit"<<running_thread->getId()<<endl;
	finished_queue_entry_t* finish_entry = new finished_queue_entry_t;
	finish_entry->tcb = running_thread;
	finish_entry->result=retval;
	addToFinishQueue(finish_entry);
	running_thread->setState(FINISHED);
	uthread_yield();
	

}

int uthread_suspend(int tid)
{
	// Move the thread specified by tid from whatever state it is
	// in to the block queue
	if(threads[tid]->getState()==RUNNING){
		join_queue_entry_t * join_entry =new join_queue_entry_t;
		join_entry->tcb = threads[tid];
		addToJoinQueue(join_entry);
		threads[tid]->setState(BLOCK);
		uthread_yield();
		return 1;

	}
	else if (threads[tid]->getState()==BLOCK){
		printf("BLOCK already!\n");
		return -1;
	}
	else if (threads[tid]->getState()==READY){
		removeFromReadyQueue(tid);
		join_queue_entry_t * join_entry =new join_queue_entry_t;
		join_entry->tcb = threads[tid];
		join_entry->waiting_for_tid = running_thread->getId();
		threads[tid]->setState(BLOCK);
		addToJoinQueue(join_entry);
		return 1;
	}
	else {
		printf("Finished already!\n");
		return -1;
	}
	
}

int uthread_resume(int tid)
{
	// Move the thread specified by tid back to the ready queue
	if(threads[tid]->getState()==RUNNING){
		printf("Running already!\n");
		return 1;
	}
	else if (threads[tid]->getState()==BLOCK){
		removeFromJoinQueue(tid);
		threads[tid]->setState(READY);
		addToReadyQueue(threads[tid]);
		return 1;
	}
	else if (threads[tid]->getState()==READY){
		printf("ready already!\n");
		return -1;
	}
	else {
		printf("Finished already!\n");
		return -1;
	}
	return -1;
}

int uthread_self()
{
	// TODO
	if (running_thread != nullptr) {
		return running_thread->getId();
	}
	return -1;
	
}

int uthread_get_total_quantums()
{
	// TODO
	int retval = 0;
	for(int i = 0; i < MAX_THREAD_NUM;i++){
		if(threads[i]!=nullptr){
			retval += threads[i]->getQuantum();
		}
	}
	return retval;
}

int uthread_get_quantums(int tid)
{
	// TODO
	return threads[tid]->getQuantum();
}
void* x(void* y)
{	
	int i=0;
	cout<<"Thread 1 is running"<<endl;
	while (1) {
		++i;
		cout << "in x (" << i << ")" << endl;
		if (i % 2 == 0) {
			cout << "x: switching"<<y << endl;
			switchThreads(1);
		}
		sleep(1);
	}
}

void* y(void* x)
{
	cout<<"Thread 2 is running"<<endl;
	int i =0;
	while (1) {
		++i;
		if(i>5){
			cout << "y: switching" <<x<< endl;
			uthread_yield();
			printf("after calling yield\n");
			cout << "in y (" << i << ")" << endl;
		}
		// if (i % 3 == 0) {
		// 	cout << "y: switching" <<x<< endl;
		// 	switchThreads(1);
		// }
		sleep(1); 
	}
}

// int main() 
// {	
// 	uthread_init(3);
// 	int i =1;
// 	cout<<"Entrying main thread..."<<endl;
// 	uthread_create(x,(void*)1);
// 	printf("after first create\n");
// 	uthread_create(y,(void*)1);
// 	printf("after second create\n");
// 	for (const auto& element : ready_queue) {
//         cout << element->getId() << " here is the id"<<endl;
//     }
// 	switchThreads(1);
	
// 	while(1){
// 		i++;
// 	}
	
// 	return 0;
// }