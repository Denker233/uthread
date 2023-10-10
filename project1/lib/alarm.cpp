#include "uthread.h"
#include "TCB.h"
#include <cassert>
#include <deque>
#include <signal.h>
#include <sys/time.h>
#include <mutex>
#include <condition_variable>

int fire_time=0;
static void startInterruptTimer(int quantum_usecs)
{	
	printf("inside start interrupt\n");
	// TODO
	struct itimerval timer;
	timer.it_value.tv_sec = 0;
   	timer.it_value.tv_usec = quantum_usecs;
   	timer.it_interval.tv_sec = 0;
   	timer.it_interval.tv_usec = quantum_usecs;
	setitimer(ITIMER_VIRTUAL, &timer, nullptr);
}


void timerhandler(int quantum_usecs){
	fire_time++;
	cout<<"alarm is fired !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<fire_time<<endl;
	startInterruptTimer(quantum_usecs);
}


int main(){
    
    struct sigaction sa;
    sa.sa_handler = timerhandler;
	sa.sa_flags=0;
	if (sigemptyset(&sa.sa_mask) < -1)
    {
        cout << "ERROR: Failed to empty to set" << endl;
        exit(1);
    }
	if (sigaddset(&sa.sa_mask, SIGVTALRM))
    {
        cout << "ERROR: Failed to add to set" << endl;
        exit(1);
    }
	sigaction(SIGVTALRM, &sa, nullptr);

	startInterruptTimer(1);

    while(1){
        if(fire_time==10){
            break;
        }
    }
    
}