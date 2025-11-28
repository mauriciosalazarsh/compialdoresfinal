// Base Test 3: While loops
int main() {
    int i = 0;
    int sum = 0;

    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }

    printf("%d\n", sum);
    return 0;
}
