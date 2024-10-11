#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
//how to create two threads
int mails=0;
pthread_mutex_t mutex;
void * routine (void *)
    {
        for(int i=0;i<5000000;i++)
           {
                pthread_mutex_lock(&mutex);   //lock mutex
                mails++;   //racecondition//read increment write
                pthread_mutex_unlock(&mutex);    //unlock mutex        
           };
           return NULL;
    };
     
     int main()
     {
           pthread_t t1,t2,t3,t4;
           pthread_mutex_init(&mutex,NULL);   //initilaize mutex 
           if(pthread_create(&t1,NULL,&routine,NULL)!=0) return 1;
           pthread_create(&t1,NULL,&routine,NULL);
           if(pthread_create(&t3,NULL,&routine,NULL)!=0) return 1;
           pthread_create(&t4,NULL,&routine,NULL);
           pthread_join(t1,NULL);
           pthread_join(t2,NULL);
           pthread_join(t3,NULL);
           pthread_join(t4,NULL);
           printf("mails :%d",mails);
           pthread_mutex_destroy(&mutex);  // destroy mutex 
           return 0;
     };