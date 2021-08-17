/**
 * @file  sorting.h
 * @brief List of sorting algorithms
 *
 * Defines a generic sorting function and several instances in the form of
 * popular sorting algorithms, including
 *      - Selection sort
 *      - Insertion sort
 *      - Bubble sort
 *      - Merge sort
 *      - Merge sort with (naive) parallelism
 *      - Quick sort
 *      - Quick sort with (naive) parallelism
 *      - Quick sort with (naive) parallelism and random pivot
 *      - std::sort from <algorithm>
 *
 * @author David Chen <dchen2@andrew.cmu.edu>
 */

#ifndef __SORTING_H__
#define __SORTING_H__

#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <algorithm>


template <class T>
using cmp_fn = std::function<bool(T&, T&)>;

template <class T>
using sort_fn = std::function<void(std::vector<T>& v, cmp_fn<T> cmp)>;


/* Selection Sort */
template <class T>
void selection_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    for (size_t i = 0; i < v.size(); i++) {
        int smallest = i;
        for (size_t j = i + 1; j < v.size(); j++) {
            if (cmp(v[j], v[smallest]))
                smallest = j;
        }
        T temp = v[i];
        v[i] = v[smallest];
        v[smallest] = temp;
    }
}


/* Insertion Sort */
template <class T>
void insertion_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    for (size_t i = 1; i < v.size(); i++) {
        if (cmp(v[i], v[i - 1])) {
            size_t j = 0;
            while (j < i && cmp(v[j], v[i]))
                j++;
            T temp;
            while (j < i) {
                temp = v[j];
                v[j] = v[i];
                v[i] = temp;
                j++;
            }
        }
    }
}


/* Bubble Sort */
template <class T>
void bubble_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    if (v.size() <= 1)
        return;
    for (size_t i = 0; i < v.size(); i++) {
        for (size_t j = 0; j < v.size() - i - 1; j++) {
            if (cmp(v[j + 1], v[j])) {
                T temp = v[j];
                v[j] = v[j + 1];
                v[j + 1] = temp;
            }
        }
    }
}


/* Merge Sort */
template <class T> void merge_sort(std::vector<T>& v, cmp_fn<T> cmp);

template <class T>
void merge(std::vector<T>& v, cmp_fn<T> cmp, size_t lo, size_t hi) {
    size_t mid = lo + (hi - lo) / 2;
    size_t i1 = lo, i2 = mid;
    std::vector<T> u;
    while (i1 < mid || i2 < hi) {
        if (i1 == mid) {
            u.push_back(v[i2]);
            i2++;
        }
        else if (i2 == hi) {
            u.push_back(v[i1]);
            i1++;
        }
        else {
            if (cmp(v[i1], v[i2])) {
                u.push_back(v[i1]);
                i1++;
            }
            else {
                u.push_back(v[i2]);
                i2++;
            }
        }
    }
    std::copy(u.begin(), u.end(), v.begin() + lo);
}

template <class T>
void merge_sort_helper(std::vector<T>& v, cmp_fn<T> cmp,
                       size_t lo, size_t hi) {
    if (hi - lo <= 1)
        return;
    size_t mid = lo + (hi - lo) / 2;
    merge_sort_helper<T>(v, cmp, lo, mid);
    merge_sort_helper<T>(v, cmp, mid, hi);
    merge(v, cmp, lo, hi);
};

template <class T>
void merge_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    merge_sort_helper<T>(v, cmp, 0, v.size());
}


/* Parallel Merge Sort */
template <class T> void pmerge_sort(std::vector<T>& v, cmp_fn<T> cmp);

template <class T>
void pmerge_sort_helper(std::vector<T>& v, cmp_fn<T> cmp,
                        size_t lo, size_t hi) {
    if (hi - lo <= 1)
        return;
    size_t mid = lo + (hi - lo) / 2;
    std::thread head(pmerge_sort_helper<T>, std::ref(v), cmp, lo, mid);
    std::thread tail(pmerge_sort_helper<T>, std::ref(v), cmp, mid, hi);
    head.join();
    tail.join();
    merge(v, cmp, lo, hi);
};

template <class T>
void pmerge_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    pmerge_sort_helper<T>(v, cmp, 0, v.size());
}


/* Quick Sort */
template <class T> void quick_sort(std::vector<T>& v, cmp_fn<T> cmp);

template <class T>
size_t partition(std::vector<T>& v, cmp_fn<T> cmp, size_t lo, size_t hi, size_t p) {
    T vp = v[p];
    std::vector<T> u1, u2;
    for (size_t i = lo; i < hi; i++) {
        if (i == p)
            continue;
        if (cmp(v[i], vp))
            u1.push_back(v[i]);
        else
            u2.push_back(v[i]);
    }
    std::copy(u1.begin(), u1.end(), v.begin() + lo);
    v[lo + u1.size()] = vp;
    std::copy(u2.begin(), u2.end(), v.begin() + lo + u1.size() + 1);
    return u1.size();
}

template <class T>
void quick_sort_helper(std::vector<T>& v, cmp_fn<T> cmp,
                       size_t lo, size_t hi) {
    if (hi - lo <= 1)
        return;
    size_t p = partition(v, cmp, lo, hi, lo + (hi - lo) / 2);
    quick_sort_helper(v, cmp, lo, lo + p);
    quick_sort_helper(v, cmp, lo + p + 1, hi);
}

template <class T>
void quick_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    quick_sort_helper(v, cmp, 0, v.size());
}


/* Parallel Quick Sort */
template <class T> void pquick_sort(std::vector<T>& v, cmp_fn<T> cmp);

template <class T>
void pquick_sort_helper(std::vector<T>& v, cmp_fn<T> cmp,
                        size_t lo, size_t hi) {
    if (hi - lo <= 1)
        return;
    size_t p = partition(v, cmp, lo, hi, lo + (hi - lo) / 2);
    std::thread head(pquick_sort_helper<T>, std::ref(v), cmp, lo, lo + p);
    std::thread tail(pquick_sort_helper<T>, std::ref(v), cmp, lo + p + 1, hi);
    head.join();
    tail.join();
}

template <class T>
void pquick_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    pquick_sort_helper(v, cmp, 0, v.size());
}


/* Randomized Parallel Quick Sort */
template <class T> void rpquick_sort(std::vector<T>& v, cmp_fn<T> cmp);

template <class T>
void rpquick_sort_helper(std::vector<T>& v, cmp_fn<T> cmp,
                         size_t lo, size_t hi) {
    if (hi - lo <= 1)
        return;
    size_t p = partition(v, cmp, lo, hi, rand() % (hi - lo) + lo);
    std::thread head(rpquick_sort_helper<T>, std::ref(v), cmp, lo, lo + p);
    std::thread tail(rpquick_sort_helper<T>, std::ref(v), cmp, lo + p + 1, hi);
    head.join();
    tail.join();
}

template <class T>
void rpquick_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    rpquick_sort_helper(v, cmp, 0, v.size());
}


/* std::sort */
template <class T>
void std_sort(std::vector<T>& v, cmp_fn<T> cmp) {
    std::sort(v.begin(), v.end(), cmp);
}

#endif