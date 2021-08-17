/**
 * @file  sorting_animator.cpp
 * @brief Implementation of sorting_animator.h
 * 
 * Most features are algorithmically simple, but some notable features include
 *      - Comparisons between data elements are detected by hacking into the
 *        comparison function supplied during the sort and forcing a draw
 *        after every comparison.
 *      - To perform multiple sorts at the same time, threads are used. This
 *        also means that the animation may not be accurate in terms of speed
 *        because the operating system may give some threads priority over
 *        others.
 *      - Word wrap in the help message is performed by cutting the text into
 *        smaller parts that fit within the screen. This is implemented via
 *        recording the accumulated width of every word and cutting off the
 *        text when the accumulator bypasses the width.
 * More documentation found in sorting_animator.h
 * 
 * @author David Chen <dchen2@andrew.cmu.edu>
 */

#include "sorting.h"
#include "sorting_animator.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <fstream>
#include <iostream>
#include <SFML/Graphics.hpp>


/***** Helpers *****/

template <class T>
bool in_btw(T lo, T hi, T x) {
    return lo <= x && x <= hi;
}

float get_xmid(sf::FloatRect r) {
    return r.left + 0.5f * r.width;
}

float get_xright(sf::FloatRect r) {
    return r.left + r.width;
}

float get_ymid(sf::FloatRect r) {
    return r.top + 0.5f * r.height;
}

float get_ybot(sf::FloatRect r) {
    return r.top + r.height;
}

bool in_box(sf::FloatRect r, float x, float y) {
    return in_btw<float>(r.left, get_xright(r), x)
        && in_btw<float>(r.top, get_ybot(r), y);
}

void resize(sf::RenderWindow& window, int w, int h) {
    window.setSize(sf::Vector2u(w, h));
    window.setView(sf::View(sf::FloatRect(0.0f, 0.0f, (float)w, (float)h)));
}

/***** Setup *****/

SortingAnimator::SortingAnimator() {
    width  = 800;
    height = 500;
    mode   = Mode::START;
    if (!text_font.loadFromFile("WalkWay_Black.ttf")) {
        std::cout << "Error in loading font" << "\n";
        exit(1);
    }
    text_size   = 50;
    text_sizef  = 50.0f;
    text_vspace = 25.0f;

    setup_start();
    setup_help_wrapper();
    sort_n = 100;
    sort_cmp = [&](SortingDatum& x, SortingDatum& y) {
        x.timer = 5;
        y.timer = 5;
        sort_draw_data();
        return x.value <= y.value;
    };
}

void SortingAnimator::setup_start() {
    sf::FloatRect title_box;
    start_title.setString("Welcome!");
    start_title.setFont(text_font);
    start_title.setCharacterSize(3 * text_size);
    start_title.setFillColor(sf::Color::Red);
    title_box = start_title.getLocalBounds();
    start_title.setOrigin(get_xmid(title_box), title_box.top);
    start_title.setPosition(0.5f * width, text_sizef);
    title_box = start_title.getGlobalBounds();

    sf::FloatRect subtitle_box;
    start_subtitle.setString("Press any key to continue.");
    start_subtitle.setFont(text_font);
    start_subtitle.setCharacterSize(text_size);
    start_subtitle.setFillColor(sf::Color::Red);
    subtitle_box = start_subtitle.getLocalBounds();
    start_subtitle.setOrigin(get_xmid(subtitle_box), subtitle_box.top);
    start_subtitle.setPosition(0.5f * width, get_ybot(title_box));
}

