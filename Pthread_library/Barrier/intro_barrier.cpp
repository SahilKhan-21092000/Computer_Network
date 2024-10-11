/*#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>

pthread_barrier_t barrier;    //waiting at particula point to achive synchronization
void *routine(void *arg)
{
    printf("Waiting at the Barrier \n");
    pthread_barrier_wait(&barrier);
    printf("We Pass Barrier\n");
    return NULL;
};
int main()
{
    pthread_t th[3];
    pthread_barrier_init(&barrier,NULL,3);
    for(int i=0;i<3;i++)
       {
          if(pthread_create(&th[i],NULL,&routine,NULL)!=0)
              {
                perror("Failed to create thread \n");
                return 1;
              }

       }
       for(int i=0;i<3;i++)
       {
          if(pthread_create(&th[i],NULL,&routine,NULL)!=0)
              {
                perror("Failed to join thread \n");
                return 2;
              }
              
       }
    pthread_barrier_destroy(&barrier); 
    return 1;

};*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

pthread_barrier_t barrier;  // Declare the barrier this code is only work in windows not in mac

void *routine(void *arg) {
    printf("Waiting at the Barrier\n");
    pthread_barrier_wait(&barrier);  // Wait at the barrier
    printf("We Passed the Barrier\n");
    return NULL;
}

int main() {
    pthread_t th[3];  // Declare an array for 3 threads (corrected size)

    // Initialize the barrier for 3 threads
    pthread_barrier_init(&barrier, NULL, 3);

    // Create 3 threads
    for (int i = 0; i < 3; i++) {
        if (pthread_create(&th[i], NULL, &routine, NULL) != 0) {
            perror("Failed to create thread\n");
            return 1;
        }
    }

    // Join 3 threads
    for (int i = 0; i < 3; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread\n");
            return 2;
        }
    }

    // Destroy the barrier after use
    pthread_barrier_destroy(&barrier);

    return 0;
}
