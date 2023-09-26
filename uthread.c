#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#define STACK_SIZE 10240

int uthread_create(void *(*start_routine)(void *), void *arg){
    ucontext_t* ucp  = malloc(sizeof(ucontext_t));

    if(ucp==NULL){
        printf("malloc fail for initilize context");
        return -1;
    }
    if (getcontext(ucp)!=0){
        printf("Can not save context");
    }

    char* new_stack_pointer = malloc(STACK_SIZE);
    ucp->uc_stack.ss_sp=new_stack_pointer;
    ucp->uc_stack.ss_size = STACK_SIZE;

    

    makecontext(ucp,start_routine,1,&arg);
    return 1;
    // free(ucp)
}