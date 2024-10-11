#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<sys/types.h>
#include<time.h>
pthread_mutex_t StoveMutex[4];
int StoveFuel[4]={100,100,100,100};     //multiple Stove



/*
    StoveFuel[4] is shared resource between multiple chef (thread)

*/




void * routine(void *)
{
    for(int i=0;i<4;i++)
         {   
            if(  pthread_mutex_trylock(&StoveMutex[i])==0)
               {   
                    int fuelneed=(rand()%40);
                    if(StoveFuel[i]-fuelneed<0)
                       {
                       printf("No more fuel in stove %d...going Home\n",i);
                       } 
                    else {
                         StoveFuel[i]=StoveFuel[i]-fuelneed;
                         usleep(50000);
                         printf("Fuel Left in %d stove %d\n",i,StoveFuel[i]);
                        }   
                pthread_mutex_unlock(&StoveMutex[i]);
               }
             else if(i==3)
                    {
                        printf("No stove Available at %d...Wait for Sometimes\n",i);
                        usleep(300000);
                        i=0;
                    }        
       }


    return NULL;
};
int main()
{
    pthread_t chef[10];
    for(int i=0;i<4;i++)
     { 
      pthread_mutex_init(&StoveMutex[i],NULL);
     };
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
       for(int i=0;i<4;i++)
       { 
          pthread_mutex_destroy(&StoveMutex[i]);
       };
    return 1;
}