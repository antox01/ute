#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "buffer.h"
#include "commands.h"
#include "editor.h"
#include "utils.h"
#include "lexer.h"

#define RGFW_OPENGL
#define RGFW_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#include "RGFW.h"

#include <OpenGL/gl3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define FONT_SIZE 48
#define FONT_SCALE 0.5

typedef struct {
    float texture_start; // Normalized
    float texture_end; // Normalized
    float texture_vertical_end; // Normalized
    float width, height;
    float bearing_left, bearing_top;
    unsigned int advance;
} Character;

typedef struct {
    GLuint vao, vbo;
    unsigned int shader_program;
    unsigned int texture_atlas;

    Character characters[256];
} Text_Renderer;

Text_Renderer text_renderer = {0};

typedef struct {
    GLuint vao, vbo;
    unsigned int shader_program;
} Cursor_Renderer;

Cursor_Renderer cursor_renderer = {0};

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

#define RED   ((Color) {.r = 0xff, .b = 0x0,  .g = 0x0,  .a = 0xff})
#define BLUE  ((Color) {.r = 0x0,  .b = 0xff, .g = 0x0,  .a = 0xff})
#define GREEN ((Color) {.r = 0x0,  .b = 0x0,  .g = 0xff, .a = 0xff})
#define WHITE ((Color) {.r = 0xff, .b = 0xff, .g = 0xff, .a = 0xff})
#define BLACK ((Color) {.r = 0x0, .b = 0x0, .g = 0x0, .a = 0x0})

typedef struct {
    float x, y, z, w;
} Vec4f;

Editor ute = {0};

const char *vertex_shader_str = 
"#version 330 core\n"
"\n"
"layout (location = 0) in vec4 position;\n"
"out vec2 text_coords;\n"
"\n"
"uniform vec2 screen_sizes;\n"
"\n"
"void main()\n"
"{\n"
"    float posx = 2*position.x/screen_sizes.x - 1.0f;\n"
"    float posy = 2*position.y/screen_sizes.y - 1.0f;\n"
"    gl_Position = vec4(posx, posy, 0.0, 1.0);\n"
"    text_coords = position.zw;\n"
"}";

const char *text_fragment_shader_str =
"#version 330 core\n"
"\n"
"in vec2 text_coords;\n"
"out vec4 frag_color;\n"
"\n"
"uniform sampler2D text;\n"
"uniform vec4 text_color;\n"
"\n"
"void main()\n"
"{\n"
"    frag_color = text_color * vec4(1.0, 1.0, 1.0, texture(text, text_coords).r);\n"
"}";

const char *cursor_fragment_shader_str =
"#version 330 core\n"
"\n"
"out vec4 frag_color;\n"
"\n"
"uniform sampler2D text;\n"
"uniform vec4 cursor_color;\n"
"\n"
"void main()\n"
"{\n"
"    frag_color = cursor_color;\n"
"}";

