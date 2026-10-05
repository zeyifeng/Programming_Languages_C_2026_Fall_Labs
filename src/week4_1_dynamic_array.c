/*
 * Author: ZeyiFeng
 * Student ID: 241ADB024
 * Allocate an integer array, calculate its sum and average, then free it.
 */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int n;
  printf("Enter number of elements: ");
  if (scanf("%d", &n) != 1 || n <= 0) {
    printf("Invalid size.\n");
    return 1;
  }

  /* Allocate space for all elements and check for failure. */
  int* arr = malloc((size_t)n * sizeof(int));
  if (arr == NULL) {
    printf("Memory allocation failed.\n");
    return 1;
  }

  printf("Enter %d integers: ", n);
  for (int i = 0; i < n; i++) {
    if (scanf("%d", &arr[i]) != 1) {
      free(arr); /* Release memory even when input fails. */
      printf("Invalid input.\n");
      return 1;
    }
  }

  long long sum = 0;
  for (int i = 0; i < n; i++) {
    sum += arr[i];
  }

  /* Convert before dividing to preserve the fractional part. */
  double average = (double)sum / n;
  printf("Sum = %lld\n", sum);
  printf("Average = %.2f\n", average);

  free(arr);
  return 0;
}
