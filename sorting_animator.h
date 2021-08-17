/**
 * @file  sorting_animator.h
 * @brief Prototype for the animator
 *
 * Defines helper functions and several classes for the animator. Also defines
 * setup, update, event handler, and draw methods for the animator.
 *
 * @author David Chen <dchen2@andrew.cmu.edu>
 */

#ifndef __SORTING_ANIMATOR_H__
#define __SORTING_ANIMATOR_H__

#include "sorting.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <SFML/Graphics.hpp>


/***** Helper Functions *****/

/**
 * @brief Checks if an argument is between two others (inclusive)
 */
template <class T> bool in_btw(T lo, T hi, T x);

/**
 * @brief Returns the x-coordinate of the center of a float rectangle
 */
float get_xmid(sf::FloatRect r);

/**
 * @brief Returns the x-coordinate of the right side of a float rectangle
 */
float get_xright(sf::FloatRect r);

/**
 * @brief Returns the y-value of the center of a float rectangle
 */
float get_ymid(sf::FloatRect r);

/**
 * @brief Returns the y-coordinate of the bottom side of a float rectangle
 */
float get_ybot(sf::FloatRect r);

/**
 * @brief Checks if a point is within a rectangle (inclusive)
 */
bool in_box(sf::FloatRect r, float x, float y);

/**
 * @brief Resizes a window and updates view accordingly to prevent stretching
 */
void resize(sf::RenderWindow& window, int w, int h);


/***** Sorting Classes *****/

/**
 * @brief Enum type for mode of the animator
 *
 * Types of modes:
 *      START:   Welcome screen
 *      HELP:    Help screen with instructions
 *      CONFIG:  Configuartions screen to adjust sorting visualizer
 *      SORTING: Visualize sorting
 *      SORTED:  Acts as a buffer after sorting to allow uers to restart
 */
enum class Mode { START, HELP, CONFIG, SORTING, SORTED };

/** @brief Data to be sorted */
struct SortingDatum {
    /** @brief Value to be considered whilst sorting */
    int value;

    /**
     * @brief Timer for color of element
     * 
     * In my implementation, if timer > 0 then the datum is drawn in red, else
     * it is drawn in white
     */
    int timer;

    SortingDatum() : value(0), timer(0) {}
    SortingDatum(int v) : value(v), timer(0) {}
};

/** @brief Struct organizing relevant sort algorithm details */
struct SortingAlgo {
    /** @brief Name of algorithm to be displayed */
    std::string name;

    /** @brief Bool indicating the algorithm is selected for visualization */
    bool selected;

    /** @brief Function to carry out the sort */
    sort_fn<SortingDatum> sort;
    
    SortingAlgo()
      : name(""), selected(false), sort(NULL) {}
    SortingAlgo(const std::string& name, const sort_fn<SortingDatum> sort)
      : name(name), selected(false), sort(sort) {}
};

/** @brief Struct implementing the animator */
class SortingAnimator {
public:
    /** @brief Initial width and height */
    int width, height;

    /** @brief Current mode of the animator */
    Mode mode;

    /** @brief Window for visualization */
    sf::RenderWindow window;

    /** @brief Font of every text */
    sf::Font text_font;

    /** @brief Regular size of (most) text */
    int text_size;

    /** @brief Float value of size */
    float text_sizef;

    /** @brief Vertical spacing between text */
    float text_vspace;

    /**
     * @brief Creates the visualizer
     *
     * To prevent bugs, we have the hardcoded values
     *      width       = 800;
     *      height      = 500;
     *      text_size   = 50;
     *      text_sizef  = 50.0f;
     *      text_vspace = 25.0f;
     * Changing these may cause bugs such as an infinite loop in the word
     * wrap algorithm for the help page or finicking scroll bars
     */
    SortingAnimator();

    /**
     * @brief Adds a sort to the animator
     *
     * @param[in] name  Name of the sort
     * @param[in] sort  Sorting algorithm as a function
     */
    void add_sort(std::string name, sort_fn<SortingDatum> sort);

    /** @brief Begins the animation (after adding desired sorts) */
    void launch();

    /** @brief Event handler */
    void handle(sf::Event event);

    /** @brief Draws the screen*/
    void draw();

private:
    /**
     * @brief Mutex to prevent multiple threads from concurrently changing
     *        window, otherwise openGL fails
     */
    std::mutex window_m;

    /** @brief Boolean indicating mouse moves should change the scroll */
    bool mouse_scrolling;


    /** @brief Text for title of start mode */
    sf::Text start_title;

    /** @brief Text for subtitle of start mode */
    sf::Text start_subtitle;


    /** @brief Text for help mode */
    std::vector<sf::Text> help_lines;

    /** @brief (x, y)-positions for each help line */
    std::vector<sf::Vector2f> help_lines_dim;

