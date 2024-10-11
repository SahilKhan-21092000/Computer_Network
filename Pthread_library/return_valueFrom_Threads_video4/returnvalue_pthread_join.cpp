#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
void* roll_dice(void *arg )
  {
        int value=(rand()%6) +1;
        printf("Thread Value :%d \n",value);
        int *result=(int*)malloc(sizeof(int));
         printf("Thread address :%p \n",result);
        *result=value;
        return result;
        
  };
  int main()
  {   
      srand(time(NULL));
      int *res;
      pthread_t th;
      if(pthread_create(&th,NULL, &roll_dice ,NULL)!=0)
      { printf("failed to create threads :\n");
        return 1;
      }
      if((pthread_join(th,(void**)&res))!=0) //doube pointer to void 
        {
            return 2;
        }
        printf("Main Address :%p",res);
         printf("\nMain value :%d",*res);
         free(res);


  };