void SortingAnimator::setup_help(bool scroll) {
    std::string   in_line;
    std::ifstream help_file("help_msg.txt");
    sf::Text      out_line;
    sf::FloatRect out_line_box;
    float top    = text_sizef;
    float xlimit = scroll ? width - text_sizef : width;
    out_line.setFillColor(sf::Color::Red);
    out_line.setCharacterSize(text_size);
    out_line.setFont(text_font);
    if (!help_file.is_open())
        std::cout << "Error opening help file" << std::endl;
    else {
        while (std::getline(help_file, in_line)) {
            out_line.setString(in_line);
            out_line_box = out_line.getLocalBounds();
            out_line.setOrigin(out_line_box.left, out_line_box.top);
            out_line.setPosition(text_sizef, top);
            out_line_box = out_line.getGlobalBounds();
            while (get_xright(out_line_box) > xlimit) {
                size_t i = 0, j = 0;
                while (out_line.findCharacterPos(j).x < xlimit) {
                    if (in_line[j] == ' ')
                        i = j;
                    j++;
                }
                out_line.setString(in_line.substr(0, i));
                help_lines.push_back(out_line);
                help_lines_dim.push_back(
                    sf::Vector2f(out_line_box.left, out_line_box.top)
                );
                top += out_line_box.height + text_vspace;
                in_line = in_line.substr(i + 1);
                out_line.setString(in_line);
                out_line_box = out_line.getLocalBounds();
                out_line.setOrigin(out_line_box.left, out_line_box.top);
                out_line.setPosition(2.0f * text_sizef, top);
                out_line_box = out_line.getGlobalBounds();
            }
            help_lines.push_back(out_line);
            help_lines_dim.push_back(
                sf::Vector2f(out_line_box.left, out_line_box.top)
            );
            top += out_line_box.height + text_vspace;
        }
        help_file.close();
    }
}

void SortingAnimator::setup_help_wrapper() {
    help_scroll = -1.0f;
    setup_help();
    if (help_lines.empty())
        return;
    if (get_ybot(help_lines.back().getGlobalBounds()) > height) {
        help_lines.clear();
        help_lines_dim.clear();
        help_scroll = 0.0f;
        setup_help(true);
        float ymax = get_ybot(help_lines.back().getGlobalBounds())
                   + text_sizef;
        help_scale = height / ymax;
    }
}

void SortingAnimator::add_sort(std::string name, sort_fn<SortingDatum> sort) {
    sort_algos.push_back(SortingAlgo(name, sort));
}

void SortingAnimator::launch() {
    setup_config();
    window.create(
        sf::VideoMode(width, height),
        "Sorting Visualizer",
        sf::Style::Titlebar | sf::Style::Close
    );
}

void SortingAnimator::setup_config() {
    config_n_string = std::to_string(sort_n);
    config_field    = 0;
    config_entry    = config_n_string.size();
    config_scroll   = -1.0f;

    sf::FloatRect n_box;
    config_n_text.setString("Quantity: " + config_n_string);
    config_n_text.setFont(text_font);
    config_n_text.setCharacterSize(text_size);
    config_n_text.setFillColor(sf::Color::Red);
    config_n_text.setPosition(text_sizef, text_sizef);
    n_box = config_n_text.getGlobalBounds();
    config_boxes.push_back(n_box);

    sf::FloatRect sort_box;
    config_sort_title.setString("Sorting Algorithms:");
    config_sort_title.setFont(text_font);
    config_sort_title.setCharacterSize(text_size);
    config_sort_title.setFillColor(sf::Color::Red);
    config_sort_title.setPosition(text_sizef, get_ybot(n_box) + text_vspace);
    sort_box = config_sort_title.getGlobalBounds();

    sf::FloatRect cont_box;
    config_cont.setString("Continue");
    config_cont.setFont(text_font);
    config_cont.setCharacterSize(text_size);
    config_cont.setFillColor(sf::Color::Red);
    config_cont.setOrigin(0, text_sizef);
    config_cont.setPosition(text_sizef, height - text_sizef);
    cont_box = config_cont.getGlobalBounds();

    config_sort_top = get_ybot(sort_box) + text_vspace;
    config_sort_bot = cont_box.top - text_vspace;

    sf::Text sort_entry;
    sf::FloatRect sort_entry_box;
    float top = config_sort_top;
    sort_entry.setFont(text_font);
    sort_entry.setCharacterSize(text_size);
    for (size_t i = 0; i < sort_algos.size(); i++) {
        sort_entry.setString(sort_algos[i].name);
        sort_entry_box = sort_entry.getLocalBounds();
        sort_entry.setOrigin(sort_entry_box.left, sort_entry_box.top);
        sort_entry.setPosition(2.0f * text_sizef, top);
        sort_entry_box = sort_entry.getGlobalBounds();
        config_boxes.push_back(sort_entry_box);
        top += sort_entry_box.height + text_vspace;
    }
    if (top - text_vspace > config_sort_bot) {
        config_scroll = 0.0f;
        float view_dim = config_sort_bot - config_sort_top;
        float real_dim = get_ybot(config_boxes.back()) - config_boxes[1].top;
        config_scale = view_dim / real_dim;
    }

    config_boxes.push_back(cont_box);

    float pi = 3.1415f;
    float s  = text_vspace;
    float t  = pi / 6;
    config_ptr.setPointCount(3);
    config_ptr.setPoint(0, sf::Vector2f(0.0f, 0.0f));
    config_ptr.setPoint(1, sf::Vector2f(s * cos(t), s * sin(t)));
    config_ptr.setPoint(2, sf::Vector2f(0.0f, s));
    config_ptr.setOrigin(0.5f * s * tan(t), 0.5f * s);

    config_entry_ptr.setSize(sf::Vector2f(2.0f, text_sizef));
    config_entry_ptr.setFillColor(sf::Color::White);

    mouse_scrolling = false;
}

