/**
 * @file  main.cpp
 *
 * Creates an animator from sorting_animator.h/cpp with the sorting algorithms
 * from sorting.h and then runs it with a typical event loop.
 * 
 * @author David Chen <dchen2@andrew.cmu.edu>
 */

#include "sorting.h"
#include "sorting_animator.h"
#include <string>
#include <iostream>
#include <SFML/Graphics.hpp>


int main() {
    SortingAnimator anim;
    anim.add_sort("Selection", selection_sort<SortingDatum>);
    anim.add_sort("Insertion", insertion_sort<SortingDatum>);
    anim.add_sort("Bubble", bubble_sort<SortingDatum>);
    anim.add_sort("Merge", merge_sort<SortingDatum>);
    anim.add_sort("Parallel Merge", pmerge_sort<SortingDatum>);
    anim.add_sort("Quicksort", quick_sort<SortingDatum>);
    anim.add_sort("Parallel Quicksort", pquick_sort<SortingDatum>);
    anim.add_sort("Randomized Parallel Quicksort", rpquick_sort<SortingDatum>);
    anim.add_sort("std::sort", std_sort<SortingDatum>);
    anim.launch();
    while (anim.window.isOpen()) {
        sf::Event event;
        while (anim.window.pollEvent(event)) {
            anim.handle(event);
        }
        anim.draw();
    }
    return 0;
}