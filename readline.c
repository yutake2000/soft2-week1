#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
  const size_t bufsize = 500;
  char buf[bufsize];

  if (argc != 2 ){
    fprintf(stderr,"usage: %s filename\n",argv[0]);
    exit(EXIT_FAILURE);
  }
  
  FILE *fp;
  
  if ((fp = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "error: can't open %s\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  while (fgets(buf, bufsize, fp) != NULL) {
    size_t len = strlen(buf) - 1;
    printf("%zd\n", len);
  }

  fclose(fp);

  return EXIT_SUCCESS;
}