/***** Event Handling *****/

void SortingAnimator::update_n() {
    if (config_n_string.empty())
        config_n_string = "0";
    sort_n = std::stoi(config_n_string);
    config_n_string = std::to_string(sort_n);
}

void SortingAnimator::update_help_scroll() {
    if (help_scroll < 0.0f)
        help_scroll = 0;
    else if (height / help_scale - help_scroll < height)
        help_scroll = height / help_scale - height;
}

void SortingAnimator::update_config_scroll() {
    float top = config_boxes[1].top;
    float bot = get_ybot(config_boxes[sort_algos.size()]);
    if (top - config_scroll > config_sort_top)
        config_scroll = top - config_sort_top;
    else if (bot - config_scroll < config_sort_bot)
        config_scroll = bot - config_sort_bot;
}

void SortingAnimator::update_config_scroll(int i) {
    float top = config_boxes[i].top;
    float bot = get_ybot(config_boxes[i]);
    if (top - config_scroll < config_sort_top)
        config_scroll = top - config_sort_top;
    else if (bot - config_scroll > config_sort_bot)
        config_scroll = bot - config_sort_bot;
}

void SortingAnimator::handle(sf::Event event) {
    switch (event.type) {
    case sf::Event::Closed:
        window.close();
        break;
    case sf::Event::KeyPressed:
        handle_key(event);
        break;
    case sf::Event::MouseWheelScrolled:
        handle_mouse_wheel(event);
        break;
    case sf::Event::MouseButtonPressed:
        handle_mouse_pressed(event);
        break;
    case sf::Event::MouseButtonReleased:
        handle_mouse_released(event);
        break;
    case sf::Event::MouseMoved:
        handle_mouse_moved(event);
        break;
    }
}

void SortingAnimator::handle_key(sf::Event event) {
    switch (mode) {
    case Mode::START:
        mode = Mode::HELP;
        break;
    case Mode::HELP:
        handle_key_help(event);
        break;
    case Mode::CONFIG:
        handle_key_config(event);
        break;
    case Mode::SORTING:
        break;
    case Mode::SORTED:
        handle_key_sorted(event);
        break;
    }
}

