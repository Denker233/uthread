#include "TCB.h"

TCB::TCB(int tid,Priority pr, void *(*start_routine)(void *arg), void *arg, State state)
{   
    saveContext();
    this->_tid=tid;
    this->_state = state;
    this->_pr = pr;
    this->_stack = new char[STACK_SIZE];
    this->sp = this->stack + STACK_SIZE;
    this->pc = stub(start_routine,arg);
    *(tcb->sp) = arg;
    tcb->sp--;
    *(tcb->sp) = start_routine;
    tcb->sp--;
    


    
}

TCB::~TCB()
{

}

void TCB::setState(State state)
{
}

State TCB::getState() const
{
}

int TCB::getId() const
{
}

void TCB::increaseQuantum()
{
    quantum++;
}

int TCB::getQuantum() const
{
    return _quantum;
}

int TCB::saveContext()
{
    if(getcontext(*_context)!=0){
        cout<<"saveContent fail"<<endl;
        return -1;
    }
    else{
        return 1;
    }
}

void TCB::loadContext()
{
    if(setcontext(*_context)==-1){
        cout<<"LoadContext Error"<<endl;
    }
}
