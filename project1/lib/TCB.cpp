#include "TCB.h"



TCB::TCB(int tid,void *(*start_routine)(void *arg), void *arg, State state)
{   
    this->_context = new ucontext_t;
    saveContext();

    this->_tid=tid;
    this->_state = state;

    this->_stack = new char[STACK_SIZE];
    this->sp = this->_stack + STACK_SIZE;

    // this->pc = &stub;
    this->_context->uc_stack.ss_sp = this->sp;
    this->_context->uc_stack.ss_size = STACK_SIZE;
    this->_context->uc_stack.ss_flags = 0;
    // *(this->sp) = arg;
    // this->sp--;
    // *(this->sp) = start_routine;
    // this->sp--;

    
}

TCB::~TCB()
{
    delete this->sp;
    delete this->_context;
}

void TCB::setState(State state)
{
    this->_state = state;
}

State TCB::getState() const
{
    return this->_state;
}

int TCB::getId() const
{
    return this->_tid;
}

void TCB::increaseQuantum()
{
    this->_quantum++;
}

int TCB::getQuantum() const
{
    return this->_quantum;
}

int TCB::saveContext()
{   
    
    if(getcontext(this->_context)!=0){
        cout<<"saveContent fail"<<endl;
        return -1;
    }
    else{
        return 1;
    }
}

ucontext_t* TCB::getContext(){
    return this->_context;
}

void TCB::loadContext()
{
    if(setcontext(this->_context)==-1){
        cout<<"LoadContext Error"<<endl;
    }
}