void SortingAnimator::handle_key_help(sf::Event event) {
    switch (event.key.code) {
    case (sf::Keyboard::Escape):
    case (sf::Keyboard::Enter):
        mode = Mode::CONFIG;
        break;
    case (sf::Keyboard::Up):
    case (sf::Keyboard::Left):
        if (help_scroll >= 0.0f) {
            help_scroll -= text_sizef;
            update_help_scroll();
        }
        break;
    case (sf::Keyboard::Down):
    case (sf::Keyboard::Right):
        if (help_scroll >= 0.0f) {
            help_scroll += text_sizef;
            update_help_scroll();
        }
    }
}

void SortingAnimator::handle_key_config(sf::Event event) {
    if (in_btw(sf::Keyboard::Num0, sf::Keyboard::Num9, event.key.code)) {
        config_n_string = config_n_string.substr(0, config_entry)
            + std::to_string(event.key.code - sf::Keyboard::Num0)
            + config_n_string.substr(config_entry);
        config_entry++;
    } else if (event.key.code == sf::Keyboard::H) {
        mode = Mode::HELP;
        return;
    } else if (event.key.code == sf::Keyboard::Up) {
        config_field = std::max(0, config_field - 1);
        if (!config_field)
            config_entry = config_n_string.size();
    } else if (event.key.code == sf::Keyboard::Down) {
        if (!config_field)
            update_n();
        config_field = std::min((int)sort_algos.size() + 1, config_field + 1);
    } else if (event.key.code == sf::Keyboard::Left) {
        if (!config_field)
            config_entry = std::max(config_entry - 1, 0);
        else {
            config_field--;
            if (!config_field)
                config_entry = config_n_string.size();
        }
    } else if (event.key.code ==  sf::Keyboard::Right) {
        if (!config_field && config_entry < (int)config_n_string.size())
            config_entry++;
        else if (!config_field) {
            update_n();
            config_field++;
        } else if (config_field < (int)sort_algos.size() + 1)
            config_field++;
    } else if (event.key.code == sf::Keyboard::Backspace) {
        if (!config_field && config_entry > 0) {
            config_n_string = config_n_string.substr(0, config_entry - 1)
                            + config_n_string.substr(config_entry);
            if (config_n_string.empty()) {
                sort_n          = 0;
                config_n_string = "0";
            } else {
                sort_n          = std::stoi(config_n_string);
                config_n_string = std::to_string(sort_n);
            }
            config_entry--;
        }
    } else if (event.key.code == sf::Keyboard::Delete) {
        if (!config_field && config_entry < (int)config_n_string.size()) {
            config_n_string = config_n_string.substr(0, config_entry)
                            + config_n_string.substr(config_entry + 1);
            if (config_n_string.empty()) {
                sort_n          = 0;
                config_n_string = "0";
            } else {
                sort_n          = std::stoi(config_n_string);
                config_n_string = std::to_string(sort_n);
            }
        }
    } else if (event.key.code == sf::Keyboard::Tab) {
        if (!config_field)
            update_n();
        config_field = (config_field + 1) % (sort_algos.size() + 2);
        if (!config_field)
            config_entry = config_n_string.size();
    } else if (event.key.code == sf::Keyboard::Enter) {
        if (!config_field) {
            update_n();
            config_field++;
        } else if (config_field < (int)sort_algos.size() + 1) {
            sort_algos[config_field - 1].selected =
                not sort_algos[config_field - 1].selected;
        } else {
            sort_setup();
            mode = Mode::SORTING;
        }
    } else if (event.key.code == sf::Keyboard::Escape) {
        mode = Mode::START;
        return;
    }

    if (config_scroll >= 0.0f
    &&  in_btw<int>(1, sort_algos.size(), config_field))
        update_config_scroll(config_field);
}

void SortingAnimator::handle_key_sorted(sf::Event event) {
    switch (event.key.code) {
    case (sf::Keyboard::Escape):
    case (sf::Keyboard::Enter):
    case (sf::Keyboard::Backspace):
        resize(window, width, height);
        if (sort_data.size() > 0) {
            for (size_t i = 0; i < sort_data.size(); i++)
                sort_data[i].clear();
            sort_data.clear();
            sort_queue.clear();
            sort_threads.clear();
        }
        mode = Mode::CONFIG;
        break;
    case (sf::Keyboard::R):
        for (size_t i = 0; i < sort_queue.size(); i++) {
            for (size_t j = 0; j < sort_n; j++)
                sort_data[i][j].value = sort_data[sort_queue.size()][j].value;
        }
        mode = Mode::SORTING;
    }
}

