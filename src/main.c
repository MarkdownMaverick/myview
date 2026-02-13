#include "rlgl.h"
#include "raylib.h"
#include "include/gl_compat.h"
#include "include/main.h"
#include "include/utils.h"
#include "include/settings.h"
#include "include/thumbnails.h"
#include "include/ui.h"
#include "include/player.h"
#include <math.h>
#include "raymath.h"
Texture2D TITLE_LOGO = {0};
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "My View");
    IsWindowState(FLAG_WINDOW_RESIZABLE);
    IsWindowState(FLAG_WINDOW_UNDECORATED);
    TITLE_LOGO = LoadTexture("gui/TITLE.png");
    AppState state = {0};
    state.currentScreen = STATE_BOOT_SCREEN;
    state.previousScreen = STATE_MAIN_MENU;
    state.isSeeking = false;
    state.lastMouseMoveTime = GetTime();
    state.seekBarAlpha = 0.0f;
    state.videoDuration = 0.0;
    state.videoTimePos = 0.0;
    state.seekProgress = 0.0f;
    state.selectedIndex = 0;
    state.gridScrollOffset = 0;
    state.showFavorites = false;
    state.showingOptions = false;
    state.optionsSelectedIndex = 0;
    state.confirmDelete = false;
    state.currentMediaIndex = -1;
    state.lastPreviewRefresh = 0.0;
    state.lastPreviewRefresh = 0.0;
    state.fadeAlpha = 1.0f;
    state.lastActivityTime = GetTime();
    state.logoVisibility = 1.0f;
    state.logoAnimatingOut = false;
    state.logoOffsetY = 0.0f;
    LoadSettings(&state);
    LoadUserSettings(&state);
    RefreshThumbnails(&state.movieLib);
    RefreshThumbnails(&state.videoLib);
    RefreshThumbnails(&state.musicLib);
    InitAudioDevice();
    SetTargetFPS(60);
    state.logoModel = LoadModel("gui/logo.glb");
    state.logoScale = 10.0f;
    state.logoPos = (Vector3){-0.2f, 1.0f, 2.6f};
    state.logoRotY = 0.0f;
    if (state.logoModel.meshCount > 0)
    {
        TraceLog(LOG_INFO, "3D logo loaded: %d meshes", state.logoModel.meshCount);
    }
    else
    {
        TraceLog(LOG_WARNING, "Failed to load gui/logo.glb");
    }
    double bootScreenStartTime = GetTime();
    FILE *ffmpeg = NULL;
    Image bootFrame = {0};
    Texture2D bootTexture = {0};
    bool bootScreenPlaying = false;
    state.bubbleFont = LoadFont("fonts/kavoon.ttf");
    SetTextureFilter(state.bubbleFont.texture, TEXTURE_FILTER_BILINEAR);
    if (FileExists("gui/boot.mp4"))
    {
        char ffmpegCmd[512];
        snprintf(ffmpegCmd, sizeof(ffmpegCmd),
                 "ffmpeg -i gui/boot.mp4 -f rawvideo -pix_fmt rgba -s %dx%d -", SCREEN_W, SCREEN_H);
        ffmpeg = popen(ffmpegCmd, "r");
        if (ffmpeg)
        {
            bootFrame = GenImageColor(SCREEN_W, SCREEN_H, BLACK);
            bootTexture = LoadTextureFromImage(bootFrame);
            bootScreenPlaying = true;
        }
        else
        {
            TraceLog(LOG_WARNING, "Failed to start FFmpeg for boot.mp4");
        }
    }
    if (FileExists("gui/background.png"))
    {
        state.backgroundTexture = LoadTexture("gui/background.png");
    }
    if (FileExists("gui/darktheme.png"))
    {
        state.darkThemeTexture = LoadTexture("gui/darktheme.png");
    }
    if (FileExists("sfx/startup.wav"))
    {
        state.startupSound = LoadSound("sfx/startup.wav");
    }
    if (FileExists("sfx/place.wav"))
    {
        state.selectSound = LoadSound("sfx/place.wav");
    }
    if (FileExists("sfx/back.wav"))
    {
        state.backSound = LoadSound("sfx/back.wav");
    }
    state.mpv = mpv_create();
    if (state.mpv)
    {
        mpv_set_option_string(state.mpv, "hwdec", "no");
        mpv_set_option_string(state.mpv, "terminal", "yes");
        mpv_set_option_string(state.mpv, "msg-level", "all=v");
        mpv_set_option_string(state.mpv, "vo", "libmpv");
        mpv_set_option_string(state.mpv, "hwdec-codecs", "all");
        mpv_set_option_string(state.mpv, "gpu-api", "opengl");
        mpv_set_option_string(state.mpv, "opengl-swapinterval", "0");
        mpv_set_option_string(state.mpv, "video-sync", "audio");
        mpv_set_option_string(state.mpv, "keep-open", "yes");
        if (mpv_initialize(state.mpv) < 0)
        {
            fprintf(stderr, "Error: mpv_initialize failed\n");
            mpv_destroy(state.mpv);
            state.mpv = NULL;
        }
        else
        {
            mpv_opengl_init_params gl_init = {.get_proc_address = get_rb_proc_address};
            mpv_render_param params[] = {
                {MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init},
                {MPV_RENDER_PARAM_INVALID, NULL}};
            int result = mpv_render_context_create(&state.mpv_ctx, state.mpv, params);
            if (result < 0)
            {
                fprintf(stderr, "Error: failed to create mpv render context: %s\n", mpv_error_string(result));
            }
            else
            {
                printf("✓ MPV initialized successfully\n");
            }
        }
    }
    else
    {
        fprintf(stderr, "Error: failed to create mpv instance\n");
    }
    while (!WindowShouldClose() && !state.quit)
    {
        if (IsKeyPressed(KEY_F11))
            ToggleFullscreen();
        if (state.currentScreen == STATE_BOOT_SCREEN)
        {
            double currentTime = GetTime();
            double elapsed = currentTime - bootScreenStartTime;
            if (elapsed >= 5.0 || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE))
            {
                state.currentScreen = STATE_MAIN_MENU;
                state.selectedIndex = 0;
                if (bootScreenPlaying)
                {
                    pclose(ffmpeg);
                    UnloadTexture(bootTexture);
                    UnloadImage(bootFrame);
                }
                if (state.startupSound.frameCount > 0)
                    PlaySound(state.startupSound);
            }
            else if (bootScreenPlaying)
            {
                size_t frameSize = SCREEN_W * SCREEN_H * 4;
                void *pixels = malloc(frameSize);
                if (pixels && fread(pixels, 1, frameSize, ffmpeg) == frameSize)
                {
                    UpdateTexture(bootTexture, pixels);
                }
                else
                {
                    TraceLog(LOG_WARNING, "Failed to read frame from FFmpeg; ending boot video");
                    bootScreenPlaying = false;
                }
                free(pixels);
            }
        }
        switch (state.currentScreen)
        {
        case STATE_MAIN_MENU:
            UpdateMainMenu(&state);
            break;
        case STATE_MUSIC:
            UpdateMediaGrid(&state, &state.musicLib);
            break;
        case STATE_MOVIES:
            UpdateMediaGrid(&state, &state.movieLib);
            break;
        case STATE_VIDEOS:
            UpdateMediaGrid(&state, &state.videoLib);
            break;
        case STATE_GENRE_SELECT:
            break;
        case STATE_SETTINGS:
            UpdateSettings(&state);
            break;
        case STATE_PLAYING:
            UpdatePlayer(&state);
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                state.currentScreen = state.previousScreen;
                state.selectedIndex = 0;
                int pause = 1;
                mpv_set_property(state.mpv, "pause", MPV_FORMAT_FLAG, &pause);
                const char *seekCmd[] = {"seek", "0", "absolute", NULL};
                mpv_command(state.mpv, seekCmd);
            }
            break;
        default:
            break;
        }
        if (state.mpv)
        {
            mpv_event *ev;
            while ((ev = mpv_wait_event(state.mpv, 0)))
            {
                if (ev->event_id == MPV_EVENT_NONE)
                    break;
                switch (ev->event_id)
                {
                case MPV_EVENT_END_FILE:
                {
                    mpv_event_end_file *ef = (mpv_event_end_file *)ev->data;
                    if (ef->reason == MPV_END_FILE_REASON_EOF)
                    {
                        TraceLog(LOG_INFO, "Video ended; returning to grid");
                        state.currentScreen = state.previousScreen;
                        state.selectedIndex = 0;
                    }
                    else if (ef->reason == MPV_END_FILE_REASON_ERROR)
                    {
                        TraceLog(LOG_ERROR, "Video playback error");
                    }
                }
                break;
                case MPV_EVENT_FILE_LOADED:
                    TraceLog(LOG_INFO, "Video loaded: %s", GetFileName(state.currentVideoPath));
                    mpv_get_property(state.mpv, "duration", MPV_FORMAT_DOUBLE, &state.videoDuration);
                    break;
                case MPV_EVENT_PLAYBACK_RESTART:
                    TraceLog(LOG_INFO, "Playback restarted (seek completed)");
                    break;
                case MPV_EVENT_SEEK:
                    TraceLog(LOG_INFO, "Seeking...");
                    break;
                case MPV_EVENT_LOG_MESSAGE:
                {
                    mpv_event_log_message *msg = (mpv_event_log_message *)ev->data;
                    if (strcmp(msg->level, "error") == 0)
                    {
                        TraceLog(LOG_ERROR, "[MPV] %s: %s", msg->prefix, msg->text);
                    }
                }
                break;
                case MPV_EVENT_SHUTDOWN:
                    TraceLog(LOG_INFO, "MPV shutdown requested");
                    break;
                default:
                    break;
                }
            }
        }
        bool userActive = false;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
            IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
            GetMouseDelta().x != 0 || GetMouseDelta().y != 0 ||
            IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
            IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
        {
            userActive = true;
        }
        if (userActive)
        {
            state.lastActivityTime = GetTime();
            state.logoAnimatingOut = false;
        }
        BeginDrawing();
        ClearBackground(GetColor(0x121212FF));
        if (state.currentScreen == STATE_BOOT_SCREEN)
        {
            if (bootScreenPlaying)
            {
                DrawTexture(bootTexture, 0, 0, WHITE);
            }
            else
            {
                float alpha = (sin(GetTime() * 4.0f) + 1.0f) / 2.0f;
                DrawText("Loading My View...", SCREEN_W / 2 - MeasureText("Loading My View...", 40) / 2,
                         SCREEN_H / 2 - 20, 40, Fade(WHITE, alpha));
            }
        }
        else
        {
            switch (state.currentScreen)
            {
            case STATE_MAIN_MENU:
                DrawMainMenu(&state);
                break;
            case STATE_MUSIC:
                DrawMediaGrid(&state, &state.musicLib, "Music Library");
                break;
            case STATE_MOVIES:
                DrawMediaGrid(&state, &state.movieLib, "Movie Library");
                break;
            case STATE_VIDEOS:
                DrawMediaGrid(&state, &state.videoLib, "Video Collection");
                break;
            case STATE_GENRE_SELECT:
                DrawGenreGrid(&state);
                break;
            case STATE_SETTINGS:
                DrawSettings(&state);
                break;
            case STATE_PLAYING:
                DrawPlayer(&state);
                break;
            default:
                break;
            }
        }
        EndDrawing();
    }
    MediaLibrary *libs[] = {&state.movieLib, &state.videoLib, &state.musicLib};
    for (int j = 0; j < 3; j++)
    {
        for (int i = 0; i < libs[j]->count; i++)
        {
            if (libs[j]->thumbnails[i].id > 0)
            {
                UnloadTexture(libs[j]->thumbnails[i]);
            }
        }
    }
    if (state.mpv_ctx)
        mpv_render_context_free(state.mpv_ctx);
    if (state.mpv)
        mpv_terminate_destroy(state.mpv);
    if (state.backgroundTexture.id > 0)
        UnloadTexture(state.backgroundTexture);
    if (state.darkThemeTexture.id > 0)
        UnloadTexture(state.darkThemeTexture);
    if (state.startupSound.frameCount > 0)
        UnloadSound(state.startupSound);
    if (state.selectSound.frameCount > 0)
        UnloadSound(state.selectSound);
    if (state.backSound.frameCount > 0)
        UnloadSound(state.backSound);
    UnloadTexture(TITLE_LOGO);
    UnloadModel(state.logoModel);
    UnloadFont(state.bubbleFont);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}