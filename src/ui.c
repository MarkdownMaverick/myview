#include "include/main.h"
#include "rlgl.h"
#include <math.h>
#include "raymath.h"

// ------------------------------------------------------------
// GetActiveGenreList:
//   Uses previousScreen so it works correctly when
//   currentScreen == STATE_GENRE_SELECT.
// ------------------------------------------------------------
static const char **GetActiveGenreList(AppState *g)
{
    if (g->previousScreen == STATE_MUSIC)
        return MUSIC_GENRE_LIST;
    return GENRE_LIST;
}

static int GetActiveGenreCount(AppState *g)
{
    const char **list = GetActiveGenreList(g);
    int count = 0;
    while (list[count] != NULL)
        count++;
    return count;
}

// Returns the actual library index for the Nth visible item under
// the current genre + favorites filter (used by the media grid).
int GetActualIndex(MediaLibrary *lib, int visibleIndex, AppState *g)
{
    const char **list = GetActiveGenreList(g);
    int count = 0;
    for (int i = 0; i < lib->count; i++)
    {
        bool genreMatch = (g->activeGenreIdx == 0) ||
                          (strstr(lib->genres[i], list[g->activeGenreIdx]) != NULL);
        bool favMatch = !g->showFavorites || lib->isFavorite[i];
        if (genreMatch && favMatch)
        {
            if (count == visibleIndex)
                return i;
            count++;
        }
    }
    return -1;
}

int GetVisibleCount(MediaLibrary *lib, AppState *g)
{
    const char **list = GetActiveGenreList(g);
    int count = 0;
    for (int i = 0; i < lib->count; i++)
    {
        bool genreMatch = (g->activeGenreIdx == 0) ||
                          (strstr(lib->genres[i], list[g->activeGenreIdx]) != NULL);
        bool favMatch = !g->showFavorites || lib->isFavorite[i];
        if (genreMatch && favMatch)
            count++;
    }
    return count;
}

static void ToggleGenreInBuffer(char *buf, const char *genre)
{
    if (strstr(buf, genre) != NULL)
    {
        char temp[256] = "";
        char copy[256];
        strncpy(copy, buf, 255);
        copy[255] = '\0';
        char *tok = strtok(copy, ", ");
        while (tok != NULL)
        {
            if (strcmp(tok, genre) != 0)
            {
                if (strlen(temp) > 0)
                    strcat(temp, ", ");
                strcat(temp, tok);
            }
            tok = strtok(NULL, ", ");
        }
        strncpy(buf, temp, 31);
        buf[31] = '\0';
    }
    else
    {
        size_t need = strlen(genre) + (strlen(buf) > 0 ? 2 : 0);
        if (strlen(buf) + need < 31)
        {
            if (strlen(buf) > 0)
                strncat(buf, ", ", 31 - strlen(buf));
            strncat(buf, genre, 31 - strlen(buf));
        }
    }
}

