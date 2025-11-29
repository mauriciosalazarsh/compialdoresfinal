int multiplicar(int a, int b) {
    return a * b;
}

int cuadrado(int x) {
    return multiplicar(x, x);
}

int main() {
    int n = 7;
    int result = cuadrado(n);
    printf("%d\n", result);
    return 0;
}
