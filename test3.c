int main() {
  int x = 0;
  int y = 1;

  // Block A
  if (x < 5) {
    // Block B
    x++;
  }
  // Block C
  y++;

  // Block D
  printf("%d %d\n", x, y);
  return 0;
}