// ============================================================
//  DrawGenreGrid
//  Two modes:
//    isTaggingMode = true  -> tag genres onto a specific item
//    isTaggingMode = false -> filter the library by genre,
//                            matching media shown below cards
// ============================================================
void DrawGenreGrid(AppState *g)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    // Resolve library and genre list based on where we came from.
    MediaLibrary *lib = (g->previousScreen == STATE_MOVIES)   ? &g->movieLib
                      : (g->previousScreen == STATE_VIDEOS)   ? &g->videoLib
                                                              : &g->musicLib;
    const char **list       = GetActiveGenreList(g);
    int          genreCount = GetActiveGenreCount(g);

    // --------------------------------------------------------
    //  TAGGING MODE
    //  User navigates the grid, ENTER toggles a tag on the
    //  currently-edited media item.  BACKSPACE exits.
    // --------------------------------------------------------
    if (g->isTaggingMode)
    {
        char       *currentGenres = lib->genres[g->editingIndex];
        const char *movieName     = GetFileNameWithoutExt(lib->paths[g->editingIndex]);

        // Number of genres available for tagging (skip "All" at index 0).
        int tagCount = genreCount - 1;

        // --- Input ---
        if (IsKeyPressed(KEY_RIGHT))
        {
            g->genreSelectedIndex = (g->genreSelectedIndex + 1) % tagCount;
            PlaySound(g->selectSound);
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            g->genreSelectedIndex = (g->genreSelectedIndex - 1 + tagCount) % tagCount;
            PlaySound(g->selectSound);
        }
        if (IsKeyPressed(KEY_DOWN))
        {
            int next = g->genreSelectedIndex + 5;
            if (next < tagCount)
            {
                g->genreSelectedIndex = next;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_UP))
        {
            int prev = g->genreSelectedIndex - 5;
            if (prev >= 0)
            {
                g->genreSelectedIndex = prev;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_ENTER))
        {
            // list[0] == "All", so real genres start at list[1]
            int gi = g->genreSelectedIndex + 1;
            if (gi < genreCount)
            {
                ToggleGenreInBuffer(currentGenres, list[gi]);
                SaveSettings(g);
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            g->editingIndex   = -1;
            g->isTaggingMode  = false;
            g->currentScreen  = g->previousScreen;
            return;
        }

        // --- Drawing ---
        DrawRectangle(0, 0, (int)sw, (int)sh, (Color){18, 18, 24, 255});
        DrawRectangleGradientV(0, 0, (int)sw, 110, (Color){40, 20, 70, 255}, (Color){18, 18, 24, 0});
        DrawText("TAG GENRES", 40, 18, 34, GOLD);
        DrawText(movieName, 44, 58, 22, WHITE);
        DrawText("ARROWS: navigate  |  ENTER: toggle  |  BACKSPACE: done", 44, 84, 17, GRAY);

        int   tagCols    = 5;
        float tagPad     = 60.0f;
        float tagSpacing = 18.0f;
        float boxW = (sw - tagPad * 2.0f - tagSpacing * (tagCols - 1)) / tagCols;
        float boxH = 80.0f;
        float gridStartY = 120.0f;

        for (int i = 0; i < tagCount; i++)
        {
            int gi  = i + 1; // skip "All"
            int col = i % tagCols;
            int row = i / tagCols;
            Rectangle box = {
                tagPad + col * (boxW + tagSpacing),
                gridStartY + row * (boxH + tagSpacing),
                boxW, boxH};

            bool isTagged   = strstr(currentGenres, list[gi]) != NULL;
            bool isSelected = (i == g->genreSelectedIndex);
            bool isHovered  = CheckCollisionPointRec(mouse, box);

            if (isHovered && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0))
                g->genreSelectedIndex = i;

            Color bg = isTagged     ? (Color){20, 70, 30, 255}
                     : isSelected   ? (Color){50, 50, 90, 255}
                     : isHovered    ? (Color){55, 55, 60, 255}
                                    : (Color){38, 38, 44, 255};
            DrawRectangleRounded(box, 0.22f, 8, bg);

            float thick  = (isSelected || isTagged) ? 3.0f : 1.5f;
            Color border = isTagged     ? GREEN
                         : isSelected   ? YELLOW
                         : isHovered    ? SKYBLUE
                                        : (Color){80, 80, 90, 255};
            DrawRectangleRoundedLinesEx(box, 0.22f, 8, thick, border);

            int fsize = 19;
            int tw    = MeasureText(list[gi], fsize);
            DrawText(list[gi],
                     (int)(box.x + box.width  / 2 - tw    / 2),
                     (int)(box.y + box.height / 2 - fsize / 2),
                     fsize,
                     isTagged ? GREEN : WHITE);

            if (isTagged)
                DrawText("v", (int)(box.x + box.width - 24), (int)(box.y + 6), 18, GREEN);

            if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                g->genreSelectedIndex = i;
                ToggleGenreInBuffer(currentGenres, list[gi]);
                SaveSettings(g);
                PlaySound(g->selectSound);
            }
        }

        // Bottom bar showing current tags
        DrawRectangleGradientV(0, (int)(sh - 60), (int)sw, 60,
                               (Color){18, 18, 24, 0}, (Color){18, 18, 24, 255});
        DrawText("Tagged:", 40, (int)(sh - 40), 20, LIGHTGRAY);
        const char *tagDisplay = (strlen(currentGenres) > 0) ? currentGenres : "(none)";
        DrawText(tagDisplay, 120, (int)(sh - 40), 20, GOLD);
        return;
    }

    // --------------------------------------------------------
    //  FILTER MODE
    //  Top half: genre card grid.
    //  Bottom half: media items matching the active genre.
    //  Focus toggles between the two sections.
    // --------------------------------------------------------

    // --- Layout constants ---
    int   gCols    = 5;
    float pad      = 40.0f;
    float gSpacing = 12.0f;
    float topH     = 90.0f;
    float cardW    = (sw - pad * 2.0f - gSpacing * (gCols - 1)) / gCols;
    float cardH    = 62.0f;
    int   genreRows = (genreCount + gCols - 1) / gCols;
    float genreSectionBottom = topH + genreRows * (cardH + gSpacing);
    float sepY       = genreSectionBottom + 6.0f;
    float mediaStartY = sepY + 46.0f;

    int   mCols    = 5;
    float mSpacing = 14.0f;
    float thumbW   = (sw - pad * 2.0f - mSpacing * (mCols - 1)) / mCols;
    float thumbH   = thumbW * 0.5625f;

    // Count items visible under the active genre filter
    int filteredCount = 0;
    for (int i = 0; i < lib->count; i++)
    {
        bool match = (g->activeGenreIdx == 0) ||
                     (strstr(lib->genres[i], list[g->activeGenreIdx]) != NULL);
        if (match) filteredCount++;
    }

    // --- Input: genre card section ---
    if (!g->genreMediaFocus)
    {
        if (IsKeyPressed(KEY_RIGHT))
        {
            g->genreSelectedIndex = (g->genreSelectedIndex + 1) % genreCount;
            PlaySound(g->selectSound);
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            g->genreSelectedIndex = (g->genreSelectedIndex - 1 + genreCount) % genreCount;
            PlaySound(g->selectSound);
        }
        if (IsKeyPressed(KEY_DOWN))
        {
            int next = g->genreSelectedIndex + gCols;
            if (next < genreCount)
            {
                g->genreSelectedIndex = next;
                PlaySound(g->selectSound);
            }
            else if (filteredCount > 0)
            {
                // Cross into the media section
                g->genreMediaFocus            = true;
                g->genreFilteredSelectedIndex = 0;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_UP))
        {
            int prev = g->genreSelectedIndex - gCols;
            if (prev >= 0)
            {
                g->genreSelectedIndex = prev;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_ENTER))
        {
            g->activeGenreIdx             = g->genreSelectedIndex;
            g->genreFilteredSelectedIndex = 0;
            // Recount after changing the filter
            filteredCount = 0;
            for (int i = 0; i < lib->count; i++)
            {
                bool match = (g->activeGenreIdx == 0) ||
                             (strstr(lib->genres[i], list[g->activeGenreIdx]) != NULL);
                if (match) filteredCount++;
            }
            if (filteredCount > 0)
            {
                g->genreMediaFocus = true;
            }
            PlaySound(g->selectSound);
        }
    }
    else // genreMediaFocus == true
    {
        // --- Input: media panel ---
        if (IsKeyPressed(KEY_RIGHT))
        {
            if (g->genreFilteredSelectedIndex < filteredCount - 1)
            {
                g->genreFilteredSelectedIndex++;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            if (g->genreFilteredSelectedIndex > 0)
            {
                g->genreFilteredSelectedIndex--;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_DOWN))
        {
            int next = g->genreFilteredSelectedIndex + mCols;
            if (next < filteredCount)
            {
                g->genreFilteredSelectedIndex = next;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_UP))
        {
            int prev = g->genreFilteredSelectedIndex - mCols;
            if (prev >= 0)
            {
                g->genreFilteredSelectedIndex = prev;
                PlaySound(g->selectSound);
            }
            else
            {
                // Cross back into the genre card section
                g->genreMediaFocus = false;
                PlaySound(g->selectSound);
            }
        }
        if (IsKeyPressed(KEY_ENTER) && filteredCount > 0)
        {
            // Find the actual library index for the selected filtered item
            int visIdx = 0;
            for (int i = 0; i < lib->count; i++)
            {
                bool match = (g->activeGenreIdx == 0) ||
                             (strstr(lib->genres[i], list[g->activeGenreIdx]) != NULL);
                if (!match) continue;
                if (visIdx == g->genreFilteredSelectedIndex)
                {
                    g->selectedIndex    = i;
                    g->currentMediaIndex = i;
                    g->showingOptions   = true;
                    g->optionsSelectedIndex = 0;
                    g->currentScreen    = g->previousScreen;
                    PlaySound(g->selectSound);
                    return;
                }
                visIdx++;
            }
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        g->currentScreen   = g->previousScreen;
        g->genreMediaFocus = false;
        PlaySound(g->backSound);
        return;
    }

    // ---- Drawing ----

    DrawRectangle(0, 0, (int)sw, (int)sh, (Color){14, 14, 20, 255});
    DrawRectangleGradientV(0, 0, (int)sw, (int)topH,
                           (Color){30, 10, 60, 255}, (Color){14, 14, 20, 0});
    DrawText("SELECT GENRE", (int)pad, 22, 40, GOLD);

    const char *hint = g->genreMediaFocus
        ? "UP: back to genres  |  ARROWS: navigate  |  ENTER: open  |  BACKSPACE: back"
        : "ARROWS: navigate  |  ENTER: select  |  DOWN: browse media  |  BACKSPACE: back";
    DrawText(hint, (int)pad, 62, 18, GRAY);

    // Genre cards
    for (int i = 0; i < genreCount; i++)
    {
        int col = i % gCols;
        int row = i / gCols;
        Rectangle card = {
            pad + col * (cardW + gSpacing),
            topH + row * (cardH + gSpacing),
            cardW, cardH};

        bool isSelected = (i == g->genreSelectedIndex) && !g->genreMediaFocus;
        bool isHovered  = CheckCollisionPointRec(mouse, card);
        bool isActive   = (i == g->activeGenreIdx);

        // Mouse hover updates keyboard cursor when focus is in genre section
        if (isHovered && !g->genreMediaFocus &&
            (GetMouseDelta().x != 0 || GetMouseDelta().y != 0))
            g->genreSelectedIndex = i;

        // Item count for this genre
        int cnt = 0;
        if (i == 0)
        {
            cnt = lib->count;
        }
        else
        {
            for (int m = 0; m < lib->count; m++)
                if (strstr(lib->genres[m], list[i]) != NULL)
                    cnt++;
        }

        Color bg = isSelected  ? (Color){60, 30, 100, 255}
                 : isActive    ? (Color){30, 60, 30,  255}
                 : isHovered   ? (Color){45, 45, 55,  255}
                               : (Color){28, 28, 36,  255};
        DrawRectangleRounded(card, 0.2f, 10, bg);

        // Selection glow
        if (isSelected)
        {
            for (int g2 = 1; g2 <= 4; g2++)
            {
                Rectangle glow = {card.x - g2 * 2, card.y - g2 * 2,
                                  card.width  + g2 * 4,
                                  card.height + g2 * 4};
                DrawRectangleRoundedLinesEx(glow, 0.2f, 10, 1.5f,
                                           Fade(PURPLE, 0.18f / g2));
            }
        }

        float thick  = isSelected ? 3.0f : isActive ? 2.5f : 1.5f;
        Color border = isActive     ? GREEN
                     : isSelected   ? YELLOW
                     : isHovered    ? SKYBLUE
                                    : (Color){70, 70, 85, 255};
        DrawRectangleRoundedLinesEx(card, 0.2f, 10, thick, border);

        // Genre name
        int fsize = 22;
        int tw    = MeasureText(list[i], fsize);
        DrawText(list[i],
                 (int)(card.x + card.width  / 2 - tw    / 2),
                 (int)(card.y + card.height / 2 - fsize / 2 - 8),
                 fsize,
                 isActive ? GREEN : WHITE);

        // Item count badge
        char badge[16];
        snprintf(badge, sizeof(badge), "%d item%s", cnt, cnt == 1 ? "" : "s");
        int bw = MeasureText(badge, 14);
        DrawText(badge,
                 (int)(card.x + card.width / 2 - bw / 2),
                 (int)(card.y + card.height / 2 + 8),
                 14,
                 cnt > 0 ? LIGHTGRAY : DARKGRAY);

        if (isActive)
            DrawText("* ACTIVE", (int)(card.x + 8), (int)(card.y + 5), 13, GREEN);

        // Mouse click selects genre and moves focus to media
        if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            g->activeGenreIdx             = i;
            g->genreSelectedIndex         = i;
            g->genreFilteredSelectedIndex = 0;
            g->genreMediaFocus            = true;
            PlaySound(g->selectSound);
        }
    }

    // ---- Separator between genre cards and media results ----
    DrawLineEx((Vector2){pad, sepY}, (Vector2){sw - pad, sepY}, 1.5f,
               (Color){80, 80, 110, 255});

    // Active filter label
    char filterLabel[80];
    if (g->activeGenreIdx == 0)
        snprintf(filterLabel, sizeof(filterLabel), "All  (%d item%s)",
                 lib->count, lib->count == 1 ? "" : "s");
    else
        snprintf(filterLabel, sizeof(filterLabel), "%s  (%d item%s)",
                 list[g->activeGenreIdx],
                 filteredCount, filteredCount == 1 ? "" : "s");

    Color labelColor = g->genreMediaFocus ? GOLD : LIGHTGRAY;
    DrawText(filterLabel, (int)pad, (int)(sepY + 8), 22, labelColor);

    if (filteredCount == 0 && g->activeGenreIdx != 0)
    {
        DrawText("No media tagged with this genre.",
                 (int)(sw / 2 - 200), (int)(mediaStartY + 30), 26, DARKGRAY);
        return;
    }

    // ---- Filtered media thumbnails ----
    int visIdx = 0;
    for (int i = 0; i < lib->count; i++)
    {
        bool match = (g->activeGenreIdx == 0) ||
                     (strstr(lib->genres[i], list[g->activeGenreIdx]) != NULL);
        if (!match) continue;

        // Lazy-load thumbnail
        if (lib->thumbnails[i].id == 0)
            GenerateOrLoadThumbnail(lib, i);

        int col  = visIdx % mCols;
        int row  = visIdx / mCols;
        float ty = mediaStartY + row * (thumbH + mSpacing);

        // Simple vertical cull — skip rows below the screen
        if (ty > sh + thumbH) { visIdx++; continue; }

        Rectangle thumb = {pad + col * (thumbW + mSpacing), ty, thumbW, thumbH};

        bool isMediaSelected = (visIdx == g->genreFilteredSelectedIndex) && g->genreMediaFocus;
        bool isHovered       = CheckCollisionPointRec(mouse, thumb);

        DrawRectangleRounded(thumb, 0.12f, 8, DARKGRAY);

        if (lib->thumbnails[i].id > 0)
        {
            DrawTexturePro(lib->thumbnails[i],
                           (Rectangle){0, 0,
                                       (float)lib->thumbnails[i].width,
                                       (float)lib->thumbnails[i].height},
                           thumb, (Vector2){0, 0}, 0.0f, WHITE);
        }
        else
        {
            DrawText("...",
                     (int)(thumb.x + thumbW / 2 - 20),
                     (int)(thumb.y + thumbH / 2 - 10),
                     20, LIGHTGRAY);
        }

        // Name overlay gradient
        DrawRectangleGradientV((int)thumb.x, (int)(thumb.y + thumbH - 36),
                               (int)thumbW, 36,
                               (Color){0, 0, 0, 0}, (Color){0, 0, 0, 210});
        const char *name = GetFileNameWithoutExt(lib->paths[i]);
        DrawText(name,
                 (int)(thumb.x + 6),
                 (int)(thumb.y + thumbH - 24),
                 16, WHITE);

        // Highlight selected / hovered
        if (isMediaSelected)
        {
            DrawRectangleRoundedLinesEx(thumb, 0.12f, 8, 3.5f, YELLOW);
            for (int g2 = 1; g2 <= 3; g2++)
            {
                Rectangle glow = {thumb.x - g2 * 2, thumb.y - g2 * 2,
                                  thumb.width  + g2 * 4,
                                  thumb.height + g2 * 4};
                DrawRectangleRoundedLinesEx(glow, 0.12f, 8, 1.5f,
                                           Fade(YELLOW, 0.25f / g2));
            }
            // "Press ENTER" hint on selected card
            DrawText("[ENTER]",
                     (int)(thumb.x + thumbW / 2 - MeasureText("[ENTER]", 14) / 2),
                     (int)(thumb.y + 6), 14, Fade(YELLOW, 0.9f));
        }
        else if (isHovered)
        {
            DrawRectangleRoundedLinesEx(thumb, 0.12f, 8, 2.0f, SKYBLUE);
        }

        // Favourite star indicator
        if (lib->isFavorite[i])
            DrawText("*", (int)(thumb.x + thumbW - 20), (int)(thumb.y + 5), 20, GOLD);

        // Mouse: single click = select & focus; double-click handled via quick reclick
        if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (isMediaSelected)
            {
                // Already selected → open options back in the grid
                g->selectedIndex     = i;
                g->currentMediaIndex = i;
                g->showingOptions    = true;
                g->optionsSelectedIndex = 0;
                g->currentScreen     = g->previousScreen;
                PlaySound(g->selectSound);
                return;
            }
            g->genreFilteredSelectedIndex = visIdx;
            g->genreMediaFocus            = true;
            PlaySound(g->selectSound);
        }

        visIdx++;
    }
}