bool init_text_renderer() {
    glGenVertexArrays(1, &text_renderer.vao);
    glGenBuffers(1, &text_renderer.vbo);
    glBindVertexArray(text_renderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, text_renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);

    FT_Library  library;
    FT_Face face;
    FT_Error ft_error;

    ft_error = FT_Init_FreeType(&library);
    if(ft_error) {
        fprintf(stderr, "ERROR: could init FreeType library\n");
        return false;
    }

    ft_error = FT_New_Face(library, "./fonts/UbuntuMonoNerdFont-Regular.ttf", 0, &face);
    if(ft_error) {
        fprintf(stderr, "ERROR: could load font file\n");
        return false;
    }

    ft_error = FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);
    if(ft_error) {
        fprintf(stderr, "ERROR: could not set pixel size of %d\n", FONT_SIZE);
        return false;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    int atlas_width = 0;
    int atlas_height = 0;

    for(int c = 32; c < 128; c++) {
        ft_error = FT_Load_Char(face, c, FT_LOAD_RENDER);
        if(ft_error) continue;
        atlas_width += face->glyph->bitmap.width;
        if(face->glyph->bitmap.rows > atlas_height) {
            atlas_height = face->glyph->bitmap.rows;
        }
    }

    unsigned char *atlas = calloc(atlas_width * atlas_height, sizeof(*atlas));
    int atlas_offset = 0;
    for(int c = 32; c < 128; c++) {
        ft_error = FT_Load_Char(face, c, FT_LOAD_RENDER);
        if(ft_error) continue;
        FT_Bitmap *bitmap = &face->glyph->bitmap;

        for(int br = 0; br < bitmap->rows; br++) {
            for(int bx = 0; bx < bitmap->width; bx++) {
                atlas[br*atlas_width + bx + atlas_offset] = bitmap->buffer[bx + br*bitmap->pitch];
            }
        }

        Character ch = {
            .texture_start = (float) atlas_offset / (float) atlas_width,
            .texture_end = (float) (atlas_offset + bitmap->width) / (float) atlas_width,
            .texture_vertical_end = (float) bitmap->rows / (float) atlas_height,
            .width = face->glyph->bitmap.width,
            .height = face->glyph->bitmap.rows,
            .bearing_left = face->glyph->bitmap_left,
            .bearing_top = face->glyph->bitmap_top,
            .advance = face->glyph->advance.x,
        };

        text_renderer.characters[c] = ch;
        atlas_offset += bitmap->width;
    }
    glGenTextures(1, &text_renderer.texture_atlas);
    glBindTexture(GL_TEXTURE_2D, text_renderer.texture_atlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas_width, atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    free(atlas);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return true;
}

bool init_cursor_renderer() {
    glGenVertexArrays(1, &cursor_renderer.vao);
    glGenBuffers(1, &cursor_renderer.vbo);
    glBindVertexArray(cursor_renderer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, cursor_renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindVertexArray(0);

    return true;
}

bool compile_text_shaders() {
    int success;
    char info_log[512];

    int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_str, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR: could not compile vertex shader:\n%s\n", info_log);
        return false;
    }

    int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &text_fragment_shader_str, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR: could not compile fragment shader:\n%s\n", info_log);
        return false;
    }

    text_renderer.shader_program = glCreateProgram();
    glAttachShader(text_renderer.shader_program, vertex_shader);
    glAttachShader(text_renderer.shader_program, fragment_shader);
    glLinkProgram(text_renderer.shader_program);
    glGetProgramiv(text_renderer.shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(text_renderer.shader_program, 512, NULL, info_log);
        fprintf(stderr, "ERROR: could not link program shader:\n%s\n", info_log);
        return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return true;
}

bool compile_cursor_shaders() {
    int success;
    char info_log[512];

    int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_str, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR: could not compile vertex shader:\n%s\n", info_log);
        return false;
    }

    int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &cursor_fragment_shader_str, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        fprintf(stderr, "ERROR: could not compile fragment shader:\n%s\n", info_log);
        return false;
    }

    cursor_renderer.shader_program = glCreateProgram();
    glAttachShader(cursor_renderer.shader_program, vertex_shader);
    glAttachShader(cursor_renderer.shader_program, fragment_shader);
    glLinkProgram(cursor_renderer.shader_program);
    glGetProgramiv(cursor_renderer.shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(cursor_renderer.shader_program, 512, NULL, info_log);
        fprintf(stderr, "ERROR: could not link program shader:\n%s\n", info_log);
        return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return true;
}

float measure_text(char *str, size_t str_len, float scale) {
    float penx = 0;

    for (size_t i = 0; i < str_len; i++) {
        Character ch = text_renderer.characters[str[i]];
        penx += (ch.advance >> 6)*scale;
    }
    return penx;
}

