#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>

// don't do this kids
using namespace std;

int count_neighbors(const vector<vector<int>> &board, int i, int j, int N) {
  int count = 0;
  for (int di = -1; di <= 1; ++di) {
    for (int dj = -1; dj <= 1; ++dj) {
      if (di == 0 && dj == 0)
        continue;
      int ni = i + di;
      int nj = j + dj;
      if (ni >= 0 && ni < N && nj >= 0 && nj < N)
        count += board[ni][nj];
    }
  }
  return count;
}

void step(const vector<vector<int>> &current, vector<vector<int>> &next,
          int N) {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      int n = count_neighbors(current, i, j, N);
      if (current[i][j] == 1)
        next[i][j] = (n == 2 || n == 3) ? 1 : 0;
      else
        next[i][j] = (n == 3) ? 1 : 0;
    }
  }
}

void init_board(vector<vector<int>> &board, int N) {
  srand(time(nullptr));
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      board[i][j] = rand() % 2;
}

void print_board(const vector<vector<int>> &board, int N) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++)
      cout << (board[i][j] ? 'O' : '.');
    cout << endl;
  }
  cout << endl;
}

int main(int argc, char *argv[]) {
  int N = 20;
  int iterations = 50;

  if (argc >= 2) {
    stringstream ss(argv[1]);
    ss >> N;
  }
  if (argc >= 3) {
    stringstream ss(argv[2]);
    ss >> iterations;
  }

  vector<vector<int>> current(N, vector<int>(N));
  vector<vector<int>> next(N, vector<int>(N));

  init_board(current, N);

  auto start = chrono::high_resolution_clock::now();

  for (int iter = 0; iter < iterations; iter++) {
    step(current, next, N);
    current.swap(next);
  }

  auto end = chrono::high_resolution_clock::now();

  chrono::duration<double> elapsed = end - start;
  double avg = elapsed.count() / iterations;

  cout << "RESULT type=seq"
       << " N=" << N << " iters=" << iterations << " procs=1"
       << " avg_iter=" << avg << endl;

  return 0;
}
