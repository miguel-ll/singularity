#include <stdlib.h>
#include <stdio.h>
#include <raylib.h>
#include "macros.h"
#include "themes.h" // include after macros
#include "stroke.h"

int msqrt(int number) {
    int root = 1;
    for (int i=0; i < number+1; i++)
	    root = (number / root + root) /2;
    return root;
}

int vec2Length(int _x1, int _y1, int _x2, int _y2) {
    int s = (_x1 - _x2)*(_x1 - _x2) + (_y1 - _y2)*(_y1 - _y2);
    return msqrt(s);
}

void renderStrokes(Stroke *_strokes) {
    Stroke *cs = _strokes;
    while(cs != NULL) {
        StrokePoint *cp = cs->points;
        while(cp != NULL) {
            DrawCircle(cp->x, cp->y, (float)cs->size/2, cs->color);
            if(cp->next != NULL)
                DrawLineEx( (Vector2){cp->x, cp->y}, (Vector2){cp->next->x, cp->next->y}, cs->size, cs->color);
            cp = cp->next;
        }
        cs = cs->next;
    }
}

void renderColors( int _win_height, Color _colors[9], int _selected, int _offset) {
    DrawRectangleRounded( (Rectangle){10 - _offset, (float)_win_height / 2 - 130, 40, 280}, _UI_B_RADIUS, 4, _UI_BG);
    int y_offset = 120;
    for(unsigned i = 0; i < 9; ++i) {
        DrawRectangle( 20 - _offset, _win_height / 2 - y_offset, 20, 20, _colors[i]);
        y_offset -= 30;
    }
    y_offset = 150 - _selected * 30;
    DrawRectangleLines( 19 - _offset, _win_height / 2 - y_offset - 1, 22, 22, WHITE);
}

void renderBrushSize(int _size, int _offset) {
    char size[5]; sprintf(size, "%i", _size);
    int txt_w = MeasureText(size, 20);
    DrawRectangleRounded( (Rectangle){10, 10 - _offset, 20 + txt_w, 40}, _UI_B_RADIUS, 4, _UI_BG);
    DrawText(size, 20, 20 - _offset, 20, WHITE);
}

int renderZoomLevel( int _win_width, int _win_height, float _level, int _offset) {
    char s[5]; sprintf(s, "x%2.1f", _level);
    int txt_w = MeasureText(s, 20);
    DrawRectangleRounded( (Rectangle){ _win_width - 30 - txt_w, _win_height - 50 + _offset,20 + txt_w, 40 }, _UI_B_RADIUS, 4, _UI_BG);
    DrawText( s, _win_width - 20 - txt_w, _win_height - 40 + _offset, 20, WHITE);

    return txt_w + 20;
}

int renderStrokeCount( int _win_width, int _win_height, unsigned _count, int _offset, int _prev_offset) {
    char s[32]; sprintf(s, "Strokes: %i", _count);
    int txt_w = MeasureText(s, 20);
    DrawRectangleRounded( (Rectangle){ _win_width - 30 - txt_w - _prev_offset, _win_height - 50 + _offset,20 + txt_w, 40 }, _UI_B_RADIUS, 4, _UI_BG);
    DrawText( s, _win_width - 20 - txt_w - _prev_offset, _win_height - 40 + _offset, 20, WHITE);
    return txt_w + 20;
}
//-stroke count-----// <count>
//-stroke data------// <point count> <size> <red> <green> <blue> <alpha>
//-point data-------// <x> <y> <x> <y> ...
void saveBoard(FILE *_file, Stroke *_strokes, unsigned _stroke_count) {
    fprintf(_file, "%u\n", _stroke_count);
    Stroke *cs = _strokes;
    while(cs != NULL) {
        fprintf(_file, "%u %i %u %u %u %u\n", cs->point_count, cs->size, cs->color.r, cs->color.g, cs->color.b, cs->color.a);
        StrokePoint *cp = cs->points;
        while(cp != NULL) {
            fprintf( _file, "%i %i ", cp->x, cp->y);
            cp = cp->next;
        }
        fprintf(_file, "\n");
        cs = cs->next;
    }
}

