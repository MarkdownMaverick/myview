#include "include/main.h"
#include "include/gl_compat.h"
#include <limits.h>
void *get_rb_proc_address(void *ctx, const char *name)
{
    (void)ctx;
    return (void *)(uintptr_t)glXGetProcAddress((const GLubyte *)name);
}
void PlayVideo(AppState *g, const char *path)
{
    if (!g->mpv)
        return;
    const char *cmd[] = {"loadfile", path, NULL};
    mpv_command(g->mpv, cmd);
    int pause = 0;
    mpv_set_property(g->mpv, "pause", MPV_FORMAT_FLAG, &pause);
    printf("Ã¢Å“â€œ Playback started\n");
    strncpy(g->currentVideoPath, path, MAX_PATH_LENGTH - 1);
    g->currentVideoPath[MAX_PATH_LENGTH - 1] = '\0';
    g->previousScreen = g->currentScreen;
    g->currentScreen = STATE_PLAYING;
    g->isPaused = false;
    g->playbackPosition = 0.0f;
}