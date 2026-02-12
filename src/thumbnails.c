#include "include/main.h"
#include <limits.h>
void GenerateOrLoadThumbnail(MediaLibrary *lib, int index)
{
    if (lib->thumbnails[index].id != 0)
        return;
    char thumbPath[512];
    char safeName[128];
    snprintf(safeName, sizeof(safeName), "%s_%08x",
             GetFileNameWithoutExt(lib->paths[index]),
             simple_hash(lib->paths[index]));
    snprintf(thumbPath, sizeof(thumbPath), "/tmp/myview_%s.png", safeName);
    if (!FileExists(thumbPath))
    {
        char command[1024];
        snprintf(command, sizeof(command),
                 "ffmpeg -y -ss 00:00:05 -i \"%s\" -frames:v 1 -vf scale=320:-2 -q:v 5 \"%s\" -loglevel quiet",
                 lib->paths[index], thumbPath);
        TraceLog(LOG_INFO, "Creating thumbnail: %s", GetFileName(lib->paths[index]));
        int ret = system(command);
        if (ret != 0)
        {
            TraceLog(LOG_WARNING, "ffmpeg failed (code %d) for %s", ret, lib->paths[index]);
            return;
        }
    }
    if (FileExists(thumbPath))
    {
        lib->thumbnails[index] = LoadTexture(thumbPath);
        if (lib->thumbnails[index].id > 0)
        {
            TraceLog(LOG_INFO, "Thumbnail ready for %s", GetFileName(lib->paths[index]));
        }
        else
        {
            TraceLog(LOG_WARNING, "Could not load texture %s", thumbPath);
        }
    }
}
void RefreshThumbnails(MediaLibrary *lib)
{
    for (int i = 0; i < lib->count; i++)
    {
        GenerateOrLoadThumbnail(lib, i);
    }
}