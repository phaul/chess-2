#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "board.h"
#include "attacks.h"
#include "perft.h"
#include "movelist.h"
#include "movegen.h"
#include "see.h"

int stopped = 0;
unsigned long long int movetime;

static void perft_unit_test1(void **state) {
  BOARD * b = initial_board();

  assert_int_equal(20, perft(b, 1, 0));
}

static void perft_unit_test2(void **state) {
  BOARD * b = initial_board();

  assert_int_equal(400, perft(b, 2, 0));
}

static void perft_unit_test3(void **state) {
  BOARD * b = initial_board();

  assert_int_equal(8902, perft(b, 3, 0));
}

static void perft_unit_test4(void **state) {
  BOARD * b = initial_board();

  assert_int_equal(197281, perft(b, 4, 0));
}

static void perft_unit_test5(void **state) {
  BOARD * b = initial_board();

  assert_int_equal(4865609, perft(b, 5, 0));
}

static void perft_unit_test6(void **state) {
  BOARD * b = initial_board();

  assert_int_equal(119060324, perft(b, 6, 0));
}

static void perft_unit_test_talkchess5(void **state) {
  BOARD * b = parse_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");

  assert_int_equal(89941194, perft(b, 5, 0));
}

static void forcing_moves_count_test(void **state)
{
  BOARD * b = parse_fen("2r3k1/1P2P3/8/8/4p3/3B4/8/6K1 w - - 0 1");
  MOVE * ptr;
  int count = 0;

  ml_open_frame();

  add_moves(b, ALL_MOVES);

  for (ptr = ml_forcing(b); ptr != NULL; ptr = ptr->next) {
    count ++;
  }

  assert_int_equal(8, count);

  ml_close_frame();
}

static void forcing_moves_test(void **state) {
  BOARD * b = parse_fen("2r3k1/1P2P3/8/8/4p3/3B4/8/6K1 w - - 0 1");
  MOVE * ptr;
  const char * moves[] = {
    "d3e4", "e7e8q", "e7e8r", "b7c8n", "b7c8b", "b7c8r", "b7c8q", "d3c4"
  };

  ml_open_frame();

  add_moves(b, ALL_MOVES);

  for (ptr = ml_forcing(b); ptr != NULL; ptr = ptr->next) {
    char buffer[6];
    int match = 0;

    print_move_buffer(ptr, buffer);

    for (int i = 0; i < 8; ++i) {
      if (0 == strcmp(moves[i], buffer)) {
        match = 1;
        break;
      }
    }

    assert_true(match);
  }


  ml_close_frame();
}

static void see_capture_test(void ** state) {
  BOARD * b = parse_fen("3Q4/3q4/1B2N3/5N2/2KPk3/3r4/2n1nb2/3R4 b - - 0 1");
  MOVE c2d4;

  c2d4.from            = (BITBOARD)1 << 10;
  c2d4.to              = (BITBOARD)1 << 27;
  c2d4.special         = 0;
  c2d4.piece           = KNIGHT;
  c2d4.capture         = PAWN;
  c2d4.promotion       = NO_PIECE;
  c2d4.en_passant      = b->en_passant;
  c2d4.castle          = 0;

  assert_int_equal(-225, see(b, &c2d4));
}

static void see_test(void ** state) {
  BOARD * b = parse_fen("7k/2b5/8/8/2N5/1R6/8/7K w - - 0 4");
  MOVE b3b6;

  b3b6.from            = (BITBOARD)1 << 17;
  b3b6.to              = (BITBOARD)1 << 41;
  b3b6.special         = 0;
  b3b6.piece           = ROOK;
  b3b6.capture         = 0;
  b3b6.promotion       = NO_PIECE;
  b3b6.en_passant      = b->en_passant;
  b3b6.castle          = 0;

  assert_int_equal(-100, see(b, &b3b6));
}

int main(void) {
  initialize_magic();

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(perft_unit_test1),
    cmocka_unit_test(perft_unit_test2),
    cmocka_unit_test(perft_unit_test3),
    cmocka_unit_test(perft_unit_test4),
    cmocka_unit_test(perft_unit_test5),
    cmocka_unit_test(perft_unit_test6),
    cmocka_unit_test(perft_unit_test_talkchess5),
    cmocka_unit_test(forcing_moves_count_test),
    cmocka_unit_test(forcing_moves_test),
    cmocka_unit_test(see_capture_test),
    cmocka_unit_test(see_test),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
