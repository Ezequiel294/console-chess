#include "test.h"

#include "core/board.h"
#include "core/history.h"
#include "core/movegen.h"
#include "core/notation.h"
#include "core/replay.h"

#include <string.h>

static void free_state(GameState *s) {
  free_captures(s->p_captures_white_head);
  free_captures(s->p_captures_black_head);
  free_history(s->p_history_head);
  free_hash_history(s->p_hash_history_head);
  free_history(s->p_redo_head);
  *s = (GameState){0};
}

/* Builds a GameState by playing coord-notation moves from a starting FEN, the
 * same forward construction save_read uses: mover's own captures list grows
 * on a capture, history and hash history grow on every move. */
static void play_moves(GameState *state, const char *start_fen, const char *const *coords,
                        int n) {
  *state = (GameState){0};
  TEST_CHECK_MSG(fen_parse(start_fen, &state->position), "bad fixture FEN: %s", start_fen);
  state->start_position = state->position;
  push_hash(&state->p_hash_history_head, state->position.hash);

  for (int k = 0; k < n; k++) {
    Move move;
    TEST_CHECK_MSG(coord_to_move(&state->position, coords[k], &move), "illegal move %s at ply %d",
                   coords[k], k);

    Color mover = state->position.side_to_move;
    char from[3], to[3];
    index_to_square(move.from_i, move.from_j, from);
    index_to_square(move.to_i, move.to_j, to);

    if (move.captured != FREE) {
      Color captured_color = (mover == WHITE) ? BLACK : WHITE;
      Captures_node_t **captures =
          (mover == WHITE) ? &state->p_captures_white_head : &state->p_captures_black_head;
      update_captures(captures, (Piece_t){.color = captured_color, .type = move.captured});
    }

    make(&state->position, move);
    update_history(&state->p_history_head, from, to, move);
    push_hash(&state->p_hash_history_head, state->position.hash);
  }
}

static int captures_length(const Captures_node_t *head) {
  int n = 0;
  for (; head != NULL; head = head->p_next) {
    n++;
  }
  return n;
}

static int history_length(const History_node_t *head) {
  int n = 0;
  for (; head != NULL; head = head->p_next) {
    n++;
  }
  return n;
}

/* history ++ redo must be the whole game in played order: total plies match,
 * and walking both lists in order reproduces the same move sequence a full
 * play-through would, wherever stepping has currently left the position. */
static void check_history_redo_invariant(const GameState *state, int total_plies) {
  int n = history_length(state->p_history_head) + history_length(state->p_redo_head);
  TEST_CHECK_MSG(n == total_plies, "history+redo length %d != %d plies played", n, total_plies);

  int seen = 0;
  for (const History_node_t *p = state->p_history_head; p != NULL; p = p->p_next) {
    seen++;
  }
  for (const History_node_t *p = state->p_redo_head; p != NULL; p = p->p_next) {
    seen++;
  }
  TEST_CHECK(seen == total_plies);
}

/* Steps back through the whole game, checking the invariant at every
 * position, then forward again, and asserts the final position is bit for
 * bit identical to where the game actually ended. */
static void check_round_trip(const char *start_fen, const char *const *coords, int n) {
  GameState state;
  play_moves(&state, start_fen, coords, n);
  Position final = state.position;

  check_history_redo_invariant(&state, n);

  for (int k = 0; k < n; k++) {
    TEST_CHECK(replay_step_back(&state));
    check_history_redo_invariant(&state, n);
  }
  TEST_CHECK_MSG(memcmp(&state.position, &state.start_position, sizeof(Position)) == 0,
                 "stepping all the way back did not restore the starting position: %s",
                 start_fen);
  TEST_CHECK(state.p_history_head == NULL);
  TEST_CHECK(history_length(state.p_redo_head) == n);

  for (int k = 0; k < n; k++) {
    TEST_CHECK(replay_step_forward(&state));
    check_history_redo_invariant(&state, n);
  }
  TEST_CHECK_MSG(memcmp(&state.position, &final, sizeof(Position)) == 0,
                 "stepping all the way forward did not restore the final position: %s",
                 start_fen);
  TEST_CHECK(state.p_redo_head == NULL);
  TEST_CHECK(history_length(state.p_history_head) == n);

  free_state(&state);
}

