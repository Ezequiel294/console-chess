#include "test.h"
#include "tests.h"

#include <string.h>

int g_tests_run = 0;
int g_tests_failed = 0;
int g_test_full = 0;

int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--full") == 0) {
      g_test_full = 1;
    }
  }

  test_fen();
  test_make_unmake();
  test_attacks();
  test_movegen();
  test_special_moves();
  test_perft();
  test_outcome();
  test_notation();

  printf("%d/%d checks passed\n", g_tests_run - g_tests_failed, g_tests_run);
  return g_tests_failed ? 1 : 0;
}
