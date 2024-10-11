#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
//how to create two threads
int mails=0;
void * routine (void *)
    {
        for(int i=0;i<50000;i++)
           {
                mails++;   //racecondition
                           //read increment write
           };
           return NULL;
    };
     
     int main()
     {
           pthread_t t1,t2;
           if(pthread_create(&t1,NULL,&routine,NULL)!=0) return 1;
           pthread_create(&t1,NULL,&routine,NULL);
           pthread_join(t1,NULL);
           pthread_join(t2,NULL);
           printf("mails :%d",mails);
           return 0;
     };