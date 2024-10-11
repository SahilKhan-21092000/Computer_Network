//create ten thread each print unique value 
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
int primes[10]={1,2,3,5,7,11,13,17,19,23};
void* routine(void* arg)
 {
     //sleep(1);   //due to race condition
     int *index=(int*)arg;
     printf("%d  ",primes[*index]);
     return NULL;
 };
int main()
{
    pthread_t th[10];
    int i; 
    for(i=0;i<10;i++)
        {
            int *a=(int*)malloc(sizeof(int));
            *a=i;
            if(pthread_create(&th[i],NULL,&routine,a)!=0)
                   {
                    perror("failed to create thread\n");
                    return 1;
                   }
        }  
    for(int i=0;i<10;i++)
        {
            if(pthread_join(th[i],NULL)!=0)
                   {
                    perror("failed to join thread\n");
                    return 2;
                   }
        } ;

    return 1;
}