// Name: Zeyi  Surname: Feng  Student ID: 241ADB024
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  const char* filename = argc > 1 ? argv[1] : "data.txt";
  char line[256];
  int count = 0;

  printf("Writing lines to %s...\n", filename);
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) {
    perror(filename);
    return EXIT_FAILURE;
  }

  int failed =
      fprintf(fp, "Hello, file I/O in C!\nThis is another line.\n") < 0;
  if (fclose(fp) != 0) failed = 1;
  if (failed) {
    perror(filename);
    return EXIT_FAILURE;
  }

  fp = fopen(filename, "r");
  if (fp == NULL) {
    perror(filename);
    return EXIT_FAILURE;
  }

  printf("Reading contents:\n");
  while (fgets(line, sizeof line, fp) != NULL) {
    printf("%s", line);
    count++;
  }

  // Check for read errors before closing the stream.
  failed = ferror(fp);
  if (fclose(fp) != 0) failed = 1;
  if (failed) {
    perror(filename);
    return EXIT_FAILURE;
  }

  printf("Lines read: %d\n", count);
  return EXIT_SUCCESS;
}
