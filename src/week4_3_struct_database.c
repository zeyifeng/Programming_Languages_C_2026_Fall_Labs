/*
 * Author: ZeyiFeng
 * Student ID: 241ADB024
 * Read a dynamic array of students and print them in input order.
 */
#include <stdio.h>
#include <stdlib.h>

struct Student {
  char name[50];
  int id;
  float grade;
};

int main(void) {
  int n;
  printf("Enter number of students: ");
  if (scanf("%d", &n) != 1 || n <= 0) {
    printf("Invalid number.\n");
    return 1;
  }

  /* Allocate one record per student and check allocation success. */
  struct Student* students = malloc((size_t)n * sizeof(struct Student));
  if (students == NULL) {
    printf("Memory allocation failed.\n");
    return 1;
  }

  for (int i = 0; i < n; i++) {
    printf("Enter data for student %d: ", i + 1);
    /* Leave room for the terminating zero in each name. */
    if (scanf("%49s %d %f", students[i].name, &students[i].id,
              &students[i].grade) != 3) {
      free(students); /* Avoid a memory leak on invalid input. */
      printf("Invalid input.\n");
      return 1;
    }
  }

  printf("\n");
  printf("%-6s %-11s %s\n", "ID", "Name", "Grade");
  for (int i = 0; i < n; i++) {
    printf("%-6d %-11s %.1f\n", students[i].id, students[i].name,
           students[i].grade);
  }

  free(students);
  return 0;
}
