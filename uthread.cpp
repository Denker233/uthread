#include "uthread.h"
#include "TCB.h"
#include <cassert>
#include <deque>

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

int find_next_index(){
	for(int i = 0; i < MAX_THREAD_NUM;i++){
		if(threads[i]==nullptr){
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
static void startInterruptTimer()
{
	// TODO
}

// Block signals from firing timer interrupt
static void disableInterrupts()
{
	// TODO
}

// Unblock signals to re-enable timer interrupt
static void enableInterrupts()
{
	// TODO
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

// Switch to the next ready thread
static void switchThreads()
{
	// TODO
	volatile int main_flag = 0, flag=0;
	if(running_thread==nullptr){// main thread
		main_thread_context = new ucontext_t;
		ucontext_t* r_context = main_thread_context;
		printf("main thread switching\n");

		int ret_val = getcontext(r_context);
		if (main_flag == 1) {
        	return;
    	}

		main_flag = 1;
	
		TCB* next_thread =popFromReadyQueue();
		if(next_thread==running_thread){
			printf("no other thread to run\n");
		}
		cout<<"main flag and flag in main branch: "<<main_flag<<0<<flag<<endl;
		running_thread = next_thread;
		setcontext(r_context);// 
	}
	else{
		printf("in else of switch threads\n");
		ucontext_t* r_context = running_thread->getContext();
		int ret_val = getcontext(r_context);
		cout<<"main flag and flag: "<<main_flag<<flag<<endl;
		if (flag == 1) {
			printf("flag==1 in else of switch threads\n");
			return;
		}

		flag = 1;
		printf("first time in else of switch after setting flag\n");
		addToReadyQueue(running_thread);
		TCB* next_thread =popFromReadyQueue();
		cout<<"Next thread in else sta"<<next_thread->getId()<<endl;
		if(next_thread==running_thread){
			printf("no other thread to run\n");
		}
		running_thread = next_thread;
		setcontext(next_thread->getContext());// 
	}

	

}

// Library functions -----------------------------------------------------------

// The function comments provide an (incomplete) summary of what each library
// function must do

// Starting point for thread. Calls top-level thread function
void stub(void *(*start_routine)(void *), void *arg)
{
	// TODO
	start_routine(arg);
	uthread_exit(0);
}

int uthread_init(int quantum_usecs)
{
	// Initialize any data structures
	// Setup timer interrupt and handler
	// Create a thread for the caller (main) thread
	return -1;
}

int uthread_create(void *(*start_routine)(void *), void *arg)
{
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
    return 1;
}

int uthread_join(int tid, void **retval)
{
	// If the thread specified by tid is already terminated, just return
	// If the thread specified by tid is still running, block until it terminates
	// Set *retval to be the result of thread if retval != nullptr
	// if(thread[tid]==nullptr){
	// 	// terminated
	// 	return 1;
	// }
	// if(thread[tid]->getState()==RUNNING){
	// 	while
	// }
	return -1;
}

int uthread_yield(void)
{
	// TODO
	return -1;
}

void uthread_exit(void *retval)
{
	// If this is the main thread, exit the program
	// Move any threads joined on this thread back to the ready queue
	// Move this thread to the finished queue
}

int uthread_suspend(int tid)
{
	// Move the thread specified by tid from whatever state it is
	// in to the block queue
	return -1;
}

int uthread_resume(int tid)
{
	// Move the thread specified by tid back to the ready queue
	return -1;
}

int uthread_self()
{
	// TODO
	return -1;
}

int uthread_get_total_quantums()
{
	// TODO
	return -1;
}

int uthread_get_quantums(int tid)
{
	// TODO
	return -1;
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
			switchThreads();
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
		cout << "in y (" << i << ")" << endl;
		if (i % 3 == 0) {
			cout << "y: switching" <<x<< endl;
			switchThreads();
		}
		sleep(1); 
	}
}

int main() 
{
	int i =1;
	cout<<"Entrying main thread..."<<endl;
	uthread_create(x,(void*)1);
	printf("after first create\n");
	uthread_create(y,(void*)1);
	printf("after second create\n");
	for (const auto& element : ready_queue) {
        cout << element->getId() << " here is the id"<<endl;
    }
	ucontext_t* context = threads[0]->getContext();
	setcontext(context);
	
	while(1){
		i++;
	}
	
	return 0;
}