// ============================================================
void DrawMainMenu(AppState *g)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (g->backgroundTexture.id > 0)
    {
        DrawTexturePro(g->backgroundTexture,
                       (Rectangle){0, 0,
                                   (float)g->backgroundTexture.width,
                                   (float)g->backgroundTexture.height},
                       (Rectangle){0, 0, sw, sh}, (Vector2){0, 0}, 0.0f, WHITE);
    }
    else
    {
        ClearBackground(GetColor(0x0f0f1aff));
    }
    const float TOP_BAR_HEIGHT = 120.0f;
    DrawRectangleGradientV(0, 0, sw, TOP_BAR_HEIGHT,
                           Fade(RED, 0.45f), Fade(GRAY, 0.10f));
    if (TITLE_LOGO.id > 0)
    {
        float logoScale = 0.5f;
        float logoW     = TITLE_LOGO.width * logoScale;
        Vector2 logoPos = {(sw / 2.0f) - (logoW / 2.0f), 20.0f};
        DrawTextureEx(TITLE_LOGO, logoPos, 0.0f, logoScale, WHITE);
    }
    bool userActive = (GetMouseDelta().x != 0 || GetMouseDelta().y != 0 ||
                       IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                       IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                       IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
                       IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE));
    if (userActive)
    {
        g->lastActivityTime  = GetTime();
        g->logoAnimatingOut  = false;
    }
    bool  shouldHide  = (GetTime() - g->lastActivityTime > 10.0);
    float targetVis   = shouldHide ? 0.0f : 1.0f;
    float speed       = shouldHide ? 0.4f : 5.0f;
    g->logoVisibility = Lerp(g->logoVisibility, targetVis, GetFrameTime() * speed);
    float visibleY    = 1.0f;
    float hiddenY     = 0.3f;
    float animatedY   = Lerp(hiddenY, visibleY, g->logoVisibility);
    g->logoPos.y      = animatedY;
    float animatedScale = Lerp(g->logoScale * 0.85f, g->logoScale, g->logoVisibility);
    Camera3D logoCamera = {0};
    logoCamera.position   = (Vector3){0.0f, 2.0f, 8.0f};
    logoCamera.target     = (Vector3){0.0f, 1.0f, 0.0f};
    logoCamera.up         = (Vector3){0.0f, 1.0f, 0.0f};
    logoCamera.fovy       = 5.0f;
    logoCamera.projection = CAMERA_PERSPECTIVE;
    if (g->logoVisibility > 0.05f)
        g->logoRotY += GetFrameTime() * 30.0f;
    BeginMode3D(logoCamera);
    DrawModelEx(g->logoModel, g->logoPos,
                (Vector3){0.0f, 1.0f, 0.0f}, g->logoRotY,
                (Vector3){animatedScale, animatedScale, animatedScale},
                Fade(WHITE, g->logoVisibility));
    EndMode3D();
    const char *items[] = {"Movies", "TV Shows", "Music", "Settings", "Exit"};
    int   n        = 5;
    float buttonW  = 360.0f;
    float buttonH  = 80.0f;
    float startY   = 220.0f;
    float spacingY = 20.0f;
    float leftMargin = 60.0f;
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < n; i++)
    {
        Rectangle btn = {leftMargin, startY + i * (buttonH + spacingY), buttonW, buttonH};
        bool hovered  = CheckCollisionPointRec(mouse, btn);
        bool selected = (i == g->selectedIndex);
        Color colorSelected = {218, 165, 32, 240};
        Color colorHovered  = {184, 134, 11, 200};
        Color colorDefault  = {139, 90,  43, 160};
        Color bg = selected ? colorSelected : hovered ? colorHovered : colorDefault;
        float waveOffset = sinf(GetTime() * 2.0f + i * 0.5f) * 1.0f;
        Rectangle waveBtn = btn;
        waveBtn.y += waveOffset;
        DrawRectangleRounded(waveBtn, 0.25f, 12, bg);
        if (selected || hovered)
            DrawRectangleRoundedLinesEx(waveBtn, 0.25f, 12, 4.0f,
                                        selected ? YELLOW : WHITE);
        int fontSz = 38;
        int txtW   = MeasureText(items[i], fontSz);
        DrawText(items[i],
                 (int)(waveBtn.x + waveBtn.width  / 2 - txtW  / 2),
                 (int)(waveBtn.y + waveBtn.height / 2 - fontSz / 2 - 8),
                 fontSz, WHITE);
    }
    MediaLibrary *previewLib  = NULL;
    const char   *previewTitle = NULL;
    switch (g->selectedIndex)
    {
    case 0: previewLib = &g->movieLib; previewTitle = "Movies";   break;
    case 1: previewLib = &g->videoLib; previewTitle = "TV Shows"; break;
    case 2: previewLib = &g->musicLib; previewTitle = "Music";    break;
    default: break;
    }
    if (previewLib && previewLib->count > 0)
    {
        Rectangle rightPanel = {sw * 0.45f, 140, sw * 0.55f - 40, sh - 100};
        DrawRectangleRounded(rightPanel, 0.12f, 100, Fade(BLACK, 0.68f));
        DrawText(previewTitle, (int)(rightPanel.x + 30), (int)rightPanel.y + 20, 42, GOLD);
        int numToShow = MIN(6, previewLib->count);
        double now = GetTime();
        if (now - g->lastPreviewRefresh >= 5.0 || g->lastPreviewRefresh == 0.0)
        {
            g->lastPreviewRefresh = now;
            g->fadeAlpha          = 0.0f;
            bool used[256] = {0};
            for (int i = 0; i < numToShow; i++)
            {
                int candidate;
                do { candidate = GetRandomValue(0, previewLib->count - 1); }
                while (used[candidate]);
                g->previewIndices[i] = candidate;
                used[candidate]      = true;
            }
        }
        if (g->fadeAlpha < 1.0f)
        {
            g->fadeAlpha += GetFrameTime() * 1.8f;
            if (g->fadeAlpha > 1.0f) g->fadeAlpha = 1.0f;
        }
        float paddingX = 40.0f;
        float pSpacing = 20.0f;
        int   cols = 2, rows = 3;
        float thumbW = (rightPanel.width - (paddingX * 2) - (pSpacing * (cols - 1))) / cols;
        float thumbH = thumbW * 0.5625f;
        float startX = rightPanel.x + paddingX;
        float startY = rightPanel.y + 90;
        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                int idx = row * cols + col;
                if (idx >= numToShow)
                {
                    Rectangle er = {startX + col * (thumbW + pSpacing),
                                    startY + row * (thumbH + pSpacing),
                                    thumbW, thumbH};
                    DrawRectangleRounded(er, 0.18f, 10, BLACK);
                    continue;
                }
                int mediaIndex = g->previewIndices[idx];
                if (previewLib->thumbnails[mediaIndex].id == 0)
                    GenerateOrLoadThumbnail(previewLib, mediaIndex);
                Rectangle r = {startX + col * (thumbW + pSpacing),
                                startY + row * (thumbH + pSpacing),
                                thumbW, thumbH};
                DrawRectangleRounded(r, 0.18f, 10, BLACK);
                if (previewLib->thumbnails[mediaIndex].id > 0)
                {
                    DrawTexturePro(previewLib->thumbnails[mediaIndex],
                                   (Rectangle){0, 0,
                                               (float)previewLib->thumbnails[mediaIndex].width,
                                               (float)previewLib->thumbnails[mediaIndex].height},
                                   r, (Vector2){0, 0}, 0.0f,
                                   Fade(WHITE, g->fadeAlpha));
                }
                const char *name = GetFileNameWithoutExt(previewLib->paths[mediaIndex]);
                DrawText(name,
                         (int)(r.x + 12),
                         (int)(r.y + thumbH - 34),
                         19, Fade(WHITE, g->fadeAlpha * 0.9f));
            }
        }
    }
}

