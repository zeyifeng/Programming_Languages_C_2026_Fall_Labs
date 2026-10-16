// Name: Zeyi  Surname: Feng  Student ID: 241ADB024
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 50

typedef struct Student {
  char name[MAX_NAME_LEN];
  int age;
  float gpa;
} Student;

int save_student(const Student* s, const char* filename);
int load_student(const char* filename, Student* out);

int main(void) {
  Student s1 = {0};
  strcpy(s1.name, "Alice");
  s1.age = 21;
  s1.gpa = 3.75f;

  Student loaded = {0};
  const char* filename = "student.txt";

  printf("Saving student to file...\n");
  if (save_student(&s1, filename) != 0) {
    fprintf(stderr, "Could not save student.\n");
    return EXIT_FAILURE;
  }

  printf("Loading student from file...\n");
  if (load_student(filename, &loaded) != 0) {
    fprintf(stderr, "Could not load student.\n");
    return EXIT_FAILURE;
  }

  printf("Loaded student: %s, %d, GPA %.2f\n", loaded.name, loaded.age,
         loaded.gpa);
  return EXIT_SUCCESS;
}

int save_student(const Student* s, const char* filename) {
  FILE* fp = fopen(filename, "w");
  if (fp == NULL) return -1;

  int failed = fprintf(fp, "%s %d %.2f\n", s->name, s->age, s->gpa) < 0;
  if (fclose(fp) != 0) failed = 1;
  return failed ? -1 : 0;
}

int load_student(const char* filename, Student* out) {
  FILE* fp = fopen(filename, "r");
  if (fp == NULL) return -1;

  // Preserve the caller's record if loading fails.
  Student temp = {0};
  int fields = fscanf(fp, "%49s %d %f", temp.name, &temp.age, &temp.gpa);
  int closed = fclose(fp);
  if (fields != 3 || closed != 0) return -1;

  *out = temp;
  return 0;
}
