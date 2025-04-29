/*
 * This test case is designed to generate a φ-function in the control flow graph.
 * The variable `x` is assigned different values in the `if` and `else` branches.
 * A φ-function will be required to merge these values at the join point after the `if-else`.
 */

int main() {
    int x = 0;

    if (x < 5) {
        x = 1;
    } else {
        x = 2;
    }

    return x;
}