void UpdateMainMenu(AppState *g)
{
    Vector2 mouse = GetMousePosition();
    const char *items[] = {"Movies", "TV Shows", "Music", "Settings", "Exit"};
    (void)items;
    int   n  = 5;
    float bw = 320, bh = 70, sy = 280, sp = 25;
    if (g->selectedIndex < 0 || g->selectedIndex >= n)
        g->selectedIndex = 0;
    if (IsKeyPressed(KEY_DOWN))
    {
        g->selectedIndex++;
        if (g->selectedIndex >= n) g->selectedIndex = n - 1;
        PlaySound(g->selectSound);
    }
    if (IsKeyPressed(KEY_UP))
    {
        g->selectedIndex--;
        if (g->selectedIndex < 0) g->selectedIndex = 0;
        PlaySound(g->selectSound);
    }
    if (IsKeyPressed(KEY_ENTER))
    {
        switch (g->selectedIndex)
        {
        case 0: g->currentScreen = STATE_MOVIES;   g->selectedIndex = 0; break;
        case 1: g->currentScreen = STATE_VIDEOS;   g->selectedIndex = 0; break;
        case 2: g->currentScreen = STATE_MUSIC;    g->selectedIndex = 0; break;
        case 3: g->currentScreen = STATE_SETTINGS; break;
        case 4: g->quit = true; break;
        }
        if (g->selectSound.frameCount > 0) PlaySound(g->selectSound);
        return;
    }
    for (int i = 0; i < n; i++)
    {
        Rectangle r = {GetScreenWidth() / 2.0f - bw / 2, sy + i * (bh + sp), bw, bh};
        if (CheckCollisionPointRec(mouse, r))
            g->selectedIndex = i;
        if (CheckCollisionPointRec(mouse, r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            switch (i)
            {
            case 0: g->currentScreen = STATE_MOVIES;   g->selectedIndex = 0; break;
            case 1: g->currentScreen = STATE_VIDEOS;   g->selectedIndex = 0; break;
            case 2: g->currentScreen = STATE_MUSIC;    g->selectedIndex = 0; break;
            case 3: g->currentScreen = STATE_SETTINGS; break;
            case 4: g->quit = true; break;
            }
            if (g->selectSound.frameCount > 0) PlaySound(g->selectSound);
        }
    }
}

int GetFilteredCount(MediaLibrary *lib, const char *targetGenre)
{
    if (strcmp(targetGenre, "All") == 0) return lib->count;
    int count = 0;
    for (int i = 0; i < lib->count; i++)
        if (strcmp(lib->genres[i], targetGenre) == 0)
            count++;
    return count;
}

void DrawMediaGrid(AppState *g, MediaLibrary *lib, const char *title)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    const float TOP_BAR_HEIGHT = 100.0f;
    const float GRID_START_Y   = TOP_BAR_HEIGHT + 10.0f;
    int   columns  = 4;
    float padding  = 30.0f;
    float cardW    = (sw - (padding * (columns + 1))) / columns;
    float cardH    = cardW * 0.5625f;
    float rowHeight = cardH + padding;
    rlPushMatrix();
    rlTranslatef(0, -g->gridScrollOffset, 0);
    float startX = padding;
    float startY = GRID_START_Y;
    int firstVisibleRow = (int)floor(g->gridScrollOffset / rowHeight) - 1;
    int lastVisibleRow  = (int)ceil((g->gridScrollOffset + sh) / rowHeight) + 1;
    int visibleStart    = MAX(0, firstVisibleRow * columns);
    int visibleEnd      = MIN(lib->count - 1, lastVisibleRow * columns + columns - 1);
    for (int i = visibleStart; i <= visibleEnd && i < lib->count; i++)
    {
        if (lib->thumbnails[i].id == 0)
            GenerateOrLoadThumbnail(lib, i);
        int   col = i % columns;
        int   row = i / columns;
        float x   = startX + col * (cardW + padding);
        float y   = startY + row * rowHeight;
        Rectangle card = {x, y, cardW, cardH};
        bool isSelected = (i == g->selectedIndex);
        bool isHovered  = CheckCollisionPointRec(GetMousePosition(), card);
        Color bg = isSelected ? MAROON : (isHovered ? DARKGRAY : GRAY);
        DrawRectangleRounded(card, 0.15f, 8, bg);
        DrawRectangleRoundedLines(card, 0.15f, 8, isSelected ? YELLOW : WHITE);
        if (lib->thumbnails[i].id > 0)
        {
            DrawTexturePro(lib->thumbnails[i],
                           (Rectangle){0, 0,
                                       (float)lib->thumbnails[i].width,
                                       (float)lib->thumbnails[i].height},
                           card, (Vector2){0, 0}, 0, WHITE);
        }
        else
        {
            DrawRectangleRec(card, DARKGRAY);
            DrawText("...",
                     (int)(card.x + cardW / 2 - 20),
                     (int)(card.y + cardH / 2 - 20),
                     40, WHITE);
        }
        const char *displayName = GetFileNameWithoutExt(lib->paths[i]);
        float     fontSize = 32.0f;
        Vector2   textPos  = {card.x + 25, card.y + cardH - 45};
        Vector2   shadowPos = {textPos.x + 2, textPos.y + 2};
        DrawTextEx(g->bubbleFont, displayName, shadowPos, fontSize, 1.5f, BLACK);
        DrawTextEx(g->bubbleFont, displayName, textPos,   fontSize, 1.5f, WHITE);
    }
    rlPopMatrix();
    DrawRectangle(0, 0, sw, TOP_BAR_HEIGHT, Fade(BLACK, 0.88f));
    DrawRectangleGradientV(0, TOP_BAR_HEIGHT - 30, sw, 30,
                           Fade(BLACK, 0.5f), Fade(BLUE, 0.0f));
    DrawText(title, padding, 20, 50, GOLD);
    Rectangle genreBtn = {sw - 810, 25, 180, 50};
    bool genreHover  = CheckCollisionPointRec(GetMousePosition(), genreBtn);
    bool genreActive = (g->activeGenreIdx != 0);
    DrawRectangleRounded(genreBtn, 0.3f, 10,
                         genreActive ? (genreHover ? LIME : GREEN)
                                     : (genreHover ? GOLD : ORANGE));
    if (genreActive)
    {
        // Determine the right list for the current screen
        const char **activeList = (g->currentScreen == STATE_MUSIC)
                                  ? MUSIC_GENRE_LIST : GENRE_LIST;
        char genreLabel[32];
        snprintf(genreLabel, sizeof(genreLabel), "%.14s", activeList[g->activeGenreIdx]);
        int glw = MeasureText(genreLabel, 20);
        DrawText(genreLabel,
                 (int)(genreBtn.x + genreBtn.width / 2 - glw / 2),
                 (int)(genreBtn.y + 15), 20, BLACK);
    }
    else
    {
        DrawText("GENRE", (int)(genreBtn.x + 30), (int)(genreBtn.y + 15), 30, BLACK);
    }
    Rectangle favBtn = {sw - 410, 25, 180, 50};
    bool favHover = CheckCollisionPointRec(GetMousePosition(), favBtn);
    DrawRectangleRounded(favBtn, 0.3f, 10, favHover ? GOLD : ORANGE);
    DrawText("FAVORITES", (int)(favBtn.x + 20), (int)(favBtn.y + 15), 30, BLACK);
    Rectangle addBtn = {sw - 200, 25, 180, 50};
    bool addHover = CheckCollisionPointRec(GetMousePosition(), addBtn);
    DrawRectangleRounded(addBtn, 0.3f, 10, addHover ? LIME : GREEN);
    DrawText("+ ADD", (int)(addBtn.x + 40), (int)(addBtn.y + 15), 30, BLACK);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, favBtn))
        {
            // TODO: toggle favorites filter
        }
        if (CheckCollisionPointRec(mouse, addBtn))
        {
            char *path = OpenFileDialog();
            if (path && IsValidMediaFile(path) && lib->count < MAX_MEDIA_FILES)
            {
                strncpy(lib->paths[lib->count], path, MAX_PATH_LENGTH - 1);
                lib->paths[lib->count][MAX_PATH_LENGTH - 1] = '\0';
                lib->count++;
                GenerateOrLoadThumbnail(lib, lib->count - 1);
                SaveSettings(g);
                g->selectedIndex = lib->count - 1;
                int   newRow    = (lib->count - 1) / columns;
                float targetY   = newRow * rowHeight;
                g->gridScrollOffset = MAX(0.0f, targetY - sh * 0.3f);
            }
        }
    }
    if (lib->count > columns * 3)
        DrawText("Use arrow keys / mouse wheel to scroll",
                 padding, sh - 40, 20, LIGHTGRAY);
    if (lib->count == 0)
        DrawText("No media here yet... press BACKSPACE to return",
                 sw / 2 - 200, sh / 2 - 20, 30, LIGHTGRAY);
    if (g->showingOptions && g->currentMediaIndex >= 0 && g->currentMediaIndex < lib->count)
    {
        Rectangle popupRect = {sw / 2 - 150, sh / 2 - 100, 300, 333};
        DrawRectangleRounded(popupRect, 0.2f, 10, Fade(BLACK,      0.4f));
        DrawRectangleRounded(popupRect, 0.2f, 10, Fade(DARKPURPLE, 0.15f));
        DrawRectangleRounded(popupRect, 0.2f, 10, Fade(DARKGRAY,   0.92f));
        for (int i = 1; i <= 5; i++)
        {
            Rectangle glowRect = {popupRect.x - i * 2.0f, popupRect.y - i * 2.0f,
                                  popupRect.width  + i * 4.0f,
                                  popupRect.height + i * 4.0f};
            DrawRectangleRounded(glowRect, 0.2f, 10, Fade(BLUE, 0.08f / i));
        }
        DrawText("Options", popupRect.x + 90, popupRect.y + 20, 30, WHITE);
        int         idx  = g->currentMediaIndex;
        const char *opts[5] = {
            "Play",
            lib->isFavorite[idx] ? "Unfavorite" : "Favorite",
            "Move", "TAG", "Delete"};
        for (int opt = 0; opt < 5; opt++)
        {
            Rectangle optRect = {popupRect.x + 20, popupRect.y + 60 + opt * 40, 260, 30};
            bool selected = (opt == g->optionsSelectedIndex);
            DrawRectangleRounded(optRect, 0.2f, 10, selected ? BLUE : SKYBLUE);
            DrawText(opts[opt], optRect.x + 10, optRect.y + 5, 20, WHITE);
        }
    }
}