    /**
     * @brief Value of the scroll of the help mode
     *
     * To fit the entire help message on the screen, a scroll bar is created.
     * If help_scroll < 0 then no scroll is needed.
     */
    float help_scroll;

    /** @brief Ratio of height to space needed to fit help message */
    float help_scale;


    /**
     * @brief Current field being edited in config mode
     *
     * Available fields include
     *      - Quantity (number of items to be sorted)
     *      - Type of sorts
     *      - Continue (to enter the visualization)
     */
    int config_field;

    /**
     * @brief Current entry being edited in config mode
     *
     * Used only when editing the Quantity field for inserts/deletes
     */
    int config_entry;

    /**
     * @brief Value of the scroll of the config mode
     *
     * To fit every sort on the screen, a scroll bar is created, but only
     * for the list of sorts.
     * If config_scroll < 0 then no scroll is needed.
     */
    float config_scroll;

    /**
     * @brief Ratio of space given for sorts to the space need to fit every
     *        sort
     */
    float config_scale;

    /** @brief String for Quantity */
    std::string config_n_string;

    /** @brief Text for Quantity */
    sf::Text config_n_text;

    /** @brief Text for to be displayed before list of sorts */
    sf::Text config_sort_title;

    /** @brief Text for the Continue option */
    sf::Text config_cont;

    /** @brief Vector of bounding boxes of all fields in config mode */
    std::vector<sf::FloatRect> config_boxes;

    /** @brief y-coordinate for the top of the sort list */
    float config_sort_top;

    /** @brief y-coordinate for the bottom of the sort list */
    float config_sort_bot;

    /** @brief Line representing the pointer indicating config_entry */
    sf::RectangleShape config_entry_ptr;
    
    /** @brief Triangle representing the pointer indicating config_field */
    sf::ConvexShape config_ptr;


    /** @brief Comparison function for sorts */
    cmp_fn<SortingDatum> sort_cmp;

    /** @brief List of sorting algorithms */
    std::vector<SortingAlgo> sort_algos;

    /** @brief Quantity of elements to be sorted */
    size_t sort_n;

    /** @brief Vector of data to be sorted (possibly by more than one sort) */
    std::vector<std::vector<SortingDatum>> sort_data;

    /** @brief Queue for sorts to be visualized (recorded by index) */
    std::vector<size_t> sort_queue;

    /** @brief Threads for each sort selected for visualization */
    std::vector<std::thread> sort_threads;


    /** @brief Sets up texts for start screen */
    void setup_start();

    /** @brief Sets up texts for help screen */
    void setup_help(bool scroll = false);

    /** @brief Sets up texts for help screen */
    void setup_help_wrapper();

    /** @brief Sets up information for the config screen */
    void setup_config();


    /** @brief Update sort_n according to config_n_string */
    void update_n();

    /** @brief Updates scroll to prevent scrolling pass the screen */
    void update_help_scroll();

    /** @brief Updates scroll to prevent scrolling pass the first/last sorts */
    void update_config_scroll();

    /**
     * @brief Updates scroll so that a specific field is on the screen
     *
     * update_config_scroll(n) guarantees sort[n - 1] is within
     * config_sort_top and config_sort_bot
     *
     * @param[in] n  Interested field
     * @pre 1 <= n <= sort_algos.size() + 1
     */
    void update_config_scroll(int n);


    /** @brief Event handler for key presses */
    void handle_key(sf::Event event);

    /** @brief Event handler for key presses during help mode */
    void handle_key_help(sf::Event event);

    /** @brief Event handler for key presses during config mode */
    void handle_key_config(sf::Event event);

    /** @brief Event handler for key presses during sorted mode */
    void handle_key_sorted(sf::Event event);

    /** @brief Event handler for mouse presses (to start scrolling) */
    void handle_mouse_pressed(sf::Event event);

    /** @brief Event handler for mouse releases (to finish scrolling) */
    void handle_mouse_released(sf::Event event);

    /** @brief Event handler for mouse moves (to scroll) */
    void handle_mouse_moved(sf::Event event);

    /** @brief Event handler for mouse wheel events (for scrolling) */
    void handle_mouse_wheel(sf::Event event);


    /** @brief Draws the start screen */
    void draw_start();

    /** @brief Draws the start screen */
    void draw_help();

    /** @brief Draws the config screen */
    void draw_config();


    /** @brief Creates the threads and data for the desired sorts */
    void sort_setup();

    /** @brief Begin sorting */
    void sort_launch();

    /**
     * @brief Draws the data being sorted
     *
     * If end == false then draw data as regular
     * If end == true  then draw data with only white bars
     * Because of how visualization is implemented, end = true is needed
     * to draw the final result of the sort
     * 
     * param[in] end  Indicator for end of sort
     */
    void sort_draw_data(bool end = false);
};

#endif