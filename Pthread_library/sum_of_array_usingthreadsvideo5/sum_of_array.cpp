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
     int sum=0;
     for(int i=0;i<5;i++)
      {
          sum+=primes[i+ *index];
      };
      free (index);
      *(int * ) arg = sum;
     return arg;
 };
int main()
{
    pthread_t th[2];
    int i; 
    for(i=0;i<2;i++)
        {
            int *a=(int*)malloc(sizeof(int));
            *a=i*5;
            if(pthread_create(&th[i],NULL,&routine,a)!=0)
                   {
                    perror("failed to create thread\n");
                    return 1;
                   }
        }  
        int globalsum=0;
    for(int i=0;i<2;i++)
        {
            int *r;
            if(pthread_join(th[i],(void **)&r)!=0)
                   {
                    perror("failed to join thread\n");
                    return 2;
                   }
            globalsum+=*r;
        } ;
    printf("Sum od Array :%d",globalsum);
    return 1;
}