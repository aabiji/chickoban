#include <raymath.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "levels.h"

typedef struct { int length; char* str; } Line;

Piece getPiece(Line* lines, int width, int height, int x, int y) {
    Piece p = { Empty, false };
    if (x >= lines[y].length) return p;
    char c = lines[y].str[x];

    if (c == '$' || c == '*') {
        p = (Piece){ Box, c == '*' };
        p.boxSlide = createAnimation((Vector2){x, y}, false, PLAYER_SPEED);
    }
    if (c == '.') p = (Piece){ Empty, true };
    if (c == '#') p = (Piece){ Border, false };
    return p;
}

Level parseLevel(Line* lines, int width, int height) {
    Level level = {
        .numGoals = 0,
        .width = width, .height = height,
        .pieces = calloc(width * height, sizeof(Piece)),
        .original = calloc(width * height, sizeof(Piece)),
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            Piece p = getPiece(lines, width, height, x, y);
            level.original[index] = p;
            level.pieces[index] = p;

            if (p.isGoal)
                level.goalIndexes[level.numGoals++] = index;

            if (x < lines[y].length && lines[y].str[x] == '@') {
                level.playerStartX = x;
                level.playerStartY = y;
            }
        }
    }

    return level;
}

int parseLevels(char* filePath, Level* levels) {
    FILE* file = fopen(filePath, "r");
    if (file == NULL) return -1;

    int width = 0;
    int height = 0;
    Line lines[40];

    int i = 0;
    char line[100];

    while (fgets(line, sizeof(line), file)) {
        size_t length = strlen(line);

        if (length == 1 && line[0] == '\n') { // puzzles are separated by a new line
            levels[i++] = parseLevel(lines, width, height);

            // reset
            for (int y = 0; y < height; y++) {
                free(lines[y].str);
            }
            width = height = 0;
        } else {
            width = length > width ? length : width; // lines can be of different lengths
            lines[height++] = (Line){ length, strdup(line) };
        }
    }

    // parse the last level
    levels[i++] = parseLevel(lines, width, height);
    for (int y = 0; y < height; y++) {
        free(lines[y].str);
    }

    fclose(file);
    return i != NUM_LEVELS ? -1 : 0;
}

void restartLevel(Level* level) {
    int size = level->width * level->height * sizeof(Piece);
    memcpy(level->pieces, level->original, size);
}

// return true if all the goal positions are covered by a box
int countCompletedGoals(Level* level) {
    int completed = 0;
    for (int i = 0; i < level->numGoals; i++) {
        int pos = level->goalIndexes[i];
        Piece p = level->pieces[pos];
        if (p.isGoal && p.type == Box)
            completed++;
    }
    return completed;
}

void solveLevel(Level* level) {
    int nextGoal = 0;
    for (int y = 0; y < level->height; y++) {
        for (int x = 0; x < level->width; x++) {
            int index = y * level->width + x;
            Piece p = level->pieces[index];

            // Move box into the next available goal position
            if (!p.isGoal && p.type == Box) {
                int goalIndex = level->goalIndexes[nextGoal++];
                int goalX = goalIndex % level->width;
                int goalY = (goalIndex - goalX) / level->width;

                level->pieces[goalIndex].type = Box;
                level->pieces[goalIndex].boxSlide =
                    createAnimation((Vector2){ goalX, goalY }, false, PLAYER_SPEED);
                level->pieces[index].type = Empty;
            }
        }
    }
}
