#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

// don't do this kids
using namespace std;

void write_pgm_mpi(const string &filename, const vector<vector<int>> &board,
                   int local_rows, int N, int rank, int size) {

  MPI_File fh;

  string header;
  if (rank == 0) {
    ostringstream oss;
    oss << "P5\n" << N << " " << N << "\n255\n";
    header = oss.str();
  }

  int header_size = header.size();
  MPI_Bcast(&header_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

  MPI_File_open(MPI_COMM_WORLD, filename.c_str(),
                MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);

  MPI_File_set_size(fh, 0);

  if (rank == 0) {
    MPI_File_write_at(fh, 0, header.c_str(), header_size, MPI_CHAR,
                      MPI_STATUS_IGNORE);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  int rows_before = 0;
  MPI_Exscan(&local_rows, &rows_before, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
  if (rank == 0)
    rows_before = 0;

  MPI_Offset offset = header_size + static_cast<MPI_Offset>(rows_before) * N;

  vector<unsigned char> buffer(local_rows * N);

  for (int i = 0; i < local_rows; ++i) {
    for (int j = 0; j < N; ++j) {
      buffer[i * N + j] = board[i + 1][j] ? 255 : 0;
    }
  }

  MPI_File_write_at_all(fh, offset, buffer.data(), local_rows * N,
                        MPI_UNSIGNED_CHAR, MPI_STATUS_IGNORE);

  MPI_File_close(&fh);
}

int count_neighbors(const vector<vector<int>> &board, int i, int j,
                    int local_rows, int N) {
  int count = 0;
  for (int di = -1; di <= 1; ++di) {
    for (int dj = -1; dj <= 1; ++dj) {
      if (di == 0 && dj == 0)
        continue;
      int ni = i + di;
      int nj = j + dj;
      if (ni >= 0 && ni < local_rows + 2 && nj >= 0 && nj < N)
        count += board[ni][nj];
    }
  }
  return count;
}

void step_inner(const vector<vector<int>> &current, vector<vector<int>> &next,
                int local_rows, int N) {
  for (int i = 2; i <= local_rows - 1; ++i) {
    for (int j = 0; j < N; ++j) {
      int n = count_neighbors(current, i, j, local_rows, N);
      if (current[i][j])
        next[i][j] = (n == 2 || n == 3);
      else
        next[i][j] = (n == 3);
    }
  }
}

void step_border(const vector<vector<int>> &current, vector<vector<int>> &next,
                 int local_rows, int N) {
  int rows[2] = {1, local_rows};
  for (int r = 0; r < 2; ++r) {
    int i = rows[r];
    for (int j = 0; j < N; ++j) {
      int n = count_neighbors(current, i, j, local_rows, N);
      if (current[i][j])
        next[i][j] = (n == 2 || n == 3);
      else
        next[i][j] = (n == 3);
    }
  }
}

void init_board(vector<vector<int>> &board, int lr, int N, int rank) {
  srand(0 + rank);
  for (int i = 1; i <= lr; ++i)
    for (int j = 0; j < N; ++j)
      board[i][j] = rand() % 2;
}

string get_frame_filename(int iter) {
  ostringstream ss;
  ss << "frames/frame_" << setw(4) << setfill('0') << iter << ".pgm";
  return ss.str();
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int N = 100;
  int iterations = 500;

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

  vector<vector<int>> current(row_count + 2, vector<int>(N, 0));
  vector<vector<int>> next(row_count + 2, vector<int>(N, 0));

  init_board(current, row_count, N, rank);

  int up = (rank == 0) ? MPI_PROC_NULL : rank - 1;
  int down = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

#ifdef SF
  if (rank == 0) {
    mkdir("frames", 0777);
  }
  MPI_Barrier(MPI_COMM_WORLD); // czekamy aż folder będzie gotowy
#endif

  MPI_Barrier(MPI_COMM_WORLD);
  double start = MPI_Wtime();

  for (int i = 0; i < iterations; ++i) {
    MPI_Request reqs[2], reqr[2];

    MPI_Irecv(current[0].data(), N, MPI_INT, up, 1, MPI_COMM_WORLD, &reqr[0]);
    MPI_Irecv(current[row_count + 1].data(), N, MPI_INT, down, 0,
              MPI_COMM_WORLD, &reqr[1]);

    MPI_Isend(current[1].data(), N, MPI_INT, up, 0, MPI_COMM_WORLD, &reqs[0]);
    MPI_Isend(current[row_count].data(), N, MPI_INT, down, 1, MPI_COMM_WORLD,
              &reqs[1]);

    if (row_count > 2)
      step_inner(current, next, row_count, N);

    MPI_Waitall(2, reqr, MPI_STATUSES_IGNORE);

    step_border(current, next, row_count, N);

    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

    current.swap(next);

#ifdef SF
    string fname = get_frame_filename(i);
    write_pgm_mpi(fname, current, row_count, N, rank, size);
#endif
  }

  double end = MPI_Wtime();

#ifdef SF
  string fname = "frames/final.pgm";
  write_pgm_mpi(fname, current, row_count, N, rank, size);
#endif

  if (rank == 0) {
    double avg = (end - start) / iterations;
    cout << "RESULT type=async"
         << " N=" << N << " iters=" << iterations << " procs=" << size
         << " avg_iter=" << avg << endl;
  }

  MPI_Finalize();
  return 0;
}
