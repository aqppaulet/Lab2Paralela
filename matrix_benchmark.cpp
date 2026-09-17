// Laboratorio 2: algoritmos, localidad de datos y memoria cache.
// Compilar: g++ -O3 -std=c++17 -march=native matrix_benchmark.cpp -o lab2

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;
using Matrix = std::vector<double>;

volatile double sink = 0.0; // evita que el compilador elimine los calculos.

struct Measurement { double milliseconds; double checksum; };

static Matrix make_matrix(int n, int seed) {
    Matrix m(static_cast<size_t>(n) * n);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            m[static_cast<size_t>(i) * n + j] =
                ((i * 17 + j * 13 + seed) % 101) / 101.0;
    return m;
}

static double checksum(const Matrix& c) {
    double sum = 0.0;
    for (size_t i = 0; i < c.size(); i += std::max<size_t>(1, c.size() / 32))
        sum += c[i];
    return sum;
}

// Version clasica i-j-k: B se recorre por columnas, con poca localidad espacial.
static Measurement classic_multiply(int n) {
    Matrix a = make_matrix(n, 1), b = make_matrix(n, 7), c(static_cast<size_t>(n) * n, 0.0);
    const auto start = Clock::now();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double value = 0.0;
            for (int k = 0; k < n; ++k)
                value += a[static_cast<size_t>(i) * n + k] * b[static_cast<size_t>(k) * n + j];
            c[static_cast<size_t>(i) * n + j] = value;
        }
    }
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    return {ms, checksum(c)};
}

// Primera version bloqueada: ii-jj-kk-i-j-k.
static Measurement blocked_ijk_multiply(int n, int block) {
    Matrix a = make_matrix(n, 1), b = make_matrix(n, 7), c(static_cast<size_t>(n) * n, 0.0);
    const auto start = Clock::now();
    for (int ii = 0; ii < n; ii += block)
        for (int jj = 0; jj < n; jj += block)
            for (int kk = 0; kk < n; kk += block)
                for (int i = ii; i < std::min(ii + block, n); ++i)
                    for (int j = jj; j < std::min(jj + block, n); ++j) {
                        double value = c[static_cast<size_t>(i) * n + j];
                        for (int k = kk; k < std::min(kk + block, n); ++k)
                            value += a[static_cast<size_t>(i) * n + k] * b[static_cast<size_t>(k) * n + j];
                        c[static_cast<size_t>(i) * n + j] = value;
                    }
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    return {ms, checksum(c)};
}

// Segunda version bloqueada: ii-kk-jj-i-k-j.
// B[k*n+j] y C[i*n+j] se recorren por filas dentro de cada bloque.
static Measurement blocked_ikj_multiply(int n, int block) {
    Matrix a = make_matrix(n, 1), b = make_matrix(n, 7), c(static_cast<size_t>(n) * n, 0.0);
    const auto start = Clock::now();
    for (int ii = 0; ii < n; ii += block)
        for (int kk = 0; kk < n; kk += block)
            for (int jj = 0; jj < n; jj += block)
                for (int i = ii; i < std::min(ii + block, n); ++i)
                    for (int k = kk; k < std::min(kk + block, n); ++k) {
                        const double aik = a[static_cast<size_t>(i) * n + k];
                        for (int j = jj; j < std::min(jj + block, n); ++j)
                            c[static_cast<size_t>(i) * n + j] +=
                                aik * b[static_cast<size_t>(k) * n + j];
                    }
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    return {ms, checksum(c)};
}

static void nested_loops(int n) {
    std::uint64_t square_count = 0, triangle_count = 0;
    std::uint64_t square_sum = 0, triangle_sum = 0;
    const auto a = Clock::now();
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) { ++square_count; square_sum += static_cast<unsigned>(i + j); }
    const auto b = Clock::now();
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < i; ++j) { ++triangle_count; triangle_sum += static_cast<unsigned>(i + j); }
    const auto c = Clock::now();
    sink += square_sum + triangle_sum;
    const double square_ms = std::chrono::duration<double, std::milli>(b - a).count();
    const double triangle_ms = std::chrono::duration<double, std::milli>(c - b).count();
    std::cout << "n,estructura,iteraciones,tiempo_ms\n"
              << n << ",cuadrada," << square_count << ',' << square_ms << '\n'
              << n << ",triangular," << triangle_count << ',' << triangle_ms << '\n';
}