void SortingAnimator::handle_mouse_pressed(sf::Event event) {
    int mx = event.mouseButton.x, my = event.mouseButton.y;
    if (mode == Mode::START) {
        mode = Mode::HELP;
    } else if (mode == Mode::HELP) {
        if (help_scroll >= 0.0f
        &&  in_btw<int>(width - text_size, width, mx)) {
            mouse_scrolling = true;
            help_scroll = my / help_scale;
            update_help_scroll();
        } else {
            mode = Mode::CONFIG;
        }
    } else if (mode == Mode::CONFIG) {
        update_n();
        if (config_scroll >= 0.0f
        &&  in_btw<int>(width - text_size, width, mx)
        &&  in_btw<float>(config_sort_top, config_sort_bot, (float)my)) {
            mouse_scrolling = true;
            config_scroll = (my - config_sort_top) / config_scale;
            update_config_scroll();
            return;
        }
        if (in_box(config_boxes[0], (float)mx, (float)my)) {
            config_field = 0;
            config_entry = config_n_string.size();
            return;
        }
        for (size_t i = 1; i < sort_algos.size() + 1; i++) {
            bool has_scroll = config_scroll >= 0.0f;
            if (!has_scroll)
                config_scroll = 0.0f;
            if (get_ybot(config_boxes[i]) - config_scroll < config_sort_top)
                continue;
            if (config_boxes[i].top - config_scroll > config_sort_bot)
                break;
            if (in_box(config_boxes[i], (float)mx, (float)my + config_scroll)
            &&  in_btw<float>(config_sort_top, config_sort_bot, (float)my)) {
                config_field = i;
                sort_algos[i - 1].selected = not sort_algos[i - 1].selected;
                update_config_scroll(i);
                if (!has_scroll)
                    config_scroll = -1.0f;
                return;
            }
            if (!has_scroll)
                config_scroll = -1.0f;
        }
        if (in_box(config_boxes.back(), (float)mx, (float)my)) {
            sort_setup();
            mode = Mode::SORTING;
        }
    }
}

void SortingAnimator::handle_mouse_released(sf::Event event) {
    if (mouse_scrolling)
        mouse_scrolling = false;
}

void SortingAnimator::handle_mouse_moved(sf::Event event) {
    if (!mouse_scrolling)
        return;
    if (mode == Mode::HELP && help_scroll >= 0.0f) {
        help_scroll = event.mouseMove.y / help_scale;
        update_help_scroll();
    } else if (mode == Mode::CONFIG && config_scroll >= 0.0f) {
        config_scroll = (event.mouseMove.y - config_sort_top) / config_scale;
        update_config_scroll();
    }
}

void SortingAnimator::handle_mouse_wheel(sf::Event event) {
    if (mode == Mode::HELP && help_scroll >= 0.0f
    &&  event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        help_scroll -= 25 * event.mouseWheelScroll.delta;
        update_help_scroll();
    } else if (mode == Mode::CONFIG && config_scroll >= 0.0f
    &&  event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        config_scroll -= 25 * event.mouseWheelScroll.delta;
        update_config_scroll();
    }
}

/***** Drawing *****/

void SortingAnimator::draw() {
    switch (mode) {
    case Mode::START:
        draw_start();
        break;
    case Mode::HELP:
        draw_help();
        break;
    case Mode::CONFIG:
        draw_config();
        break;
    case Mode::SORTING:
        sort_launch();
        break;
    case Mode::SORTED:
        break;
    }
}

void SortingAnimator::draw_start() {
    window.clear(sf::Color::Black);
    window.draw(start_title);
    window.draw(start_subtitle);
    window.display();
}