void UpdateMediaGrid(AppState *g, MediaLibrary *lib)
{
    if (IsKeyPressed(KEY_BACKSPACE) && (!g->showingOptions))
    {
        g->currentScreen = STATE_MAIN_MENU;
        g->selectedIndex = 0;
        PlaySound(g->backSound);
        return;
    }
    if (lib->count == 0)
    {
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            g->currentScreen = STATE_MAIN_MENU;
            g->selectedIndex = 0;
            PlaySound(g->backSound);
        }
        g->selectedIndex = -1;
        return;
    }
    int columns = 4;
    if (IsKeyPressed(KEY_DOWN) && (!g->showingOptions))
    {
        g->selectedIndex += columns;
        if (g->selectedIndex >= lib->count) g->selectedIndex = lib->count - 1;
        PlaySound(g->selectSound);
    }
    if (IsKeyPressed(KEY_UP) && (!g->showingOptions))
    {
        g->selectedIndex -= columns;
        if (g->selectedIndex < 0) g->selectedIndex = 0;
        PlaySound(g->selectSound);
    }
    if (IsKeyPressed(KEY_RIGHT) && (!g->showingOptions))
    {
        g->selectedIndex++;
        if (g->selectedIndex >= lib->count) g->selectedIndex = lib->count - 1;
        PlaySound(g->selectSound);
    }
    if (IsKeyPressed(KEY_LEFT) && (!g->showingOptions))
    {
        g->selectedIndex--;
        if (g->selectedIndex < 0) g->selectedIndex = 0;
        PlaySound(g->selectSound);
    }
    if (g->showingOptions || g->confirmDelete)
    {
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            g->confirmDelete  = false;
            g->showingOptions = false;
            PlaySound(g->backSound);
            return;
        }
        if (g->confirmDelete)
        {
            if (IsKeyPressed(KEY_LEFT))  g->optionsSelectedIndex = 0;
            if (IsKeyPressed(KEY_RIGHT)) g->optionsSelectedIndex = 1;
            if (IsKeyPressed(KEY_ENTER))
            {
                if (g->optionsSelectedIndex == 0)
                {
                    int idx = g->currentMediaIndex;
                    if (lib->thumbnails[idx].id > 0)
                        UnloadTexture(lib->thumbnails[idx]);
                    for (int i = idx; i < lib->count - 1; i++)
                    {
                        strcpy(lib->paths[i], lib->paths[i + 1]);
                        lib->thumbnails[i]  = lib->thumbnails[i + 1];
                        lib->isFavorite[i]  = lib->isFavorite[i + 1];
                    }
                    lib->count--;
                    SaveSettings(g);
                    if (g->selectedIndex >= lib->count)
                        g->selectedIndex = lib->count - 1;
                }
                g->confirmDelete  = false;
                g->showingOptions = false;
            }
            return;
        }
        if (g->showingOptions)
        {
            if (IsKeyPressed(KEY_DOWN)) g->optionsSelectedIndex = MIN(g->optionsSelectedIndex + 1, 4);
            if (IsKeyPressed(KEY_UP))   g->optionsSelectedIndex = MAX(g->optionsSelectedIndex - 1, 0);
            if (IsKeyPressed(KEY_ENTER))
            {
                int idx = g->currentMediaIndex;
                switch (g->optionsSelectedIndex)
                {
                case 0:
                    PlayVideo(g, lib->paths[idx]);
                    break;
                case 1:
                    lib->isFavorite[idx] = !lib->isFavorite[idx];
                    SaveSettings(g);
                    break;
                case 2:
                    g->showingOptions = false;
                    break;
                case 3:
                    g->editingIndex        = g->currentMediaIndex;
                    g->previousScreen      = g->currentScreen;
                    g->currentScreen       = STATE_GENRE_SELECT;
                    g->isTaggingMode       = true;
                    g->genreSelectedIndex  = 0;
                    g->showingOptions      = false;
                    PlaySound(g->selectSound);
                    break;
                case 4:
                    g->confirmDelete        = true;
                    g->optionsSelectedIndex = 0;
                    return;
                }
            }
            return;
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER) && lib->count > 0 && (!g->showingOptions))
        {
            g->showingOptions       = true;
            g->optionsSelectedIndex = 0;
            g->currentMediaIndex    = g->selectedIndex;
        }
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) g->selectedIndex += columns * 3;
    if (IsKeyPressed(KEY_PAGE_UP))   g->selectedIndex -= columns * 3;
    if (g->selectedIndex < 0)              g->selectedIndex = 0;
    if (g->selectedIndex >= lib->count)    g->selectedIndex = lib->count - 1;
    float cardH       = (GetScreenWidth() - 150) / 4 * 0.5625f + 30;
    float selectedY   = (g->selectedIndex / columns) * cardH;
    float targetOffset = selectedY - GetScreenHeight() * 0.3f;
    if (targetOffset < 0) targetOffset = 0;
    g->gridScrollOffset += (targetOffset - g->gridScrollOffset) * 0.25f;
    Vector2 mouse = GetMousePosition();
    float   sw    = (float)GetScreenWidth();
    Rectangle genreBtn = {sw - 810, 25, 180, 50};
    if (CheckCollisionPointRec(mouse, genreBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        g->previousScreen     = g->currentScreen;
        g->currentScreen      = STATE_GENRE_SELECT;
        g->isTaggingMode      = false;
        g->genreSelectedIndex = g->activeGenreIdx;
        g->genreMediaFocus    = false;
    }
}

