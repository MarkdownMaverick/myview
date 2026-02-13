#include "include/main.h"
#include <limits.h>
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