Stroke* openBoard(FILE *_file, unsigned *_stroke_count, unsigned *_point_count, Stroke **_last_stroke) {
    fscanf(_file, "%u\n", _stroke_count);
    unsigned points;
    int size;
    Stroke *strokes = NULL;
    StrokePoint *lp = NULL;
    for(unsigned i = 0; i < *_stroke_count; ++i) {
        unsigned r, g, b, a;
        fscanf(_file, "%u %i %u %u %u %u\n", &points, &size, &r, &g, &b, &a);
        if(i == 0) {
            strokes = createStroke(size, (Color){r, g, b, a});
            strokes->point_count = points;
            *_point_count += points;
            *_last_stroke = strokes;
        } else {
            (*_last_stroke)->next = createStroke(size, (Color){r, g, b, a});
            (*_last_stroke)->next->point_count = points;
            *_point_count += points;
            *_last_stroke = (*_last_stroke)->next;
        }
        int x, y;
        for(unsigned ii = 0; ii < points; ++ii) {
            fscanf(_file, "%i %i ", &x, &y);
            if(ii == 0) {
                (*_last_stroke)->points = createStrokePoint(x, y);
                lp = (*_last_stroke)->points;
            } else {
                lp->next = createStrokePoint(x, y);
                lp = lp->next;
            }
        }
    }
    return strokes;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int win_width = _INIT_W, win_height = _INIT_H;
    InitWindow(win_width, win_height, "singularity");

    Camera2D camera = {
        (Vector2){0, 0}, // offset
        (Vector2){0, 0}, // target
        0, 1 // rotation, zoom
    };
    SetTargetFPS(_FPS);
    HideCursor();

    int brush_size = _INIT_BRUSH_SIZE;
    Color colors[9] = { _PALETTE_01, _PALETTE_02, _PALETTE_03, _PALETTE_04, _PALETTE_05, _PALETTE_06, _PALETTE_07, _PALETTE_08, _PALETTE_09 };

    unsigned brush_color = 0;
    Vector2 last_mouse_pos = {0, 0};
    Stroke *strokes = NULL;
    unsigned stroke_count = 0;
    unsigned point_count = 0;
    Stroke *last_stroke = NULL;
    StrokePoint *last_point = NULL;
    Stroke *history[64];
    unsigned history_index = 0;
    int drawing = 0;
    int ui_offset = 0;
    int animating_ui = 0;
    int ui_closed = 0;
    int win_gained_focus = 0;

    if(FileExists("savefile")) {
        FILE *savefile = fopen("savefile", "r");
        strokes = openBoard(savefile, &stroke_count, &point_count, &last_stroke);
        fclose(savefile);
    }
    while(!WindowShouldClose()) {
        int redraw = 0;
        // Redraw when window gains focus
        if(IsWindowFocused()) {
            if(!win_gained_focus) {
                win_gained_focus = 1;
                redraw = 1;
            }
        } else {
            win_gained_focus = 0;
        }
        if(IsWindowResized()) {
            redraw = 1;
            win_width = GetScreenWidth();
            win_height = GetScreenHeight();
        }
        float mouse_wheel = GetMouseWheelMove();
        if(mouse_wheel < 0) {
            redraw = 1;
            camera.zoom -= 0.1;
            if(camera.zoom < 0.1) camera.zoom = 0.1;
        }else if(mouse_wheel > 0) {
            redraw = 1;
            camera.zoom += 0.1;
            if(camera.zoom > 3) camera.zoom = 3;
        }
        int mouse_x = GetMouseX();
        int mouse_y = GetMouseY();
        if(last_mouse_pos.x != mouse_x || last_mouse_pos.y != mouse_y)
            redraw = 1;
        // Key bindings
        if(IsKeyPressed(_KEY_RESET_ZOOM)) {
            redraw = 1;
            camera.zoom = 1;
        }
        if(IsKeyPressed(_KEY_INC_BRUSH_SIZE)) {
            redraw = 1;
            brush_size += _BRUSH_INC;
        }else if(IsKeyPressed(_KEY_DEC_BRUSH_SIZE)) {
            redraw = 1;
            brush_size -= _BRUSH_INC;
            if(brush_size <= 0) brush_size = _BRUSH_INC;
        }else if(IsKeyPressed(_KEY_RESET_BRUSH_SIZE)) {
            redraw = 1;
            brush_size = _INIT_BRUSH_SIZE;
        } // save
            if(IsKeyPressed(_KEY_SAVE)) {
                FILE *savefile = fopen("savefile", "w");
                saveBoard(savefile, strokes, stroke_count);
                fclose(savefile);
            } // Undo & Redo
            if(IsKeyPressed(_KEY_UNDO) && history_index < 64) {
                redraw = 1;
                if(stroke_count > 1) {
                    Stroke **ls = &strokes;
                    for(int i = 0; i < stroke_count; ++i) {
                        if(i == stroke_count-2) {
                            history[history_index] = (*ls)->next;
                            point_count -= (*ls)->next->point_count;
                            (*ls)->next = NULL;
                            last_stroke = *ls;
                        }
                        ls = &((*ls)->next);
                    }
                    ++history_index;
                    --stroke_count;
                }else if(stroke_count == 1) {
                    history[history_index] = strokes;
                    strokes = NULL;
                    last_stroke = NULL;
                    ++history_index;
                    --stroke_count;
                    point_count = 0;
                }
            }else if(IsKeyPressed(_KEY_REDO) && history_index != 0) {
                redraw = 1;
                if(last_stroke != NULL) {
                    --history_index;
                    last_stroke->next = history[history_index];
                    history[history_index] = NULL;
                    last_stroke = last_stroke->next;
                    ++stroke_count;
                }else {
                    --history_index;
                    strokes = history[history_index];
                    history[history_index] = NULL;
                    last_stroke = strokes;
                    ++stroke_count;
                }
                point_count += last_stroke->point_count;
            }
                if(IsKeyDown(_KEY_PAN_UP)) {
                    redraw = 1;
                    camera.target.y -= 8;
                }
                if(IsKeyDown(_KEY_PAN_RIGHT)) {
                    redraw = 1;
                    camera.target.x += 8;
                }
                if(IsKeyDown(_KEY_PAN_DOWN)) {
                    redraw = 1;
                    camera.target.y += 8;
                }
                if(IsKeyDown(_KEY_PAN_LEFT)) {
                    redraw = 1;
                    camera.target.x -= 8;
                }
        // Colors
        // TODO: Make it not bad
        if(IsKeyPressed(_KEY_COLOR01)) {
            redraw = 1;
            brush_color = 0;
        }else if(IsKeyPressed(_KEY_COLOR02)) {
            redraw = 1;
            brush_color = 1;
        }else if(IsKeyPressed(_KEY_COLOR03)) {
            redraw = 1;
            brush_color = 2;
        }else if(IsKeyPressed(_KEY_COLOR04)) {
            redraw = 1;
            brush_color = 3;
        }else if(IsKeyPressed(_KEY_COLOR05)) {
            redraw = 1;
            brush_color = 4;
        }else if(IsKeyPressed(_KEY_COLOR06)) {
            redraw = 1;
            brush_color = 5;
        }else if(IsKeyPressed(_KEY_COLOR07)) {
            redraw = 1;
            brush_color = 6;
        }else if(IsKeyPressed(_KEY_COLOR08)) {
            redraw = 1;
            brush_color = 7;
        }else if(IsKeyPressed(_KEY_COLOR09)) {
            redraw = 1;
            brush_color = 8;
        }
        // Start drawing
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            redraw = 1;
            // Clear history
            if(history_index != 0) {
                for(unsigned i = 0; i < history_index; ++i) {
                    StrokePoint *cp = history[i]->points;
                    while(cp != NULL) {
                        StrokePoint *np = cp->next;
                        free(cp);
                        cp = np;
                    }
                    free(history[i]);
                }
                history_index = 0;
            }
            if(stroke_count == 0) {
                strokes = createStroke(brush_size, colors[brush_color]);
                strokes->points = createStrokePoint( (mouse_x / camera.zoom) + camera.target.x, (mouse_y / camera.zoom) + camera.target.y);
                ++strokes->point_count;
                last_stroke = strokes;
                last_point = strokes->points;
            }else {
                last_stroke->next = createStroke( brush_size, colors[brush_color]);
                last_stroke->next->points = createStrokePoint( (mouse_x / camera.zoom) + camera.target.x, (mouse_y / camera.zoom) + camera.target.y);
                ++last_stroke->next->point_count;
                last_stroke = last_stroke->next;
                last_point = last_stroke->points;
            }
            ++stroke_count;
            ++point_count;
            drawing = 1;
        }
        // Stop drawing
        else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && drawing) {
            redraw = 1;
            last_point->next = createStrokePoint( (mouse_x / camera.zoom) + camera.target.x, (mouse_y / camera.zoom) + camera.target.y);
            last_point = NULL;
            ++last_stroke->point_count;
            ++point_count;
            drawing = 0;
        }
        // Draw
        else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && drawing) {
         redraw = 1;
         int new_x = (mouse_x / camera.zoom) + camera.target.x;
         int new_y = (mouse_y / camera.zoom) + camera.target.y;
        if(vec2Length( last_point->x, last_point->y, new_x, new_y) > 3) {
                last_point->next = createStrokePoint(new_x, new_y);
                last_point = last_point->next;
                ++last_stroke->point_count;
                ++point_count;
            }
        }
        // UI
        if(IsKeyPressed(_KEY_TOGGLE_UI)) {
            animating_ui = 1;
        }
        if(animating_ui) {
            redraw = 1;
            float delta = GetFrameTime();
            if(ui_closed) {
                ui_offset -= 1000 * delta;
                if(ui_offset < 0) {
                    ui_offset = 0;
                    animating_ui = 0;
                    ui_closed = 0;
                }
            }else {
                ui_offset += 1000 * delta;
                if(ui_offset > 50) {
                    ui_offset = 50;
                    animating_ui = 0;
                    ui_closed = 1;
                }
            }
        }
            BeginDrawing();
            if(redraw) ClearBackground(_BG_COLOR);

            BeginMode2D(camera);

            if(redraw) renderStrokes(strokes);

            EndMode2D();

            if(redraw) {
                renderColors(win_height, colors, brush_color+1, ui_offset);
                renderBrushSize(brush_size, ui_offset);
                int zoom_lvl_w = renderZoomLevel(
                    win_width, win_height, camera.zoom, ui_offset
                );
                renderStrokeCount( win_width, win_height, stroke_count, ui_offset, zoom_lvl_w + 20);
                // Mouse Cursor
                DrawCircleLines( mouse_x, mouse_y, (float)brush_size / 2 * camera.zoom, (Color){230, 230, 230, 150});
            }
        EndDrawing();

        last_mouse_pos.x = mouse_x;
        last_mouse_pos.y = mouse_y;
    }
    // clean up
    Stroke *cs = strokes;
    while(cs != NULL) {
        StrokePoint *cp = cs->points;
        while(cp != NULL) {
            StrokePoint *np = cp->next;
            free(cp);
            cp = np;
        }
        Stroke *ns = cs->next;
        free(cs);
        cs = ns;
    }
    CloseWindow();
}

