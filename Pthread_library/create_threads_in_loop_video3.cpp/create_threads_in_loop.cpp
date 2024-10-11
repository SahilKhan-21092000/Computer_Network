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
           printf("Mails :%d\n",mails);
           return NULL;
    };
     
     int main()
     {
           pthread_t threads[4];
           pthread_mutex_init(&mutex,NULL);   //initilaize mutex 
           for(int i=0;i<4;i++)
             {
                if(pthread_create(threads+i,NULL,&routine,NULL)!=0)
                  { 
                     printf("failed to create threads :\n");
                     return 1;
                  }
                  printf("Threads %d start their execution \n",i);     //one threads at the time
                if(pthread_join(threads[i],NULL)!=0)
                   {
                    printf("Failed to join thread :\n");
                    return 2;
                   }  ;
                printf("Threads %d end their execution \n",i);   


             };
           printf("mails :%d",mails);
           pthread_mutex_destroy(&mutex);  // destroy mutex 
           return 0;
     };