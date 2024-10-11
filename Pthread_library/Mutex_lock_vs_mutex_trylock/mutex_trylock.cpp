#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<sys/types.h>
pthread_mutex_t mutex;

void* routine(void *arg)
{
     if(pthread_mutex_trylock(&mutex)==0)   //no wait if lock is not available 
       {    //continue check for lock untill not getting mutex
       printf("Get lock by thread id :%d\n",getpid());
       //sleep(2); if we not comment this then no one able to get the lock except for the first
       pthread_mutex_unlock(&mutex);  //we need to unlock mntex to prevent from deadlock 
       }
     else {
             printf("Didn't get lock by thread id :%d\n",getpid()); 
          }  
     return NULL;

};
int main()
{
       pthread_t th[100];
       pthread_mutex_init(&mutex,NULL);
       for(int i=0;i<100;i++)
          {
            if(pthread_create(&th[i],NULL,&routine,NULL)!=0)
                {
                    perror("failed to create thread\n");
                }
          }
          for(int i=0;i<10;i++)
          {
            if(pthread_join(th[i],NULL)!=0)
                {
                    perror("failed to join thread\n");
                }
          }
          pthread_mutex_destroy(&mutex);
          return 1;

};