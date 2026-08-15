#ifndef TESTS_H
#define TESTS_H

/* One entry point per area, called from main.c. Declared here rather than in
 * test.h so the list of suites is visible in one place. */

void test_fen(void);
void test_make_unmake(void);
void test_attacks(void);
void test_movegen(void);
void test_special_moves(void);
void test_perft(void);
void test_outcome(void);
void test_notation(void);

#endif /* TESTS_H */
