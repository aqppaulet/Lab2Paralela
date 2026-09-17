CXX := g++
CXXFLAGS := -O3 -std=c++17 -Wall -Wextra -Wpedantic
TARGET := lab2

.PHONY: all clean experiment cachegrind
all: $(TARGET)

$(TARGET): matrix_benchmark.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

experiment: $(TARGET)
	@mkdir -p results
	@echo 'n,algoritmo,bloque,repeticiones,tiempo_mediana_ms,checksum' > results/matrices.csv
	@for n in 128 256 512 768; do ./$(TARGET) matrix $$n 32 3 | tail -n +2 >> results/matrices.csv; done
	@echo 'n,algoritmo,bloque,repeticiones,tiempo_mediana_ms,checksum' > results/bloques_512.csv
	@for b in 8 16 32 64 128; do ./$(TARGET) matrix 512 $$b 3 | tail -n 2 >> results/bloques_512.csv; done
	@echo 'n,estructura,iteraciones,tiempo_ms' > results/bucles.csv
	@for n in 10000 20000 40000; do ./$(TARGET) loops $$n | tail -n +2 >> results/bucles.csv; done

cachegrind: $(TARGET)
	@mkdir -p results
	valgrind --tool=cachegrind --cachegrind-out-file=results/classic.cg ./$(TARGET) single clasico 256 32
	valgrind --tool=cachegrind --cachegrind-out-file=results/blocked_ijk.cg ./$(TARGET) single bloques_ijk 256 32
	valgrind --tool=cachegrind --cachegrind-out-file=results/blocked_ikj.cg ./$(TARGET) single bloques_ikj 256 32
	@cg_annotate results/classic.cg > results/classic_cachegrind.txt
	@cg_annotate results/blocked_ijk.cg > results/blocked_ijk_cachegrind.txt
	@cg_annotate results/blocked_ikj.cg > results/blocked_ikj_cachegrind.txt

clean:
	rm -f $(TARGET)
