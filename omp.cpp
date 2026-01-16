#include <cstdlib>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <vector>

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

#pragma omp parallel for collapse(2)
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
  srand(0);
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      board[i][j] = rand() % 2;
}

int main(int argc, char *argv[]) {
  int N = 20;
  int t = 12;
  int iterations = 50;

  if (argc >= 2) {
    stringstream ss(argv[1]);
    ss >> N;
  }
  if (argc >= 3) {
    stringstream ss(argv[2]);
    ss >> iterations;
  }

  if (argc >= 4) {
    stringstream ss(argv[3]);
    ss >> t;
    if (t > 0)
      omp_set_num_threads(t);
  }

  vector<vector<int>> current(N, vector<int>(N));
  vector<vector<int>> next(N, vector<int>(N));

  init_board(current, N);

  double start = omp_get_wtime();

  for (int iter = 0; iter < iterations; iter++) {
    step(current, next, N);
    current.swap(next);
  }

  double end = omp_get_wtime();

  cout << "Avg iteration time: " << (end - start) / iterations << " s\n";

  return 0;
}