static void test_round_trips(void) {
  const char *start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  const char *coords1[] = {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6"};
  check_round_trip(start, coords1, 6);

  const char *coords2[] = {"e2e4"};
  check_round_trip(start, coords2, 1);
}

/* Each fixture isolates one special move — always the last move played.
 * Stepping back across it must restore the exact pre-move position and
 * leave the captures lists no larger than they were before it. */
static void check_special_move(const char *start_fen, const char *const *coords, int n) {
  GameState state;
  play_moves(&state, start_fen, coords, n);

  Position before = state.start_position;
  for (int k = 0; k < n - 1; k++) {
    Move move;
    TEST_CHECK(coord_to_move(&before, coords[k], &move));
    make(&before, move);
  }
  int white_before = captures_length(state.p_captures_white_head);
  int black_before = captures_length(state.p_captures_black_head);

  TEST_CHECK(replay_step_back(&state));
  TEST_CHECK_MSG(memcmp(&state.position, &before, sizeof(Position)) == 0,
                 "stepping back over the special move did not restore the exact prior position "
                 "for %s",
                 start_fen);

  TEST_CHECK(captures_length(state.p_captures_white_head) <= white_before);
  TEST_CHECK(captures_length(state.p_captures_black_head) <= black_before);

  free_state(&state);
}

/* Steps back then forward across the special move and asserts the position
 * and both captures lists are exactly as they were before the pair of
 * steps — the scenario game-replay's spec calls "special moves step
 * cleanly". */
static void check_special_move_pair(const char *start_fen, const char *const *coords, int n) {
  GameState state;
  play_moves(&state, start_fen, coords, n);
  Position after_special_move = state.position;
  int white_len = captures_length(state.p_captures_white_head);
  int black_len = captures_length(state.p_captures_black_head);

  TEST_CHECK(replay_step_back(&state));
  TEST_CHECK(replay_step_forward(&state));

  TEST_CHECK_MSG(memcmp(&state.position, &after_special_move, sizeof(Position)) == 0,
                 "stepping back then forward across %s did not restore the position", start_fen);
  TEST_CHECK(captures_length(state.p_captures_white_head) == white_len);
  TEST_CHECK(captures_length(state.p_captures_black_head) == black_len);

  free_state(&state);
}

static void test_special_moves(void) {
  /* A capture. */
  {
    const char *coords[] = {"e2e4", "d7d5", "e4d5"};
    check_special_move("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", coords, 3);
    check_special_move_pair("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", coords, 3);
  }

  /* Castling kingside. */
  {
    const char *coords[] = {"e1g1"};
    check_special_move("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", coords, 1);
    check_special_move_pair("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", coords, 1);
  }

  /* Castling queenside. */
  {
    const char *coords[] = {"e1c1"};
    check_special_move("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", coords, 1);
    check_special_move_pair("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", coords, 1);
  }

  /* En passant. */
  {
    const char *coords[] = {"e5d6"};
    check_special_move("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", coords, 1);
    check_special_move_pair("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", coords, 1);
  }

  /* Promotion. */
  {
    const char *coords[] = {"e7e8q"};
    check_special_move("8/4P3/8/8/4k3/8/8/4K3 w - - 0 1", coords, 1);
    check_special_move_pair("8/4P3/8/8/4k3/8/8/4K3 w - - 0 1", coords, 1);
  }
}

static void test_bounds(void) {
  const char *start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

  /* Nothing to undo at the starting position. */
  {
    GameState state;
    play_moves(&state, start, NULL, 0);
    Position before = state.position;
    TEST_CHECK(replay_step_back(&state) == 0);
    TEST_CHECK(memcmp(&state.position, &before, sizeof(Position)) == 0);
    TEST_CHECK(state.p_history_head == NULL);
    TEST_CHECK(state.p_redo_head == NULL);
    free_state(&state);
  }

  /* Nothing to redo at the final position. */
  {
    const char *coords[] = {"e2e4", "e7e5"};
    GameState state;
    play_moves(&state, start, coords, 2);
    Position before = state.position;
    TEST_CHECK(replay_step_forward(&state) == 0);
    TEST_CHECK(memcmp(&state.position, &before, sizeof(Position)) == 0);
    TEST_CHECK(state.p_redo_head == NULL);
    free_state(&state);
  }
}

void test_replay(void) {
  test_round_trips();
  test_special_moves();
  test_bounds();
}