enum class Algorithm { Classic, BlockedIJK, BlockedIKJ };

static Measurement run_algorithm(Algorithm algorithm, int n, int block) {
    if (algorithm == Algorithm::Classic) return classic_multiply(n);
    if (algorithm == Algorithm::BlockedIJK) return blocked_ijk_multiply(n, block);
    return blocked_ikj_multiply(n, block);
}

static double median_time(int n, int block, int repetitions, Algorithm algorithm, double& result_checksum) {
    std::vector<double> samples;
    for (int r = 0; r < repetitions; ++r) {
        Measurement m = run_algorithm(algorithm, n, block);
        samples.push_back(m.milliseconds); result_checksum = m.checksum;
    }
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

static void matrix_benchmark(int n, int block, int repetitions) {
    double c1 = 0, c2 = 0, c3 = 0;
    const double classic = median_time(n, block, repetitions, Algorithm::Classic, c1);
    const double blocked_ijk = median_time(n, block, repetitions, Algorithm::BlockedIJK, c2);
    const double blocked_ikj = median_time(n, block, repetitions, Algorithm::BlockedIKJ, c3);
    if (std::abs(c1 - c2) > 1e-8 || std::abs(c1 - c3) > 1e-8)
        throw std::runtime_error("Resultados distintos");
    sink += c1;
    std::cout << std::fixed << std::setprecision(3)
              << "n,algoritmo,bloque,repeticiones,tiempo_mediana_ms,checksum\n"
              << n << ",clasico,0," << repetitions << ',' << classic << ',' << c1 << '\n'
              << n << ",bloques_ijk," << block << ',' << repetitions << ',' << blocked_ijk << ',' << c2 << '\n'
              << n << ",bloques_ikj," << block << ',' << repetitions << ',' << blocked_ikj << ',' << c3 << '\n';
}

static void usage(const char* p) {
    std::cerr << "Uso:\n  " << p << " loops N\n  " << p
              << " matrix N BLOQUE [REPETICIONES]\n  " << p
              << " single clasico|bloques_ijk|bloques_ikj N BLOQUE\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "loops") {
            nested_loops(std::stoi(argv[2]));
        } else if ((argc == 4 || argc == 5) && std::string(argv[1]) == "matrix") {
            const int n = std::stoi(argv[2]), block = std::stoi(argv[3]);
            const int reps = argc == 5 ? std::stoi(argv[4]) : 3;
            if (n <= 0 || block <= 0 || reps <= 0) throw std::runtime_error("Parametros positivos requeridos");
            matrix_benchmark(n, block, reps);
        } else if (argc == 5 && std::string(argv[1]) == "single") {
            const std::string algorithm = argv[2];
            const int n = std::stoi(argv[3]), block = std::stoi(argv[4]);
            Algorithm selected;
            if (algorithm == "clasico") selected = Algorithm::Classic;
            else if (algorithm == "bloques_ijk" || algorithm == "bloques") selected = Algorithm::BlockedIJK;
            else if (algorithm == "bloques_ikj") selected = Algorithm::BlockedIKJ;
            else throw std::runtime_error("Uso: single clasico|bloques_ijk|bloques_ikj N BLOQUE");
            if (n <= 0 || block <= 0) throw std::runtime_error("Parametros positivos requeridos");
            const Measurement m = run_algorithm(selected, n, block);
            sink += m.checksum;
            std::cout << std::fixed << std::setprecision(3) << "algoritmo,n,bloque,tiempo_ms,checksum\n"
                      << algorithm << ',' << n << ',' << block << ',' << m.milliseconds << ',' << m.checksum << '\n';
        } else { usage(argv[0]); return 1; }
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
