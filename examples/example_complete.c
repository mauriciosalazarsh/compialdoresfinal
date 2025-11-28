// Complete example in C demonstrating all compiler features

// Function: Calculate sum of two integers
int suma(int a, int b) {
    return a + b;
}

// Function: Calculate factorial recursively
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Function: Find maximum of three numbers using ternary operator
int maximo(int a, int b, int c) {
    int max_ab = (a > b) ? a : b;
    return (max_ab > c) ? max_ab : c;
}

// Main function
int main() {
    // Variable declarations
    int x = 10;
    int y = 20;
    int z = 15;

    // Arithmetic operations
    int sum = x + y;
    int diff = y - x;
    int prod = x * y;
    int quot = y / x;

    printf("%d\n", sum);
    printf("%d\n", diff);
    printf("%d\n", prod);
    printf("%d\n", quot);

    // If-else statement
    if (x > y) {
        printf("%d\n", x);
    } else {
        printf("%d\n", y);
    }

    // While loop
    int i = 0;
    int total = 0;
    while (i < 5) {
        total = total + i;
        i = i + 1;
    }
    printf("%d\n", total);

    // For loop
    int sum2 = 0;
    for (int j = 0; j < 10; j++) {
        sum2 = sum2 + j;
    }
    printf("%d\n", sum2);

    // Function calls
    int result = suma(x, y);
    printf("%d\n", result);

    int fact = factorial(5);
    printf("%d\n", fact);

    int max = maximo(x, y, z);
    printf("%d\n", max);

    // Ternary expression
    int min = (x < y) ? x : y;
    printf("%d\n", min);

    return 0;
}
