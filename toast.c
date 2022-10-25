#include <dirent.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include "ncurses.h"

#define PAD 1

#define PRIM_COLOR 1
#define SECD_COLOR 2
#define EXIT_COLOR 3

#define NO_COL_W 4
#define DR_COL_W 14
#define TP_COL_W 6
#define TMP_PREC 1

struct curses_info {
    WINDOW *window;
    int old_cursor;
};
struct curses_info c_info;

struct thermal_info {
    char **paths;
    char **types;
    int num_zones;
};
struct thermal_info thrm_info;

float get_temp(char *path) {
    FILE *temp_file = fopen(path, "r");
    float temp;
    fscanf(temp_file, "%f", &temp);
    fclose(temp_file);
    
    temp /= 1000;
    temp = ceilf(temp * 100) / 100;
    return temp;
}

int get_num_thermal_zones() {
    int num_zones = 0;

    DIR *dp;
    struct dirent *ep;

    const char *path = "/sys/class/thermal";
    dp = opendir(path);
    if (dp != NULL) {
        while ( (ep = readdir(dp)) ) {
            int size = 12;
            char prefix[size];
            memcpy(prefix, &ep->d_name, size);
            prefix[size] = '\0';  

            if (strcmp(prefix, "thermal_zone") == 0) {
                num_zones++;
            }
        }
    }
    return num_zones;
}

void print_header() {

    attron(COLOR_PAIR(PRIM_COLOR));
    mvprintw(1, PAD, "_____  ___    __    __  _____ ");
    mvprintw(2, PAD, " | |  / / \\  / /\\  ( (`  | |  ");
    mvprintw(3, PAD, " |_|  \\_\\_/ /_/--\\ _)_)  |_|  ");
    attroff(COLOR_PAIR(PRIM_COLOR));

    attron(COLOR_PAIR(SECD_COLOR));
    mvprintw(5, PAD, "%*s", NO_COL_W, "No");
    mvprintw(5, PAD + NO_COL_W, "%*s", DR_COL_W, "Driver");
    mvprintw(5, PAD + NO_COL_W + DR_COL_W, "%*s", TP_COL_W + TMP_PREC + PAD, "Temp ");
    attroff(COLOR_PAIR(SECD_COLOR));

    attron(COLOR_PAIR(EXIT_COLOR));
    mvprintw(LINES-1, 1, " Press q to quit.");
    attroff(COLOR_PAIR(EXIT_COLOR));
}

void get_thermal_zones() {
    const char *base_p = "/sys/class/thermal/thermal_zone";
    
    for (int i = 0; i < thrm_info.num_zones; ++i) {
        int path_len = strlen(base_p) + strlen("/temp") + 3;
        thrm_info.paths[i] = malloc(path_len * sizeof (char));
        snprintf(thrm_info.paths[i], path_len, "%s%d/temp", base_p, i);
        
        char *type_p = malloc(path_len * sizeof (char));
        snprintf(type_p, path_len, "%s%d/type", base_p, i);

        FILE *f = fopen(type_p, "r");
        
        thrm_info.types[i] = malloc(64);
        memset(thrm_info.types[i], 0x00, 64);
        fscanf(f, "%s", thrm_info.types[i]);
        fclose(f);
    }
}

void run() {
    while (1) {
        for (int i = 0; i < thrm_info.num_zones; ++i) {
            
            float temp = get_temp(thrm_info.paths[i]);
            
            mvprintw(6 + i, PAD, "%*d", NO_COL_W, i);
            mvprintw(6 + i, PAD + NO_COL_W, " %*s", DR_COL_W, thrm_info.types[i]);
            mvprintw(6 + i, PAD + NO_COL_W + DR_COL_W, " %*.*f", TP_COL_W, TMP_PREC, temp);
            refresh();
        }
        timeout(1000);
        if (getch() == 'q') return;
    }
}

void start_curses() {
    if ((c_info.window = initscr()) == NULL) {
        perror("Could not initialize window");
    }
    c_info.old_cursor = curs_set(0);

    // For utf-8
    setlocale(LC_CTYPE, "");

    if (!has_colors()) {
        endwin();
        perror("Your terminal does not support colors");
        exit(EXIT_FAILURE);
    }
    if (start_color() < 0) perror("Could not enable colors");
    
    init_pair(PRIM_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair(SECD_COLOR, COLOR_BLACK, COLOR_BLUE);
    init_pair(EXIT_COLOR, COLOR_BLACK, COLOR_WHITE);
}

void stop_curses() {
    delwin(c_info.window);
    curs_set(c_info.old_cursor);
    endwin();
    refresh();
}

void cleanup() {
    for (int i = 0; i < thrm_info.num_zones; ++i) {
        free(thrm_info.paths[i]);
        free(thrm_info.types[i]);
    }
    free(thrm_info.paths);
    free(thrm_info.types);
}

int main() {
    start_curses();

    print_header();
    
    thrm_info.num_zones = get_num_thermal_zones();
    thrm_info.paths = malloc(thrm_info.num_zones * sizeof(char *));
    thrm_info.types = malloc(thrm_info.num_zones * sizeof(char *));
    get_thermal_zones(thrm_info.paths, thrm_info.types, thrm_info.num_zones);

    run();

    stop_curses();
    cleanup();
}
