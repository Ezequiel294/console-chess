#ifndef TEST_H
#define TEST_H

#include <stdio.h>

/* A minimal check-and-count harness. Deliberately not an assert: one failure
 * should not stop the run, since the whole point of the suite is to see every
 * failure in one pass rather than bisecting one crash at a time. */

extern int g_tests_run;
extern int g_tests_failed;

/* Set from main() when invoked with --full. test_perft() reads this to decide
 * whether to run the depth-5-and-above fixtures make test-full asks for, on
 * top of the depth-4 default every `make test` run checks. */
extern int g_test_full;

#define TEST_CHECK(cond)                                                     \
  do {                                                                       \
    g_tests_run++;                                                          \
    if (!(cond)) {                                                           \
      g_tests_failed++;                                                     \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
    }                                                                        \
  } while (0)

#define TEST_CHECK_MSG(cond, ...)                                            \
  do {                                                                       \
    g_tests_run++;                                                          \
    if (!(cond)) {                                                           \
      g_tests_failed++;                                                     \
      fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);                   \
      fprintf(stderr, __VA_ARGS__);                                          \
      fprintf(stderr, "\n");                                                 \
    }                                                                        \
  } while (0)

#endif /* TEST_H */
