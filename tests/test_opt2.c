// Optimization Test 2: Dead code elimination
int main() {
    int x = 10;
    int y = 20;
    int z = x + y;

    if (0) {
        z = 100;
    }

    printf("%d\n", z);
    return 0;
}