void SortingAnimator::draw_help() {
    window.clear(sf::Color::Black);
    for (size_t i = 0; i < help_lines.size(); i++) {
        if (help_scroll < 0.0f)
            window.draw(help_lines[i]);
        else {
            sf::FloatRect line_box = help_lines[i].getLocalBounds();
            help_lines[i].setOrigin(line_box.left, line_box.top);
            sf::Vector2f v = help_lines_dim[i];
            help_lines[i].setPosition(v.x, v.y - help_scroll);
            window.draw(help_lines[i]);
        }
    }
    if (help_scroll >= 0.0f) {
        sf::RectangleShape scroll;
        scroll.setSize(sf::Vector2f(text_sizef, (float)height));
        scroll.setPosition((float)width - text_sizef, 0);
        scroll.setFillColor(sf::Color(128, 128, 128));
        window.draw(scroll);
        scroll.setSize(sf::Vector2f(text_sizef, height * help_scale));
        float scroll_pos = help_scroll * help_scale;
        if (scroll_pos < 0)
            scroll_pos = 0;
        else if (scroll_pos > height)
            scroll_pos = (float)height;
        scroll.setPosition(sf::Vector2f(width - text_sizef, scroll_pos));
        scroll.setFillColor(sf::Color::Black);
        scroll.setOutlineThickness(1.0f);
        scroll.setOutlineColor(sf::Color::White);
        window.draw(scroll);
    }
    window.display();
}

void SortingAnimator::draw_config() {
    window.clear(sf::Color::Black);

    bool has_scroll = config_scroll >= 0.0f;
    if (!has_scroll)
        config_scroll = 0.0f;

    config_n_text.setString("Quantity: " + config_n_string);
    size_t i_offset = std::string("Quantity: ").size() + config_entry;
    if (!config_field) {
        float offset = config_n_text.findCharacterPos(i_offset).x;
        config_entry_ptr.setPosition(offset, text_sizef);
    }

    sf::Text sort_entry;
    sf::FloatRect sort_entry_box;
    sort_entry.setFont(text_font);
    sort_entry.setCharacterSize(text_size);
    for (size_t i = 0; i < sort_algos.size(); i++) {
        sort_entry.setString(sort_algos[i].name);
        if (sort_algos[i].selected)
            sort_entry.setFillColor(sf::Color::Blue);
        else
            sort_entry.setFillColor(sf::Color::Red);
        sort_entry_box = sort_entry.getLocalBounds();
        sort_entry.setOrigin(sort_entry_box.left, sort_entry_box.top);
        sort_entry.setPosition(
            2.0f * text_size,
            config_boxes[i + 1].top - config_scroll
        );
        window.draw(sort_entry);
    }

    if (!config_field)
        config_ptr.setPosition(text_vspace, get_ymid(config_boxes[0]));
    else if (0 < config_field && config_field < (int)sort_algos.size() + 1) {
        float mid = get_ymid(config_boxes[config_field]);
        config_ptr.setPosition(text_vspace, mid - config_scroll);
    } else if (config_field == sort_algos.size() + 1)
        config_ptr.setPosition(text_vspace, get_ymid(config_boxes.back()));

    sf::RectangleShape clear;
    clear.setFillColor(sf::Color::Black);
    clear.setSize(sf::Vector2f((float)width, config_sort_top));
    window.draw(clear);
    clear.setSize(sf::Vector2f((float)width, height - config_sort_bot));
    clear.setPosition(0, config_sort_bot);
    window.draw(clear);

    if (has_scroll) {
        sf::RectangleShape scroll;
        float scroll_height = config_sort_bot - config_sort_top;
        scroll.setSize(sf::Vector2f(text_sizef, scroll_height));
        scroll.setPosition((float)width - text_size, config_sort_top);
        scroll.setFillColor(sf::Color(128, 128, 128));
        window.draw(scroll);
        scroll.setSize(sf::Vector2f(text_sizef, scroll_height * config_scale));
        float scroll_pos = config_sort_top + config_scroll * config_scale;
        if (scroll_pos < config_sort_top)
            scroll_pos = config_sort_top;
        else if (scroll_pos + scroll_height * config_scale > config_sort_bot)
            scroll_pos = config_sort_bot - scroll_height * config_scale;
        scroll.setPosition(sf::Vector2f(width - text_sizef, scroll_pos));
        scroll.setFillColor(sf::Color::Black);
        scroll.setOutlineThickness(1.0f);
        scroll.setOutlineColor(sf::Color::White);
        window.draw(scroll);
    }

    window.draw(config_n_text);
    window.draw(config_sort_title);
    window.draw(config_cont);
    if (!in_btw<int>(1, sort_algos.size(), config_field)
    ||  in_btw(config_sort_top, config_sort_bot, config_ptr.getPosition().y))
        window.draw(config_ptr);
    if (!config_field)
        window.draw(config_entry_ptr);

    if (!has_scroll)
        config_scroll = -1.0f;
    window.display();
}

