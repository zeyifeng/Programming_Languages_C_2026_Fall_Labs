#include <stdio.h>

long long factorial(int n) {
  long long result = 1;
  for (int i = 1; i <= n; i++) {
    result = result * i;
  }
  return result;
}

int main() {
  int n;
  printf("Enter n: ");
  scanf("%d", &n);
  if (n < 0) {
    printf("Error: n cannot be negative\n");
  } else {
    printf("%d! = %lld\n", n, factorial(n));
  }
  return 0;
}