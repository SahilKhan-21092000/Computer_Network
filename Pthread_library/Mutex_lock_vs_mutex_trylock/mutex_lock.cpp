#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<sys/types.h>
pthread_mutex_t mutex;

void* routine(void *arg)
{
     pthread_mutex_lock(&mutex);   //continue check for lock untill not getting mutex
    printf("Get lock by thread id :%d\n",getpid()); // wait if lock is not available 
    sleep(2);
    pthread_mutex_unlock(&mutex);  //we need to unlock mntex to prevent from deadlock 
     return NULL;

};
int main()
{
       pthread_t th[4];
       pthread_mutex_init(&mutex,NULL);
       for(int i=0;i<4;i++)
          {
            if(pthread_create(&th[i],NULL,&routine,NULL)!=0)
                {
                    perror("failed to create thread\n");
                }
          }
          for(int i=0;i<4;i++)
          {
            if(pthread_join(th[i],NULL)!=0)
                {
                    perror("failed to join thread\n");
                }
          }
          pthread_mutex_destroy(&mutex);
          return 1;

};