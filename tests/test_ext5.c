// Extension Test 5: Typedef (type aliases)
typedef int entero;
typedef long numero_grande;
typedef float decimal;

int main() {
    entero x = 10;
    entero y = 20;
    entero suma = x + y;
    printf("%d\n", suma);

    numero_grande big = 1000000;
    printf("%ld\n", big);

    decimal pi = 3.14;
    printf("%f\n", pi);

    return 0;
}
