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

int save_student_binary(const Student* s, const char* filename);
int load_student_binary(const char* filename, Student* out);
long file_size(const char* filename);

int main(void) {
  // Clear unused name bytes before writing the entire struct.
  Student s1 = {0};
  strcpy(s1.name, "Alice");
  s1.age = 21;
  s1.gpa = 3.75f;
  Student loaded = {0};
  const char* bin_file = "student.bin";
  const char* txt_file = "student.txt";

  printf("Saving student in binary format...\n");
  if (save_student_binary(&s1, bin_file) != 0) {
    perror(bin_file);
    return EXIT_FAILURE;
  }

  FILE* fp = fopen(txt_file, "w");
  if (fp == NULL) {
    perror(txt_file);
    return EXIT_FAILURE;
  }
  int failed = fprintf(fp, "%s %d %.2f\n", s1.name, s1.age, s1.gpa) < 0;
  if (fclose(fp) != 0) failed = 1;
  if (failed) {
    perror(txt_file);
    return EXIT_FAILURE;
  }

  printf("Loading student from binary file...\n");
  if (load_student_binary(bin_file, &loaded) != 0) {
    fprintf(stderr, "Could not load binary student.\n");
    return EXIT_FAILURE;
  }

  long text_size = file_size(txt_file);
  long binary_size = file_size(bin_file);
  if (text_size < 0 || binary_size < 0) {
    fprintf(stderr, "Could not determine file size.\n");
    return EXIT_FAILURE;
  }

  printf("Loaded student: %s, %d, GPA %.2f\n", loaded.name, loaded.age,
         loaded.gpa);
  printf("Text file size: %ld bytes\n", text_size);
  printf("Binary file size: %ld bytes\n", binary_size);
  return EXIT_SUCCESS;
}

int save_student_binary(const Student* s, const char* filename) {
  FILE* fp = fopen(filename, "wb");
  if (fp == NULL) return -1;
  int failed = fwrite(s, sizeof(Student), 1, fp) != 1;
  if (fclose(fp) != 0) failed = 1;
  return failed ? -1 : 0;
}

int load_student_binary(const char* filename, Student* out) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) return -1;
  Student temp = {0};
  int failed = fread(&temp, sizeof(Student), 1, fp) != 1;
  if (fclose(fp) != 0) failed = 1;
  if (failed) return -1;
  *out = temp;
  return 0;
}

long file_size(const char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL) return -1;
  long size = -1;
  if (fseek(fp, 0, SEEK_END) == 0) size = ftell(fp);
  if (fclose(fp) != 0) return -1;
  return size;
}
