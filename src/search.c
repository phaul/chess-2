#include "search.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <limits.h>

#include "attacks.h"
#include "evaluate.h"
#include "movegen.h"
#include "moveexec.h"
#include "movelist.h"
#include "chess.h"

static struct timespec start;

#define STDIN 0
int check_for_input() {
  struct timespec timeout = { 0, 0 };
  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(STDIN, &fds);

  pselect(1, &fds, NULL, NULL, & timeout, NULL);

  if (FD_ISSET(STDIN, &fds)) {
    char * line = NULL;
    size_t count;

    count = getline(&line, &count, stdin);

    if (strncmp(("stop"), line, strlen("stop")) == 0) {

      free(line);

      return 1;
    }

    free(line);
  }

  return 0;
}

unsigned long long int time_delta() {
  struct timespec end;
  unsigned long long int delta;

  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end)) {
    printf("info clock_gettime failed\n");
  }

  delta = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;

  return delta;
}

extern int stopped;
extern unsigned long long movetime;

int quiesce(BOARD * board, int alpha, int beta) {
  MOVE * ptr;
  int legal_found = 0;
  int stand_pat;

  ml_open_frame();

  add_moves(board, ALL_MOVES);

  for (ptr = ml_first(); ptr != NULL; ptr = ptr->next) {
    execute_move(board, ptr);

    if (! in_check(board, 1 - board->next)) {
      undo_move(board, ptr);
      legal_found = 1;
      break;
    }
    undo_move(board, ptr);
  }

  if (!legal_found) {
    ml_close_frame();

    if (in_check(board, board->next))
      return -10000;
    else
      return 0;
  }

  stand_pat = evaluate(board);

  if (stand_pat >= beta) {
    ml_close_frame();

    return beta;
  }

  if (alpha < stand_pat)
    alpha = stand_pat;

  for (ptr = ml_forcing(board); ptr != NULL; ptr = ptr->next) {
    int score;

    execute_move(board, ptr);

    if (in_check(board, 1 - board->next)) {
      undo_move(board, ptr);
      continue;
    }

    score = -quiesce(board, -beta, -alpha);

    undo_move(board, ptr);

    if (score >= beta) {
      ml_close_frame();
      return beta;
    }

    if (score > alpha)
      alpha = score;
  }

  ml_close_frame();
  return alpha;
}

/* * 100 */
static const int log[] = {
  -INT_MAX,
  0   ,  69  ,  109 ,  138 ,  160 ,  179 ,  194 ,  207 ,  219 ,  230,
  239 ,  248 ,  256 ,  263 ,  270 ,  277 ,  283 ,  289 ,  294 ,  299,
  304 ,  309 ,  313 ,  317 ,  321 ,  325 ,  329 ,  333 ,  336 ,  340,
  343 ,  346 ,  349 ,  352 ,  355 ,  358 ,  361 ,  363 ,  366 ,  368,
  371 ,  373 ,  376 ,  378 ,  380 ,  382 ,  385 ,  387 ,  389 ,  391,
  393 ,  395 ,  397 ,  398 ,  400 ,  402 ,  404 ,  406 ,  407 ,  409
};

static int lmr(int depth, int count) {
  int value = (log[depth] * log[count] / 19500);

  return value > depth - 1 ? 0 : depth - 1 - value;
}

int negascout(BOARD* board, int depth, int reduced_depth, int alpha, int beta, MOVE * pv, MOVE * npv, KILLER * killer) {
  MOVE lpv[30];
  int legal_found = 0;
  int score;
  int beta2;
  unsigned long long delta;
  int count;

  if (stopped) {
    return -1;
  }
  else {
    if (check_for_input()) {
      stopped = 1;
      return -1;
    }

    if (movetime) {
      delta = time_delta();
      if (movetime < delta) {
        stopped = 1;
        return -1;
      }
    }
  }

  if (reduced_depth == 0) {
    return quiesce(board, alpha, beta);
    /* return colour * evaluate(board); */
  }

  ml_open_frame();

  add_moves(board, ALL_MOVES);

  beta2 = beta;

  count = 1;

  for (MOVE * ptr = ml_sort(board, depth > 1 ? pv : NULL, depth, killer); ptr != NULL; ptr = ptr->next) {

    execute_move(board, ptr);

    if (in_check(board, 1 - board->next)) {
      undo_move(board, ptr);
      continue;
    }

    score = -negascout(board, depth - 1, lmr(reduced_depth, count), -beta2, -alpha, pv + 1, lpv, killer);

    if (alpha < score && score < beta && legal_found) {
      score = -negascout(board, depth - 1, lmr(reduced_depth, count), -beta, -alpha, pv + 1, lpv, killer);
    }

    undo_move(board, ptr);

    if (alpha < score || (alpha == score && !legal_found)) {
      if (depth > 1) {
        memcpy(npv + 1, lpv, sizeof(MOVE) * (depth - 1));
      }
      *npv = *ptr;
      alpha = score;
    }

    legal_found = 1;

    if (alpha >= beta) {
      save_killer(killer, depth, ptr);

      ml_close_frame();

      return alpha;
    }

    beta2 = alpha + 1;

    count++;
  }

  ml_close_frame();

  if (!legal_found) {
    if (in_check(board, board->next))
      return (board->next == WHITE ? -10000 : 10000);
    else
      return 0;
  }

  return alpha;
}

int iterative_deepening(BOARD * board, int max_depth) {
  MOVE pvm[2][80];
  KILLER killer;
  int score = 0;
  MOVE * bestmove = NULL;
  unsigned long long int delta;

  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start)) {
    printf("info clock_gettime failed\n");
  }

  for (int depth = 1; depth <= max_depth; ++depth) {
    int pvm_bank = depth & 1;

    stopped = 0;

    reset_killer(&killer);

    printf("info depth %d\n", depth);

    score = negascout(board, depth, depth , -10000, 10000, pvm[1 - pvm_bank], pvm[pvm_bank], &killer);

    /* if (board->next == BLACK) { */
    /*   score *= -1; */
    /* } */

    if (stopped) break;

    delta = time_delta();

    printf("info score cp %d depth %d time %llu pv ", score, depth, delta);

    for (int i = 0; i < depth; ++i) {
      print_move(&pvm[pvm_bank][i]);
      printf(" ");
    }
    printf("\n");

    fflush(stdout);

    bestmove = pvm[pvm_bank];

    if (score <= -10000 || score >= 10000) break;
  }

  if (bestmove) {
    printf("bestmove ");
    print_move(bestmove);
    printf("\n");
  }

  return score;
}