/***** Visualizing Sorting *****/

void SortingAnimator::sort_setup() {
    for (size_t i = 0; i < sort_algos.size(); i++) {
        if (sort_algos[i].selected) {
            sort_queue.push_back(i);
            sort_threads.push_back(std::thread());
        }
    }
    if (!sort_queue.size()) {
        window.clear(sf::Color::Black);
        window.display();
        mode = Mode::SORTED;
        return;
    }

    sort_data.push_back(std::vector<SortingDatum>());
    for (size_t i = 0; i < sort_n; i++)
        sort_data[0].push_back(SortingDatum(i + 1));
    std::random_shuffle(sort_data[0].begin(), sort_data[0].end());
    for (size_t j = 1; j < sort_queue.size() + 1; j++) {
        sort_data.push_back(std::vector<SortingDatum>());
        for (size_t i = 0; i < sort_n; i++)
            sort_data[j].push_back(SortingDatum(sort_data[0][i].value));
    }

    resize(window, width, sort_queue.size() * height);
    window.clear(sf::Color::Black);
    window.display();
    window.setActive(false);
}


void SortingAnimator::sort_launch() {
    for (size_t i = 0; i < sort_queue.size(); i++)
        sort_threads[i] = std::thread(
            [&](int i) {
                sort_algos[sort_queue[i]].sort(sort_data[i], sort_cmp);
            },
            i
        );
    for (size_t i = 0; i < sort_queue.size(); i++)
        sort_threads[i].join();
    sort_draw_data(true);
    mode = Mode::SORTED;
}

void SortingAnimator::sort_draw_data(bool end) {
    window_m.lock();
    window.setActive(true);
    window.clear(sf::Color::Black);
    float dx = (float)width / sort_n;
    float dy = (float)height / (sort_n + 1);
    sf::RectangleShape rect;
    sf::Text name;
    rect.setOutlineThickness(1.0f);
    rect.setOutlineColor(sf::Color::Black);
    name.setCharacterSize(text_size);
    name.setFont(text_font);
    name.setFillColor(sf::Color::Blue);
    for (size_t i = 0; i < sort_threads.size(); i++) {
        for (size_t j = 0; j < sort_n; j++) {
            rect.setSize(sf::Vector2f(dx, sort_data[i][j].value * dy));
            rect.setOrigin(sf::Vector2f(0, sort_data[i][j].value * dy));
            rect.setPosition(sf::Vector2f(j * dx, (i + 1) * (float)height));
            if (end || !sort_data[i][j].timer)
                rect.setFillColor(sf::Color::White);
            else {
                rect.setFillColor(sf::Color::Red);
                sort_data[i][j].timer--;
            }
            window.draw(rect);
        }
        name.setString(sort_algos[sort_queue[i]].name);
        name.setPosition(0.0f, i * (float)height);
        window.draw(name);
    }
    window.display();
    window.setActive(false);
    window_m.unlock();
}