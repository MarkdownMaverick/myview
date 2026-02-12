#ifndef MAIN_H
#define MAIN_H
#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <cjson/cJSON.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#define SCREEN_W 1900
#define SCREEN_H 1080
#define MAX_MEDIA_FILES 256
#define MAX_PATH_LENGTH PATH_MAX
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
typedef enum
{
    STATE_BOOT_SCREEN,
    STATE_MAIN_MENU,
    STATE_MUSIC,
    STATE_MOVIES,
    STATE_VIDEOS,
    STATE_GENRE_SELECT,
    STATE_SETTINGS,
    STATE_PLAYING
} AppStateEnum;
typedef struct
{
    char paths[MAX_MEDIA_FILES][MAX_PATH_LENGTH];
    Texture2D thumbnails[MAX_MEDIA_FILES];
    char genres[MAX_MEDIA_FILES][32];
    int count;
    bool isFavorite[MAX_MEDIA_FILES];
} MediaLibrary;
typedef struct
{
    Model logoModel;
    Vector3 logoPos;
    float logoOffsetY;
    float logoRotY;
    float logoScale;
    double lastActivityTime;
    float logoVisibility;
    bool logoAnimatingOut;
    AppStateEnum currentScreen;
    AppStateEnum previousScreen;
    int previewIndices[6];
    double lastPreviewRefresh;
    float fadeAlpha;
    bool quit;
    MediaLibrary movieLib;
    MediaLibrary videoLib;
    MediaLibrary musicLib;
    int activeGenreIdx;
    bool isTaggingMode;
    int genreSelectedIndex;
    int editingIndex;
    char currentVideoPath[MAX_PATH_LENGTH];
    bool isPaused;
    float playbackPosition;
    Texture2D backgroundTexture;
    Sound startupSound;
    Sound selectSound;
    Sound backSound;
    Font bubbleFont;
    int selectedIndex;
    float gridScrollOffset;
    double lastMouseMoveTime;
    bool isSeeking;
    float seekBarAlpha;
    double videoDuration;
    double videoTimePos;
    float seekProgress;
    int showFavorites;
    bool showingOptions;
    int optionsSelectedIndex;
    bool confirmDelete;
    int currentMediaIndex;
    mpv_handle *mpv;
    mpv_render_context *mpv_ctx;
} AppState;
bool IsValidMediaFile(const char *path);
char *OpenFileDialog(void);
Rectangle GetButtonRect(int index);
uint32_t simple_hash(const char *str);
void SaveSettings(AppState *g);
void LoadSettings(AppState *g);
void GenerateOrLoadThumbnail(MediaLibrary *lib, int index);
void RefreshThumbnails(MediaLibrary *lib);
void DrawMediaGrid(AppState *g, MediaLibrary *lib, const char *title);
void DrawMainMenu(AppState *g);
void UpdateMainMenu(AppState *g);
void DrawPlayer(AppState *g);
void DrawGenreGrid(AppState *g);
void PlayVideo(AppState *g, const char *path);
void *get_rb_proc_address(void *ctx, const char *name);
extern const char *GENRE_LIST[];
extern const char *MUSIC_GENRE_LIST[];
extern Texture2D TITLE_LOGO;
#endif