void UpdatePlayer(AppState *g)
{
    double now = GetTime();
    if (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)
        g->lastMouseMoveTime = now;
    bool  mouseActive  = (now - g->lastMouseMoveTime < 3.0);
    float targetAlpha  = mouseActive ? 1.0f : 0.0f;
    g->seekBarAlpha   += (targetAlpha - g->seekBarAlpha) * 0.15f;
    double timePos  = 0.0;
    double duration = 0.0;
    mpv_get_property(g->mpv, "time-pos",  MPV_FORMAT_DOUBLE, &timePos);
    mpv_get_property(g->mpv, "duration",  MPV_FORMAT_DOUBLE, &duration);
    g->videoTimePos  = timePos;
    g->videoDuration = duration;
    if (duration <= 0) return;
    g->seekProgress  = (float)(timePos / duration);
    float sw   = (float)GetScreenWidth();
    float barY = GetScreenHeight() - 100.0f;
    float barH = 8.0f;
    Rectangle barRect = {40, barY, sw - 80, barH};
    Vector2   mouse   = GetMousePosition();
    bool overBar = CheckCollisionPointRec(mouse, barRect);
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (overBar || g->isSeeking))
    {
        g->isSeeking  = true;
        float seekX   = Clamp(mouse.x, barRect.x, barRect.x + barRect.width);
        float newProgress = (seekX - barRect.x) / barRect.width;
        char  seekStr[64];
        snprintf(seekStr, sizeof(seekStr), "%.3f", newProgress * duration);
        const char *cmd[] = {"seek", seekStr, "absolute", NULL};
        mpv_command(g->mpv, cmd);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        g->isSeeking = false;
}

