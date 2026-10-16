// Name: Zeyi  Surname: Feng  Student ID: 241ADB024
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STUDENTS 100
#define NAME_LEN 50
#define LINE_LEN 256
#define DATA_FILE "students.txt"

typedef struct Student {
  char name[NAME_LEN];
  int id;
  float gpa;
} Student;

int load_students(Student arr[], int max);
int save_students(const Student arr[], int count);
void add_student(Student arr[], int* count);
void list_students(const Student arr[], int count);
int read_line(const char* prompt, char* buf, size_t size);

int main(void) {
  Student students[MAX_STUDENTS];
  int count = 0;
  char input[LINE_LEN];
  int running = 1;

  count = load_students(students, MAX_STUDENTS);
  printf("Loaded %d student(s) from %s\n", count, DATA_FILE);

  while (running) {
    printf("\n=== Student Management System ===\n");
    printf("1. List students\n");
    printf("2. Add student\n");
    printf("3. Save and Exit\n");

    int choice;
    if (read_line("Select an option: ", input, sizeof input) != 0) {
      choice = 3;
    } else {
      char* end;
      long value = strtol(input, &end, 10);
      choice = (end != input && *end == '\0') ? (int)value : 0;
    }

    switch (choice) {
      case 1:
        list_students(students, count);
        break;
      case 2:
        add_student(students, &count);
        break;
      case 3:
        if (save_students(students, count) != 0) {
          perror(DATA_FILE);
          return EXIT_FAILURE;
        }
        printf("Data saved. Goodbye!\n");
        running = 0;
        break;
      default:
        printf("Invalid option. Try again.\n");
    }
  }

  return EXIT_SUCCESS;
}

// Provided helper: remove newlines and discard excess input.
int read_line(const char* prompt, char* buf, size_t size) {
  printf("%s", prompt);
  fflush(stdout);
  if (fgets(buf, (int)size, stdin) == NULL) {
    return -1;
  }
  size_t len = strcspn(buf, "\r\n");
  if (buf[len] == '\0' && len == size - 1) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
  }
  buf[len] = '\0';
  return 0;
}

// Allow trailing whitespace, but reject other trailing characters.
static int only_space(const char* text) {
  while (*text != '\0') {
    if (!isspace((unsigned char)*text)) return 0;
    text++;
  }
  return 1;
}

int load_students(Student arr[], int max) {
  FILE* fp = fopen(DATA_FILE, "r");
  if (fp == NULL) return 0;  // Normal on the first run.

  char line[LINE_LEN];
  int count = 0;
  while (count < max && fgets(line, sizeof line, fp) != NULL) {
    // Skip oversized lines completely instead of loading fragments.
    if (strchr(line, '\n') == NULL && !feof(fp)) {
      int c;
      while ((c = fgetc(fp)) != '\n' && c != EOF) {
      }
      continue;
    }

    Student temp = {0};
    int end = 0;
    if (sscanf(line, "%49[^,],%d,%f %n", temp.name, &temp.id, &temp.gpa,
               &end) == 3 &&
        end > 0 && !only_space(temp.name) && only_space(line + end)) {
      arr[count++] = temp;
    }
  }

  fclose(fp);
  return count;
}

int save_students(const Student arr[], int count) {
  FILE* fp = fopen(DATA_FILE, "w");
  if (fp == NULL) return -1;

  int failed = 0;
  for (int i = 0; i < count; i++) {
    if (fprintf(fp, "%s,%d,%.2f\n", arr[i].name, arr[i].id, arr[i].gpa) < 0) {
      failed = 1;
      break;
    }
  }
  if (fclose(fp) != 0) failed = 1;
  return failed ? -1 : 0;
}

void add_student(Student arr[], int* count) {
  if (*count >= MAX_STUDENTS) {
    printf("Cannot add more students (limit reached).\n");
    return;
  }

  char name[LINE_LEN], id_text[LINE_LEN], gpa_text[LINE_LEN];
  if (read_line("Enter name: ", name, sizeof name) != 0 ||
      read_line("Enter ID: ", id_text, sizeof id_text) != 0 ||
      read_line("Enter GPA: ", gpa_text, sizeof gpa_text) != 0) {
    return;
  }

  char* id_end;
  errno = 0;
  long id = strtol(id_text, &id_end, 10);
  int valid_id = errno != ERANGE && id_end != id_text && only_space(id_end) &&
                 id >= INT_MIN && id <= INT_MAX;

  char* gpa_end;
  errno = 0;
  float gpa = strtof(gpa_text, &gpa_end);
  // Range comparisons also reject NaN.
  int valid_gpa = errno != ERANGE && gpa_end != gpa_text &&
                  only_space(gpa_end) && gpa >= 0.0f && gpa <= 4.0f;

  if (only_space(name) || strlen(name) >= NAME_LEN || strchr(name, ',') ||
      !valid_id || !valid_gpa) {
    printf("Invalid input. Student not added.\n");
    return;
  }

  strcpy(arr[*count].name, name);
  arr[*count].id = (int)id;
  arr[*count].gpa = gpa;
  (*count)++;
  printf("Student added successfully!\n");
}

void list_students(const Student arr[], int count) {
  if (count == 0) {
    printf("No students found.\n");
    return;
  }

  printf("%-5s %-20s %s\n", "ID", "Name", "GPA");
  for (int i = 0; i < count; i++) {
    printf("%-5d %-20s %.2f\n", arr[i].id, arr[i].name, arr[i].gpa);
  }
}
