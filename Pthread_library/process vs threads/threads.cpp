#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
//how to create two threads
int x=2;
void * routine (void *)
    {
        x=x+5;
        printf("Test for threads %d\n",getpid());
        sleep(3);
        printf("Value for threads %d\n",x);
        printf("End Threds %d\n",getpid());
        return NULL;
    };

void * routine2(void*)
   {
     sleep(1);
     printf("Value of x :%d\n",x);
     return NULL;
   };  
     int main()
     {
           pthread_t t1,t2;
           if(pthread_create(&t1,NULL,&routine,NULL)!=0) return 1;
           
           if(pthread_create(&t2,NULL,&routine2,NULL)!=0) return 2;
           printf("Hyyy threads\n");
           pthread_join(t1,NULL);
           pthread_join(t2,NULL);
           return 0;
     };