void DrawPlayer(AppState *g)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    ClearBackground(MAGENTA);
    rlDrawRenderBatchActive();
    mpv_opengl_fbo fbo = {.fbo = 0, .w = (int)sw, .h = (int)sh, .internal_format = 0};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
        {0}};
    mpv_render_context_render(g->mpv_ctx, params);
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlDisableDepthTest();
    rlEnableColorBlend();
    DrawRectangle(0, sh - 80, sw, 80, (Color){20, 20, 20, 200});
    DrawText(GetFileName(g->currentVideoPath), 20, sh - 60, 20, GOLD);
    double timePos = 0;
    mpv_get_property(g->mpv, "time-pos", MPV_FORMAT_DOUBLE, &timePos);
    char timeBuffer[64];
    sprintf(timeBuffer, "Time: %.0f sec", timePos);
    DrawText(timeBuffer, sw - 200, sh - 60, 20, WHITE);
    if (g->seekBarAlpha > 0.05f)
    {
        float barY = GetScreenHeight() - 100.0f;
        float barH = 8.0f;
        Rectangle barRect = {40, barY, sw - 80, barH};
        DrawRectangleRounded(barRect,    0.5f, 8, Fade(BLACK, 0.6f * g->seekBarAlpha));
        Rectangle progressRect = {barRect.x, barRect.y,
                                   barRect.width * g->seekProgress, barH};
        DrawRectangleRounded(progressRect, 0.5f, 8, Fade(RED, g->seekBarAlpha));
        float knobX = barRect.x + barRect.width * g->seekProgress;
        float knobR = 12.0f;
        DrawCircle((int)knobX, (int)(barRect.y + barH / 2), knobR,
                   Fade(YELLOW, g->seekBarAlpha));
        DrawCircleLines((int)knobX, (int)(barRect.y + barH / 2), knobR,
                        Fade(WHITE, g->seekBarAlpha));
        char curTime[32], totTime[32];
        snprintf(curTime, sizeof(curTime), "%02d:%02d",
                 (int)(g->videoTimePos / 60), (int)g->videoTimePos % 60);
        snprintf(totTime, sizeof(totTime), "%02d:%02d",
                 (int)(g->videoDuration / 60), (int)g->videoDuration % 60);
        DrawText(curTime, (int)(barRect.x - 60),                    (int)(barY + 10), 20,
                 Fade(WHITE, g->seekBarAlpha));
        DrawText(totTime, (int)(barRect.x + barRect.width + 10),    (int)(barY + 10), 20,
                 Fade(WHITE, g->seekBarAlpha));
    }
}