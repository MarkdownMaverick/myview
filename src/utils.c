#include "include/main.h"
#include <string.h>
#include <ctype.h>  
#include <limits.h>
uint32_t simple_hash(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; 
    return hash;
}
Rectangle GetButtonRect(int index) {
    float screenWidth = (float)GetScreenWidth();
    float centerX = screenWidth / 2.0f;
    return (Rectangle){centerX - 150, 300.0f + (index * 100.0f), 300, 60};
}
bool IsValidMediaFile(const char *path) {
    if (!path || strlen(path) == 0)
        return false;
    const char *validExts[] = {".mp4", ".mkv", ".mp3", ".avi", ".mov", ".wav", ".flac", ".m4a", ".webm"};
    const char *ext = strrchr(path, '.');
    if (!ext)
        return false;
    for (int i = 0; i < 9; i++) {
        if (strcasecmp(ext, validExts[i]) == 0)
            return true;
    }
    return false;
}
char *OpenFileDialog(void) {
    static char path[MAX_PATH_LENGTH];
    FILE *f = popen("zenity --file-selection --title=\"Select Media\" --file-filter=\"Media | *.mp4 *.mkv *.mp3 *.avi *.mov *.wav *.flac *.m4a *.webm\"", "r");
    if (f == NULL) {
        fprintf(stderr, "Error: Failed to open file dialog\n");
        return NULL;
    }
    if (fgets(path, MAX_PATH_LENGTH, f) != NULL) {
        path[strcspn(path, "\n")] = 0;
        int status = pclose(f);
        if (status == -1) {
            fprintf(stderr, "Error: Failed to close file dialog\n");
            return NULL;
        }
        return path;
    }
    pclose(f);
    return NULL;
}