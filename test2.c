int main() {
    int x = 5;
    int y = 10;
    int z = 0;

    // If statement
    if (x < y) {
        z = x + y;
    } else {
        z = x - y;
    }

    // While loop
    while (z > 0) {
        z--;
    }

    // For loop
    for (int i = 0; i < 5; i++) {
        x += i;
    }

    // Nested control flow
    if (x > 10) {
        for (int j = 0; j < 3; j++) {
            while (y < 20) {
                y++;
            }
        }
    }

    return 0;
}
