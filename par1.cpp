#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <sstream>
#include <vector>

// don't do this kids
using namespace std;

int count_neighbors(const vector<vector<int>> &board, int i, int j,
                    int local_rows, int N) {
  int count = 0;
  for (int di = -1; di <= 1; ++di) {
    for (int dj = -1; dj <= 1; ++dj) {
      if (di == 0 && dj == 0)
        continue;

      int ni = i + di;
      int nj = j + dj;

      if (ni >= 0 && ni < local_rows + 2 && nj >= 0 && nj < N) {
        count += board[ni][nj];
      }
    }
  }
  return count;
}

void step(const vector<vector<int>> &current, vector<vector<int>> &next,
          int local_rows, int N) {

  for (int i = 1; i <= local_rows; ++i) {
    for (int j = 0; j < N; ++j) {
      int n = count_neighbors(current, i, j, local_rows, N);
      if (current[i][j] == 1)
        next[i][j] = (n == 2 || n == 3) ? 1 : 0;
      else
        next[i][j] = (n == 3) ? 1 : 0;
    }
  }
}

void init_board(vector<vector<int>> &board, int lr, int N, int rank) {
  srand(0 + rank);
  for (int i = 1; i <= lr; ++i)
    for (int j = 0; j < N; ++j)
      board[i][j] = rand() % 2;
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

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

  int base = N / size;
  int rem = N % size;
  int row_count = base + (rank < rem ? 1 : 0);

  vector<vector<int>> current(row_count + 2, vector<int>(N));
  vector<vector<int>> next(row_count + 2, vector<int>(N));

  init_board(current, row_count, N, rank);

  int up = (rank == 0) ? MPI_PROC_NULL : rank - 1;
  int down = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

  MPI_Barrier(MPI_COMM_WORLD);
  double start = MPI_Wtime();

  for (int i = 0; i < iterations; ++i) {

    MPI_Sendrecv(current[1].data(), N, MPI_INT, up, 0,
                 current[row_count + 1].data(), N, MPI_INT, down, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Sendrecv(current[row_count].data(), N, MPI_INT, down, 1,
                 current[0].data(), N, MPI_INT, up, 1, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

    step(current, next, row_count, N);

    current.swap(next);
  }

  double end = MPI_Wtime();

  if (rank == 0) {
    double avg = (end - start) / iterations;
    cout << "RESULT type=sendrecv"
         << " N=" << N << " iters=" << iterations << " procs=" << size
         << " avg_iter=" << avg << endl;
  }

  MPI_Finalize();
  return 0;
}
