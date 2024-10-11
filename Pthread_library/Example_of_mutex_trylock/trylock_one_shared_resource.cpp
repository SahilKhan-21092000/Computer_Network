#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<sys/types.h>
#include<time.h>
pthread_mutex_t StoveMutex;
int StoveFuel=100;     //single Stove



/*
    StoveFuel is shared resource between multiple chef (thread)

*/




void * routine(void *)
{
    pthread_mutex_lock(&StoveMutex);
    int fuelneed=(rand()%40);
    if(StoveFuel-fuelneed<0)
       {
          printf("No more fuel ....going Home\n");
       } 
    else {
              StoveFuel=StoveFuel-fuelneed;
              usleep(50000);
              printf("Fuel Left %d\n",StoveFuel);
        }   
    pthread_mutex_unlock(&StoveMutex);


    return NULL;
};
int main()
{
    pthread_t chef[10];
    pthread_mutex_init(&StoveMutex,NULL);
    for(int i=0;i<9;i++)
       {
            if(pthread_create(&chef[i],NULL,&routine,NULL)!=0)
               {
                       perror("Failed to create thread for chef\n");
                       return 1;
               }

       }
       for(int i=0;i<9;i++)
       {
            if(pthread_join(chef[i],NULL)!=0)
               {
                       perror("Failed to join thread for chef\n");
                       return 2;
               }

       };
    pthread_mutex_destroy(&StoveMutex);
    return 1;
}