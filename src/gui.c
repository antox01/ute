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

typedef struct {
    unsigned int texture;
    float width, height;
    float bearing_left, bearing_top;
    unsigned int advance;
} Character;

typedef struct {
    GLuint vao, vbo;
    unsigned int shader_program;

    Character characters[256];
} Text_Renderer;

Text_Renderer text_renderer = {0};

typedef struct {
    GLuint vao, vbo;
    unsigned int shader_program;
} Cursor_Renderer;

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

const char *fragment_shader_str =
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

    for(int c = 0; c < 128; c++) {
        ft_error = FT_Load_Char(face, c, FT_LOAD_RENDER);
        if(ft_error) continue;

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        Character ch = {
            .texture = texture,
            .width = face->glyph->bitmap.width,
            .height = face->glyph->bitmap.rows,
            .bearing_left = face->glyph->bitmap_left,
            .bearing_top = face->glyph->bitmap_top,
            .advance = face->glyph->advance.x,
        };

        text_renderer.characters[c] = ch;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return true;
}

bool compile_shaders() {
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
    glShaderSource(fragment_shader, 1, &fragment_shader_str, NULL);
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
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
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
    glUseProgram(text_renderer.shader_program);
    glUniform4f(glGetUniformLocation(text_renderer.shader_program, "text_color"), (float)color.r/255.0, (float)color.g/255.0, (float)color.b/255.0, (float)color.a/255.0);
    glUniform2f(glGetUniformLocation(text_renderer.shader_program, "screen_sizes"), (float) SCREEN_WIDTH, (float) SCREEN_HEIGHT);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(text_renderer.vao);

    float penx = x;

    for (size_t i = 0; i < str_len; i++) {
        Character ch = text_renderer.characters[str[i]];

        float xpos = penx + ch.bearing_left * scale;
        float ypos = SCREEN_HEIGHT - FONT_SIZE*scale - y - (ch.height - ch.bearing_top) * scale;

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
        float vertices[6][4] = {
            {xpos,     ypos + h, 0.0f, 0.0f},
            {xpos,     ypos,     0.0f, 1.0f},
            {xpos + w, ypos,     1.0f, 1.0f},

            {xpos,     ypos + h, 0.0f, 0.0f},
            {xpos + w, ypos,     1.0f, 1.0f},
            {xpos + w, ypos + h, 1.0f, 0.0f},
        };

        glBindTexture(GL_TEXTURE_2D, ch.texture);
        glBindBuffer(GL_ARRAY_BUFFER, text_renderer.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        penx += (ch.advance >> 6)*scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return penx;
}

float render_cursor(float x, float y, float scale, Color color) {
}

char *shift_args(int *argc, char ***argv) {
    char *arg = **argv;
    UTE_ASSERT(*argc > 0 && arg != NULL, "ERROR: no arguments provided");
    *argc = *argc -1;
    *argv = *argv + 1;
    return arg;
}

void render_display(Editor *ute) {
    int cy, cx;
    Buffer *buffer = current_buffer(ute);
    Display *display = &ute->display;

    int saved_cursor = buffer->cursor;
    buffer_posyx(buffer, saved_cursor, &cy, &cx);

    UTE_ASSERT(cx >= 0 && cy >= 0, "ERROR: got cx or cy negative");

    size_t width = ute->screen_width;
    size_t height = ute->screen_height;

    int cur_x = 0;
    int cur_y = 0;
    
    float scale = 0.8;

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
            if(measure_text(display->data, display->count, scale) >= width) {
                display->count--;
                break;
            }
            curr_char++;
        }
        render_text(display->data, display->count, 0, y, scale, WHITE);
        y+=scale*FONT_SIZE;
        buffer_y++;
    }

    cur_y = cy - buffer->sy;
    cur_x = cx - buffer->sx;
    if(cur_y >= height) cur_y = height - 1;
    render_cursor(cur_x, cur_y, scale, WHITE);
}

int main(int argc, char **argv) {
    RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
    hints->major = 3;
    hints->minor = 3;
    RGFW_setGlobalHints_OpenGL(hints);

    RGFW_window *window = RGFW_createWindow("UTE", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGFW_windowNoResize | RGFW_windowCenter | RGFW_windowOpenGL);
    if(window == NULL) {
        fprintf(stderr, "ERROR: could not open RGFW window\n");
        return 1;
    }
    RGFW_window_makeCurrentContext_OpenGL(window);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    if(!init_text_renderer()) return 1;

    if(!compile_shaders()) return 1;

    Editor ute = {.screen_height = SCREEN_HEIGHT, .screen_width = SCREEN_WIDTH};
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
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        update_display(&ute);
        render_display(&ute);

        RGFW_window_swapBuffers_OpenGL(window);
    }

    RGFW_window_close(window);
    return 0;
}

