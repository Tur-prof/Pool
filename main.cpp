#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>

#include "optimized_thread.h"

template <typename It>
void PrintRange(It range_begin, It range_end) {
    for (auto it = range_begin; it != range_end; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}
RequestHandler rh;
static bool use_pool = false;
static bool use_multithreading = false;
constexpr int max_size_to_thread = 10000;
using It = std::vector<int>::iterator;

template <typename RandomIt>
void MergeSort(RandomIt range_begin, RandomIt range_end) {
    // 1. Если диапазон содержит меньше 2 элементов, выходим из функции
    auto range_length = range_end - range_begin;
    if (range_length < 2) {
        return;
    }

    // 2. Создаём вектор, содержащий все элементы текущего диапазона
    std::vector elements(range_begin, range_end);
    // Тип элементов — typename iterator_traits<RandomIt>::value_type

    // 3. Разбиваем вектор на две равные части
    auto mid = elements.begin() + range_length / 2;

    if (use_multithreading && range_length > max_size_to_thread)
    {
        auto future1 = std::async(std::launch::async, [&]() {MergeSort(elements.begin(), mid); });
        auto future2 = std::async(std::launch::async, [&]() {MergeSort(mid, elements.end()); });
        
        future1.get();
        future2.get();
    }
    else if (use_pool && range_length > max_size_to_thread)
    {
        res_type result = rh.push_request(MergeSort, elements.begin(), mid);
        result.wait();
        MergeSort(mid, elements.end());
    }
    else
    {
        MergeSort(elements.begin(), mid);
        MergeSort(mid, elements.end());
    }
    // 5. С помощью алгоритма merge сливаем отсортированные половины
    // в исходный диапазон
    std::merge(elements.begin(), mid, mid, elements.end(), range_begin);
}

int main() {

    std::mt19937 generator;

    std::vector<int> test_vector(50000);

    // Заполняет диапазон последовательно возрастающими значениями
    std::iota(test_vector.begin(), test_vector.end(), 1);

    // Перемешивает элементы в случайном порядке
    std::shuffle(test_vector.begin(), test_vector.end(), generator);

    // Выводим вектор до сортировки
    //PrintRange(test_vector.begin(), test_vector.end());

    // Сортируем вектор с помощью сортировки слиянием
     MergeSort(test_vector.begin(), test_vector.end());

    // Выводим результат
    //PrintRange(test_vector.begin(), test_vector.end());

    std::vector<int> copy_vector{ test_vector };
    std::vector<int> copy_vector2{ test_vector };
    //PrintRange(copy_vector2.begin(), copy_vector2.end());

    std::cout << "Sequenced:" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    MergeSort(copy_vector.begin(), copy_vector.end());
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "The time of merge sort: " << std::chrono::duration_cast<std::chrono::milliseconds> (stop - start).count() << " ms" << std::endl;
    //PrintRange(copy_vector.begin(), copy_vector.end());

    std::cout << "Async:" << std::endl;
    use_multithreading = true;
    start = std::chrono::high_resolution_clock::now();
    MergeSort(test_vector.begin(), test_vector.end());
    stop = std::chrono::high_resolution_clock::now();
    std::cout << "The time of merge sort: " << std::chrono::duration_cast<std::chrono::milliseconds> (stop - start).count() << " ms" << std::endl;
    //PrintRange(test_vector.begin(), test_vector.end());

    assert(copy_vector == test_vector);
    std::cout << "Vectors are sorted correctly!" << std::endl;
    use_multithreading = false;

    std::cout << "Pool:" << std::endl;
    use_pool = true;
    start = std::chrono::high_resolution_clock::now();
    MergeSort(copy_vector2.begin(), copy_vector2.end());
    stop = std::chrono::high_resolution_clock::now();
    std::cout << "The time of merge sort with pool: " << std::chrono::duration_cast<std::chrono::milliseconds> (stop - start).count() << " ms" << std::endl;
    //PrintRange(copy_vector2.begin(), copy_vector2.end());

    assert(copy_vector2 == test_vector);
    std::cout << "Vectors are sorted correctly!" << std::endl;

     //unsigned int n = std::thread::hardware_concurrency();
     //std::cout << n << " concurrent threads are supported.\n";

    return 0;
}
