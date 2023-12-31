﻿#include <iostream> 
#include <fstream> 
#include <chrono>
#include <mpi.h> 
#include <iomanip> 
/*
* Лабораторная работа #2
* "Суперкомпьютерные вычисления"
* Вариант №1. Реализовать блочный алгоритм распределенного параллельного
* перемножения матриц A и B с размерами (8 * 5) и (5 * 3) соответственно.
* Работу выполнил Быков Егор ИДМ-22-01
*/
using namespace std;

int len_for_node(int size, int all) {
    if (size == 1) {
        return all;
    }
    if (!(all % size)) {


        return all / size;
    }
    return 0;
}

void master() {
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size == 1) {
        cout << "Нельзя запускать без дочерних процессов, добавьте потоки" << endl;
        return;
    }

    int n = 8;
    int k = 5;
    int m = 3;

    int* init_data = new int[4];
    init_data[0] = n;
    init_data[1] = k;
    init_data[2] = m;

    int len = len_for_node(size - 1, n);
    init_data[3] = len;
    if (!len) {
        cout << "Неправильное количество процессор, должно быть N - где N  - количество строк в матрице А + 1 для основного процесса (master)" << endl;
        exit(69);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(init_data, 4, MPI_INTEGER, 0, MPI_COMM_WORLD);
    delete[] init_data;

     int* a = new int[n * k];
     int* b = new int[k * m];

    //Ввод и вывод исходных матриц на экран
    for (int i = 0; i < n * k; i++) {
        a[i] = rand() % (6)+5;
        if (i % k == 0)
            cout << endl;
        cout << a[i] << " ";
    }
    cout << endl;
    for (int i = 0; i < k * m; i++) {
        b[i] = rand() % (6) + 5;
        if (i % m == 0)
            cout << endl;
        cout << b[i] << " ";
    }
    cout << endl << endl;

    int* empty = new int[len * k];
    int* buf = new int[(len + n) * k];
    memcpy(buf + (len * k), a, n * k * sizeof(int));

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Scatter(buf, len * k, MPI_INTEGER, empty, len * k, MPI_INTEGER, 0, MPI_COMM_WORLD);
    //очищения памяти для массивов
    delete[] empty;
    delete[] buf;
    delete[] a;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(b, k * m, MPI_INTEGER, 0, MPI_COMM_WORLD);
    delete[] b;

    int* c = new int[(len + n) * m];
    empty = new int[len * m];
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(empty, len * m, MPI_INTEGER, c, len * m, MPI_INTEGER, 0, MPI_COMM_WORLD);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            cout << setw(5) << c[len * m + i * m + j] << " ";
        }
        cout << endl;
    }
    delete[] c;
}

void slave() {
    int* buf = new int[4];

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(buf, 4, MPI_INTEGER, 0, MPI_COMM_WORLD);
    int k = buf[1],
        m = buf[2],
        len = buf[3];
    delete[] buf;

    int empty[1];
    int* a = new int[len * k];

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Scatter(empty, 0, MPI_INTEGER, a, len * k, MPI_INTEGER, 0, MPI_COMM_WORLD);

    int* b = new int[k * m];

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(b, k * m, MPI_INTEGER, 0, MPI_COMM_WORLD);

    int* c = new int[len * m];
    for (int i = 0; i < len; i++) {
        for (int j = 0; j < m; j++) {
            c[i * m + j] = 0;
            for (int y = 0; y < k; y++) {
                c[i * m + j] += a[i * k + y] * b[y * m + j];
            }
        }
    }
    //очищения памяти для массивов a и b
    delete[] a;
    delete[] b;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(c, len * m, MPI_INTEGER, empty, 0, MPI_INTEGER, 0, MPI_COMM_WORLD);
    //очищения памяти для массива c
    delete[] c;
}
// старт работы программы
int main(int argc, char** argv) {
    auto start = std::chrono::high_resolution_clock::now();
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // в зависимости какой номер процесса (основной процесс или вспомогательный) вызывается соотвествующая номеру процесса функция
    rank ? slave() : master();

    MPI_Finalize();
    auto finish = std::chrono::high_resolution_clock::now();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);
    std::cout << "\n" << microseconds.count() << "us\n";
    return 0;
}