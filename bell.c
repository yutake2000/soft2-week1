#include <stdio.h>
#include <unistd.h> // sleepのためにinclude 

int main (int argc, char **argv){
  for (int i = 10 ; i > 0 ; i--){
    printf("\a% 5d",i);
    fflush(stdout);
    sleep(1);
    printf("\r");
  }
  printf("\n");
  return 0;
}