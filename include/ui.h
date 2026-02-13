#ifndef UI_H
#define UI_H
#include "main.h"
const char *GENRE_LIST[] = {
    "All", "Biography", "Comedy", "Crime", "Documentary", "Drama", "Family",
    "Fantasy", "History", "Horror", "Kids", "Music", "Musical", "Mystery",
    "News", "Romance", "Sci-Fi & Fantasy", "Sport", "Thriller", "War", NULL};
const char *MUSIC_GENRE_LIST[] = {
    "All", "Alternative", "Blues", "Classical", "Country", "Dance", "Electronic",
    "Folk", "Funk", "Hip-Hop", "Indie", "Jazz", "Latin", "Metal", "Pop",
    "Punk", "R&B", "Reggae", "Rock", "Soul", NULL};
void DrawMainMenu(AppState *g);
void UpdateMainMenu(AppState *g);
void DrawMediaGrid(AppState *g, MediaLibrary *lib, const char *title);
void DrawPlayer(AppState *g);
void UpdateMediaGrid(AppState *g, MediaLibrary *lib);
void UpdatePlayer(AppState *g);
void DrawSettings(AppState *g);
void UpdateSettings(AppState *g);
#endif