#include "include/main.h"
#include <limits.h>
#include <pthread.h>
#include <string.h>
void SaveSettings(AppState *g)
{
    cJSON *root = cJSON_CreateObject();
    if (!root)
    {
        fprintf(stderr, "Error: Failed to create JSON object\n");
        return;
    }
    const char *keys[] = {"movies", "videos", "music"};
    MediaLibrary *libs[] = {&g->movieLib, &g->videoLib, &g->musicLib};
    for (int j = 0; j < 3; j++)
    {
        cJSON *arr = cJSON_CreateArray();
        if (!arr)
            continue;
        for (int i = 0; i < libs[j]->count; i++)
        {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "path", libs[j]->paths[i]);
            cJSON_AddStringToObject(item, "genre", libs[j]->genres[i]);
            cJSON_AddBoolToObject(item, "favorite", libs[j]->isFavorite[i]);
            cJSON_AddItemToArray(arr, item);
        }
        cJSON_AddItemToObject(root, keys[j], arr);
    }
    char *rendered = cJSON_Print(root);
    if (rendered)
    {
        SaveFileText("settings.json", rendered);
        free(rendered);
    }
    cJSON_Delete(root);
}
void LoadSettings(AppState *g)
{
    if (!FileExists("settings.json"))
        return;
    char *data = LoadFileText("settings.json");
    if (!data)
    {
        fprintf(stderr, "Error: Failed to load settings.json\n");
        return;
    }
    cJSON *root = cJSON_Parse(data);
    if (!root)
    {
        fprintf(stderr, "Error: Failed to parse settings.json\n");
        UnloadFileText(data);
        return;
    }
    const char *keys[] = {"movies", "videos", "music"};
    MediaLibrary *libs[] = {&g->movieLib, &g->videoLib, &g->musicLib};
    for (int j = 0; j < 3; j++)
    {
        cJSON *arr = cJSON_GetObjectItem(root, keys[j]);
        if (arr && cJSON_IsArray(arr))
        {
            int size = cJSON_GetArraySize(arr);
            libs[j]->count = MIN(size, MAX_MEDIA_FILES);
            for (int i = 0; i < libs[j]->count; i++)
            {
                cJSON *item = cJSON_GetArrayItem(arr, i);
                if (item && cJSON_IsObject(item))
                {
                    cJSON *pathItem = cJSON_GetObjectItem(item, "path");
                    if (pathItem && cJSON_IsString(pathItem))
                    {
                        strncpy(libs[j]->paths[i], pathItem->valuestring, MAX_PATH_LENGTH - 1);
                        libs[j]->paths[i][MAX_PATH_LENGTH - 1] = '\0';
                    }
                    cJSON *favItem = cJSON_GetObjectItem(item, "favorite");
                    libs[j]->isFavorite[i] = favItem ? cJSON_IsTrue(favItem) : false;
                    cJSON *genreItem = cJSON_GetObjectItem(item, "genre");
                    if (genreItem && cJSON_IsString(genreItem))
                    {
                        strncpy(libs[j]->genres[i], genreItem->valuestring, 31);
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
    UnloadFileText(data);
}
void SaveUserSettings(AppState *g)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return;
    cJSON_AddNumberToObject(root, "theme", g->currentTheme);
    char *rendered = cJSON_Print(root);
    if (rendered)
    {
        SaveFileText("usersettings.json", rendered);
        free(rendered);
    }
    cJSON_Delete(root);
}
void LoadUserSettings(AppState *g)
{
    if (!FileExists("usersettings.json")) return;
    char *data = LoadFileText("usersettings.json");
    if (!data) return;
    cJSON *root = cJSON_Parse(data);
    if (!root) { UnloadFileText(data); return; }
    cJSON *themeItem = cJSON_GetObjectItem(root, "theme");
    if (themeItem && cJSON_IsNumber(themeItem))
        g->currentTheme = (int)themeItem->valuedouble;
    cJSON_Delete(root);
    UnloadFileText(data);
}
static const char *ImpGetFileName(const char *path)
{
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}
static void ImpGetBaseName(const char *path, char *buf, int bufLen)
{
    const char *name = ImpGetFileName(path);
    strncpy(buf, name, bufLen - 1);
    buf[bufLen - 1] = '\0';
    char *dot = strrchr(buf, '.');
    if (dot) *dot = '\0';
}
static bool ImpLibHasBaseName(MediaLibrary *lib, const char *baseName)
{
    for (int i = 0; i < lib->count; i++)
    {
        char existing[512];
        ImpGetBaseName(lib->paths[i], existing, sizeof(existing));
        if (strcasecmp(existing, baseName) == 0)
            return true;
    }
    return false;
}
static bool ImpLibAdd(MediaLibrary *lib, const char *path)
{
    if (lib->count >= MAX_MEDIA_FILES)
        return false;
    int idx = lib->count;
    strncpy(lib->paths[idx], path, MAX_PATH_LENGTH - 1);
    lib->paths[idx][MAX_PATH_LENGTH - 1] = '\0';
    lib->genres[idx][0]    = '\0';
    lib->isFavorite[idx]   = false;
    lib->thumbnails[idx]   = (Texture2D){0};
    lib->count++;
    return true;
}
static void *ImportThreadFn(void *arg)
{
    AppState *g = (AppState *)arg;
    const char *cmd =
        "find / "
        "\\( -path /proc -o -path /sys -o -path /dev "
        "   -o -path /run -o -path /snap \\) -prune "
        "-o -name '*.mp4' -type f -print 2>/dev/null "
        "| while IFS= read -r _f; do "
        "    _d=$(ffprobe -v error "
        "         -show_entries format=duration "
        "         -of default=noprint_wrappers=1:nokey=1 "
        "         \"$_f\" 2>/dev/null); "
        "    [ -n \"$_d\" ] && printf '%s\\t%s\\n' \"$_d\" \"$_f\"; "
        "done";
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        g->importRunning = false;
        g->importDone    = true;
        return NULL;
    }
    char line[MAX_PATH_LENGTH + 64];
    int  total = 0, added = 0, skipped = 0;
    while (fgets(line, sizeof(line), pipe))
    {
        line[strcspn(line, "\n")] = '\0';
        char *tab = strchr(line, '\t');
        if (!tab) continue;
        *tab = '\0';
        double      durSec  = atof(line);
        const char *path    = tab + 1;
        if (durSec <= 0.0 || strlen(path) == 0) continue;
        total++;
        g->importTotal = total;
        char baseName[512];
        ImpGetBaseName(path, baseName, sizeof(baseName));
        if (ImpLibHasBaseName(&g->movieLib, baseName) ||
            ImpLibHasBaseName(&g->videoLib, baseName) ||
            ImpLibHasBaseName(&g->musicLib, baseName))
        {
            skipped++;
            g->importSkipped = skipped;
            continue;
        }
        double durMin = durSec / 60.0;
        bool   ok     = false;
        if (durMin < 10.0)
            ok = ImpLibAdd(&g->musicLib, path);
        else if (durMin >= 60.0)
            ok = ImpLibAdd(&g->movieLib, path);
        else
            ok = ImpLibAdd(&g->videoLib, path);
        if (ok)
        {
            added++;
            g->importAdded = added;
        }
        else
        {
            skipped++;           
            g->importSkipped = skipped;
        }
    }
    pclose(pipe);
    g->importAdded   = added;
    g->importSkipped = skipped;
    g->importTotal   = total;
    SaveSettings(g);          
    g->importRunning = false;
    g->importDone    = true;  
    return NULL;
}
void ImportAllMp4(AppState *g)
{
    if (g->importRunning) return;   
    g->importRunning     = true;
    g->importDone        = false;
    g->importAdded       = 0;
    g->importSkipped     = 0;
    g->importTotal       = 0;
    g->importFinishedTime = 0.0;
    pthread_t tid;
    if (pthread_create(&tid, NULL, ImportThreadFn, g) != 0)
    {
        g->importRunning = false;
        g->importDone    = true;
        fprintf(stderr, "ImportAllMp4: pthread_create failed\n");
        return;
    }
    pthread_detach(tid);    
}