float render_text(char *str, size_t str_len, float x, float y, float scale, Color color) {

    float char_width = measure_text("A", 1, FONT_SCALE);
    float char_height = FONT_SCALE*FONT_SIZE;

    float screen_width = ute.screen_width * char_width;
    float screen_height = ute.screen_height * char_height;
    glUseProgram(text_renderer.shader_program);
    glUniform4f(glGetUniformLocation(text_renderer.shader_program, "text_color"), (float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0, (float)color.a/255.0);
    glUniform2f(glGetUniformLocation(text_renderer.shader_program, "screen_sizes"), (float) screen_width, (float) screen_height);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(text_renderer.vao);

    glBindTexture(GL_TEXTURE_2D, text_renderer.texture_atlas);
    float penx = x;

    Vec4f *vertices = calloc(str_len * 6, sizeof(Vec4f));
    int vertices_index = 0;

    for (size_t i = 0; i < str_len; i++) {
        Character ch = text_renderer.characters[str[i]];

        float xpos = penx + ch.bearing_left * scale;
        float ypos = screen_height - y - FONT_SIZE*scale - (ch.height - ch.bearing_top) * scale;

        float w = ch.width * scale;
        float h = ch.height * scale;

        if(str[i] == ' ') {
            penx += (ch.advance >> 6)*scale;
            continue;
        }

        /*
         * 
         *      +------+
         *      |      |
         *      |      |
         *      |      |
         * xpos +------+
         * ypos 
         */
        vertices[vertices_index++] = (Vec4f){xpos,     ypos + h, ch.texture_start, 0.0f};
        vertices[vertices_index++] = (Vec4f){xpos,     ypos,     ch.texture_start, ch.texture_vertical_end};
        vertices[vertices_index++] = (Vec4f){xpos + w, ypos,     ch.texture_end, ch.texture_vertical_end};

        vertices[vertices_index++] = (Vec4f){xpos,     ypos + h, ch.texture_start, 0.0f};
        vertices[vertices_index++] = (Vec4f){xpos + w, ypos,     ch.texture_end, ch.texture_vertical_end};
        vertices[vertices_index++] = (Vec4f){xpos + w, ypos + h, ch.texture_end, 0.0f};


        penx += (ch.advance >> 6)*scale;
    }
    glBindBuffer(GL_ARRAY_BUFFER, text_renderer.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices_index * sizeof(*vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, vertices_index);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(vertices);
    return penx;
}

void render_cursor(float x, float y, float w, float h, Color color) {

    float char_width = measure_text("A", 1, FONT_SCALE);
    float char_height = FONT_SCALE*FONT_SIZE;

    float screen_width = ute.screen_width * char_width;
    float screen_height = ute.screen_height * char_height;

    glUseProgram(cursor_renderer.shader_program);
    glUniform4f(glGetUniformLocation(cursor_renderer.shader_program, "cursor_color"), (float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0, (float)color.a/255.0);
    glUniform2f(glGetUniformLocation(cursor_renderer.shader_program, "screen_sizes"), (float) screen_width, (float) screen_height);
    glBindVertexArray(cursor_renderer.vao);

    float ypos = screen_height - y - h;

    float vertices[6][4] = {
        {x,     ypos + h, 0.0f, 0.0f},
        {x,     ypos,     0.0f, 0.0f},
        {x + w, ypos,     0.0f, 0.0f},

        {x,     ypos + h, 0.0f, 0.0f},
        {x + w, ypos,     0.0f, 0.0f},
        {x + w, ypos + h, 0.0f, 0.0f},
    };
    glBindBuffer(GL_ARRAY_BUFFER, cursor_renderer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(0);
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    UTE_ASSERT(*argc > 0 && arg != NULL, "ERROR: no arguments provided");
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

void render_prompt(Editor *ute) {
    if(ute->mode != COMMAND_MODE) return;

    float char_width = measure_text("A", 1, FONT_SCALE);
    float char_height = FONT_SCALE*FONT_SIZE;

    size_t width = ute->screen_width * char_width;
    size_t ypos = (ute->screen_height - 1) * char_height;

    size_t command_pos = 0;

    if(ute->prompt != NULL) {
        command_pos = render_text(ute->prompt, strlen(ute->prompt), 4, ypos, FONT_SCALE, WHITE);
    }

    if(ute->command.lines.count > 0) {
        Line line = ute->command.lines.data[ute->command.lines.count - 1];
        String_View sv = { .data = &ute->command.sb.data[line.start], .count = line.end - line.start};

        command_pos = render_text(sv.data, sv.count, 4 + command_pos, ypos, FONT_SCALE, WHITE);
    }

    render_cursor(command_pos, ypos, 5, char_height, WHITE);
}

void render_display(Editor *ute) {
    int cy, cx;
    Buffer *buffer = current_buffer(ute);
    Display *display = &ute->display;

    int saved_cursor = buffer->cursor;
    buffer_posyx(buffer, saved_cursor, &cy, &cx);

    UTE_ASSERT(cx >= 0 && cy >= 0, "ERROR: got cx or cy negative");
    
    // NOTE: This assumes monospace characters
    float char_width = measure_text("A", 1, FONT_SCALE);
    float char_height = FONT_SCALE*FONT_SIZE;

    size_t width = ute->screen_width * char_width;
    size_t height = (ute->screen_height - 1)* char_height;

    int cur_x = 0;
    int cur_y = 0;

    size_t y = 0;
    size_t buffer_y = 0;
    while(y < height && buffer_y + buffer->sy < buffer->lines.count) {
        Line line = buffer->lines.data[buffer_y+buffer->sy];
        int x = 0;
        size_t curr_char = line.start + buffer->sx;
        display->count = 0;
        while(curr_char < line.end) {
            if(buffer->sb.data[curr_char] == '\n') break;
            ute_da_append(display, buffer->sb.data[curr_char]);
            if(measure_text(display->data, display->count, FONT_SCALE) >= width) {
                display->count--;
                break;
            }
            curr_char++;
        }
        render_text(display->data, display->count, x, y, FONT_SCALE, WHITE);
        y+=char_height;
        buffer_y++;
    }

    cur_y = cy - buffer->sy;
    cur_x = cx - buffer->sx;
    if(cur_y >= height) cur_y = height - 1;
    render_cursor(cur_x*char_width, cur_y*char_height, char_width, char_height, WHITE);
}

void window_resizes(RGFW_window *win, i32 w, i32 h) {
    (void) win;
    // printf("Callback called, w:%d, h:%d\n", w, h);
    float char_width = measure_text("A", 1, FONT_SCALE);
    float char_height = FONT_SCALE*FONT_SIZE;

    ute.screen_width = w / char_width;
    ute.screen_height = h/ char_height;
}

int main(int argc, char **argv) {
    RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
    hints->major = 3;
    hints->minor = 3;
    RGFW_setGlobalHints_OpenGL(hints);

    RGFW_window *window = RGFW_createWindow("UTE", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGFW_windowCenter | RGFW_windowOpenGL);
    if(window == NULL) {
        fprintf(stderr, "ERROR: could not open RGFW window\n");
        return 1;
    }
    RGFW_setWindowResizedCallback(window_resizes);
    RGFW_window_makeCurrentContext_OpenGL(window);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    if(!init_text_renderer()) return 1;
    if(!init_cursor_renderer()) return 1;

    float char_width = measure_text("A", 1, FONT_SCALE);
    float char_height = FONT_SCALE*FONT_SIZE;
    ute.screen_width = SCREEN_WIDTH / char_width;
    ute.screen_height = SCREEN_HEIGHT / char_height;
    if(!compile_text_shaders()) return 1;
    if(!compile_cursor_shaders()) return 1;

    shift_args(&argc, &argv);

    if(argc > 0) {
        const char *file_name = strdup(shift_args(&argc, &argv));
        String_Builder sb = {0};
        if(read_file(&sb, file_name)) {
            Buffer buffer = {0};
            buffer_insert_str(&buffer, sb.data, sb.count);
            buffer.file_name = (char *)file_name;

            buffer_set_cursor(&buffer, 0);
            ute_da_append(&ute.buffers, buffer);
            ute.curr_buffer = ute.buffers.count - 1;
        } else {
            // TODO: error reporting
        }
        if(sb.max_size > 0) free(sb.data);
    }

    if(ute.buffers.count == 0) {
        Buffer buffer = {0};

        buffer_set_cursor(&buffer, 0);
        ute_da_append(&ute.buffers, buffer);
        ute.curr_buffer = ute.buffers.count - 1;
    }

    while(RGFW_window_shouldClose(window) == RGFW_FALSE) {
        RGFW_event event;
        if(RGFW_window_checkEvent(window, &event) == RGFW_TRUE) {
            if(event.type == RGFW_keyPressed) {
                manage_key(&ute, event.key.sym);
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        update_display(&ute);
        render_display(&ute);

        render_prompt(&ute);

        RGFW_window_swapBuffers_OpenGL(window);
    }

    RGFW_window_close(window);
    return 0;
}

