Project Name: Sorting Visualizer

Project Description: Animates many different sorting algorithms. If desired,
	users can add their own algorithms. Parallel algorithms are supported.

Running the project: The project must be compiled with the SFML modules
	graphics, window, and system which have their own dependencies that need to
	be linked. Depending on your software, please visit
	https://www.sfml-dev.org/tutorials/2.5/ for build instructions.
	Other libraries are sorting.h and sorting_animator.h/cpp. To include your
	own sorting algorithms, you must write the corresponding function in a
	file to be compiled (perhaps sorting.h or main.cpp) and then add it into the
	application with the add_sort method. See main.cpp for examples.
