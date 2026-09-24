// ============================================================================
// Phantom Arcade — Native In-Core 15.7kHz CRT Frontend for GroovyNLC
// Embedded directly inside MiSTer_groovyNLC HPS binary
// ============================================================================
#include "phantom_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define FB_WIDTH  256
#define FB_HEIGHT 240
#define FB_STRIDE (FB_WIDTH * 3)

#define DEFAULT_PC_IP    "192.168.1.126"
#define DEFAULT_UDP_PORT 1999

#define COLOR_BG_R 0x0A
#define COLOR_BG_G 0x0D
#define COLOR_BG_B 0x16

#define COLOR_CARD_R 0x14
#define COLOR_CARD_G 0x1A
#define COLOR_CARD_B 0x2A

#define COLOR_BORDER_R 0x2E
#define COLOR_BORDER_G 0x36
#define COLOR_BORDER_B 0x4E

#define COLOR_AMBER_R 0xF5
#define COLOR_AMBER_G 0x9E
#define COLOR_AMBER_B 0x0B

#define COLOR_CYAN_R 0x06
#define COLOR_CYAN_G 0xB6
#define COLOR_CYAN_B 0xD4

#define COLOR_WHITE_R 0xF8
#define COLOR_WHITE_G 0xFA
#define COLOR_WHITE_B 0xFC

#define COLOR_GRAY_R 0x94
#define COLOR_GRAY_G 0xA3
#define COLOR_GRAY_B 0xB8

#define COLOR_DARK_R 0x47
#define COLOR_DARK_G 0x55
#define COLOR_DARK_B 0x69

#define COLOR_GREEN_R 0x10
#define COLOR_GREEN_G 0xB9
#define COLOR_GREEN_B 0x81

#define COLOR_RED_R 0xEF
#define COLOR_RED_G 0x44
#define COLOR_RED_B 0x44

typedef struct {
    char id[64];
    char title[96];
    char system[32];
    char systemName[32];
    char res[32];
    char mode[32];
    int hSync;
    int vSync;
} GameEntry;

static GameEntry games[256];
static int game_count = 0;
static int selected_idx = 0;
static int current_cat = 0;
static int is_launching = 0;
static uint32_t launch_start_time = 0;

static char pc_server_ip[64] = DEFAULT_PC_IP;
static int pc_udp_port = DEFAULT_UDP_PORT;

// Hold-to-scroll tracking
static uint32_t hold_start_time = 0;
static uint32_t last_scroll_time = 0;
static int last_direction = 0; // -1 = up, 1 = down

// Marquee scroll tracking
static uint32_t marquee_timer_origin = 0;
static int marquee_last_selected = -1;

#include "font_arcade.h"

static uint32_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static inline void put_pixel(unsigned char *fb, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    int offset = (y * FB_WIDTH + x) * 3;
    fb[offset + 0] = r;
    fb[offset + 1] = g;
    fb[offset + 2] = b;
}

static void draw_rect(unsigned char *fb, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            put_pixel(fb, i, j, r, g, b);
        }
    }
}

static void draw_box(unsigned char *fb, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int i = x; i < x + w; i++) {
        put_pixel(fb, i, y, r, g, b);
        put_pixel(fb, i, y + h - 1, r, g, b);
    }
    for (int j = y; j < y + h; j++) {
        put_pixel(fb, x, j, r, g, b);
        put_pixel(fb, x + w - 1, j, r, g, b);
    }
}

static void draw_char(unsigned char *fb, int x, int y, char c, uint8_t r, uint8_t g, uint8_t b) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font8x12[(int)(c - 32)];
    for (int row = 0; row < 12; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                put_pixel(fb, x + col, y + row, r, g, b);
            }
        }
    }
}

static void draw_text(unsigned char *fb, int x, int y, const char *str, uint8_t r, uint8_t g, uint8_t b) {
    int curX = x;
    while (*str) {
        draw_char(fb, curX, y, *str, r, g, b);
        curX += 8;
        str++;
    }
}

static void draw_text_marquee(unsigned char *fb, int x, int y, int max_chars, const char *str, uint8_t r, uint8_t g, uint8_t b, int is_sel) {
    int len = (int)strlen(str);
    if (len <= max_chars) {
        draw_text(fb, x, y, str, r, g, b);
        return;
    }
    if (!is_sel) {
        char buf[64];
        int count = max_chars < 63 ? max_chars : 63;
        strncpy(buf, str, count);
        buf[count] = 0;
        draw_text(fb, x, y, buf, r, g, b);
        return;
    }

    uint32_t now = get_time_ms();
    int extra = len - max_chars;
    uint32_t scroll_speed_ms = 180;
    uint32_t pause_origin_ms = 2000;
    uint32_t pause_end_ms    = 2000;
    uint32_t scroll_duration_ms = extra * scroll_speed_ms;
    uint32_t total_cycle_ms = pause_origin_ms + scroll_duration_ms + pause_end_ms;

    uint32_t elapsed = (now - marquee_timer_origin) % total_cycle_ms;
    int char_offset = 0;
    if (elapsed < pause_origin_ms) {
        char_offset = 0;
    } else if (elapsed < pause_origin_ms + scroll_duration_ms) {
        char_offset = (elapsed - pause_origin_ms) / scroll_speed_ms;
        if (char_offset > extra) char_offset = extra;
    } else {
        char_offset = extra;
    }

    char buf[64];
    int count = max_chars < 63 ? max_chars : 63;
    strncpy(buf, str + char_offset, count);
    buf[count] = 0;
    draw_text(fb, x, y, buf, r, g, b);
}

static void save_state() {
    FILE *f = fopen("/media/fat/config/phantom_state.ini", "w");
    if (!f) return;
    fprintf(f, "[State]\n");
    fprintf(f, "category=%d\n", current_cat);
    fprintf(f, "selected=%d\n", selected_idx);
    if (selected_idx >= 0 && selected_idx < game_count) {
        fprintf(f, "game_id=%s\n", games[selected_idx].id);
    }
    fclose(f);
}

static void load_state() {
    FILE *f = fopen("/media/fat/config/phantom_state.ini", "r");
    if (!f) return;
    char line[128];
    char target_id[64] = "";
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "category=", 9) == 0) current_cat = atoi(line + 9);
        else if (strncmp(line, "selected=", 9) == 0) selected_idx = atoi(line + 9);
        else if (strncmp(line, "game_id=", 8) == 0) {
            strncpy(target_id, line + 8, sizeof(target_id) - 1);
            char *nl = strchr(target_id, '\r'); if (!nl) nl = strchr(target_id, '\n');
            if (nl) *nl = 0;
        }
    }
    fclose(f);
    if (strlen(target_id) > 0) {
        for (int i = 0; i < game_count; i++) {
            if (strcmp(games[i].id, target_id) == 0) {
                selected_idx = i;
                break;
            }
        }
    }
}

static void load_config() {
    FILE *f = fopen("/media/fat/config/phantom.ini", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "PC_SERVER_IP=", 13) == 0) {
            char *val = line + 13;
            while (*val == ' ' || *val == '\t') val++;
            char *nl = strchr(val, '\r'); if (!nl) nl = strchr(val, '\n');
            if (nl) *nl = 0;
            if (strlen(val) > 0) strncpy(pc_server_ip, val, sizeof(pc_server_ip) - 1);
        } else if (strncmp(line, "UDP_PORT=", 9) == 0) {
            int p = atoi(line + 9);
            if (p > 0) pc_udp_port = p;
        }
    }
    fclose(f);
}

static void init_catalog() {
    game_count = 0;
    #define ADD_GAME(gid, gname, gsys, gsysname, gres, gmode, gh, gv) do { \
        if (game_count < 250) { \
            strncpy(games[game_count].id, gid, 63); \
            strncpy(games[game_count].title, gname, 95); \
            strncpy(games[game_count].system, gsys, 31); \
            strncpy(games[game_count].systemName, gsysname, 31); \
            strncpy(games[game_count].res, gres, 31); \
            strncpy(games[game_count].mode, gmode, 31); \
            games[game_count].hSync = gh; \
            games[game_count].vSync = gv; \
            game_count++; \
        } \
    } while(0)

    ADD_GAME("kinst",           "Killer Instinct (v1.5, USA)",             "groovymame", "GroovyMAME",  "240p @ 60.0Hz", "15.7kHz Native", 15734, 6000);
    ADD_GAME("kinst2",          "Killer Instinct 2 (v1.4, USA)",           "groovymame", "GroovyMAME",  "240p @ 60.0Hz", "15.7kHz Native", 15734, 6000);
    ADD_GAME("sfiii3",          "Street Fighter III: 3rd Strike (Euro)",   "groovymame", "Capcom CPS-3", "224p @ 59.6Hz", "15.6kHz Native", 15600, 5963);
    ADD_GAME("mk2",             "Mortal Kombat II (rev L3.1)",             "groovymame", "Midway Y",    "254p @ 54.7Hz", "15.2kHz Native", 15200, 5471);
    ADD_GAME("umk3",            "Ultimate Mortal Kombat 3 (rev 1.2)",       "groovymame", "Midway Wolf", "254p @ 54.7Hz", "15.2kHz Native", 15200, 5471);
    ADD_GAME("mvsc",            "Marvel vs. Capcom (Euro 980123)",         "groovymame", "Capcom CPS-2", "224p @ 59.6Hz", "15.6kHz Native", 15600, 5963);
    ADD_GAME("garou",           "Garou: Mark of the Wolves (NGM-2530)",    "groovymame", "SNK Neo-Geo",  "224p @ 59.2Hz", "15.6kHz Native", 15625, 5918);
    ADD_GAME("mslug3",          "Metal Slug 3 (NGM-2560)",                 "groovymame", "SNK Neo-Geo",  "224p @ 59.2Hz", "15.6kHz Native", 15625, 5918);
    ADD_GAME("tektagt",         "Tekken Tag Tournament (TEG3/VER.C1)",     "groovymame", "Namco S12",   "240p @ 60.0Hz", "15.7kHz Native", 15734, 6000);
    ADD_GAME("naomi_vf4ft",     "Virtua Fighter 4 Final Tuned (Ver.B)",    "flycast",    "Sega NAOMI",  "480i / 240p",   "15.7kHz CRT",    15734, 6000);
    ADD_GAME("naomi_cvs2",      "Capcom vs. SNK 2 (GDL-0008)",             "flycast",    "Sega NAOMI",  "240p @ 60.0Hz", "15.7kHz CRT",    15734, 6000);
    ADD_GAME("naomi_ikaruga",   "Ikaruga (GDL-0010)",                      "flycast",    "Sega NAOMI",  "240p Tate CRT", "15.7kHz CRT",    15734, 6000);
    ADD_GAME("ps2_3rdstrike",   "SF III 3rd Strike (USA, PS2)",            "pcsx2",      "PlayStation 2","240p @ 60.0Hz", "15.7kHz CRT",    15734, 6000);
    ADD_GAME("ps2_espgaluda",   "Espgaluda (Japan, PS2)",                  "pcsx2",      "PlayStation 2","240p @ 60.0Hz", "15.7kHz CRT",    15734, 6000);
    ADD_GAME("gc_melee",        "Super Smash Bros. Melee (v1.02, USA)",    "dolphin",    "GameCube",    "480i @ 60.0Hz", "15.7kHz CRT",    15734, 6000);
    ADD_GAME("model2_daytona",  "Daytona USA (USA, Model 2)",              "model2",     "Sega Model 2", "384p @ 60.0Hz", "24.8kHz MedRes", 24800, 6000);
}

static void send_udp_launch(const char *game_id) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return;

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(pc_server_ip);

    char packet[128];
    snprintf(packet, sizeof(packet), "LAUNCH:%s", game_id);

    // Send to default port (1999) AND GMC port (32105) for maximum compatibility
    serv.sin_port = htons(pc_udp_port);
    sendto(s, packet, strlen(packet), 0, (struct sockaddr*)&serv, sizeof(serv));

    serv.sin_port = htons(32105);
    sendto(s, packet, strlen(packet), 0, (struct sockaddr*)&serv, sizeof(serv));

    close(s);
}

void phantom_frontend_init() {
    load_config();
    init_catalog();
    load_state();
    marquee_timer_origin = get_time_ms();
    marquee_last_selected = selected_idx;
}

void phantom_frontend_on_connect() {
    is_launching = 0;
}

void phantom_frontend_on_disconnect() {
    is_launching = 0;
    save_state();
    marquee_timer_origin = get_time_ms();
    marquee_last_selected = selected_idx;
}

int phantom_frontend_is_launching() {
    return is_launching;
}

void phantom_frontend_render(unsigned char *fb, int width, int height) {
    if (!fb || width < FB_WIDTH || height < FB_HEIGHT) return;

    // Reset marquee timer when selection changes
    if (selected_idx != marquee_last_selected) {
        marquee_timer_origin = get_time_ms();
        marquee_last_selected = selected_idx;
    }

    // 1. Clear background (Deep arcade navy)
    draw_rect(fb, 0, 0, FB_WIDTH, FB_HEIGHT, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B);

    // 2. CRT Overscan Safety Border (14px horizontal inset, 6px vertical)
    const int safeX = 14;
    const int safeY = 6;
    const int safeW = FB_WIDTH - (safeX * 2);  // 228px
    const int safeH = FB_HEIGHT - (safeY * 2); // 228px

    // Outer card container
    draw_rect(fb, safeX, safeY, safeW, safeH, COLOR_CARD_R, COLOR_CARD_G, COLOR_CARD_B);
    draw_box(fb, safeX, safeY, safeW, safeH, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B);

    // 3. Header: "PHANTOM ARCADE" + CRT Modeline indicator
    int headY = safeY + 4;
    draw_text(fb, safeX + 6, headY, "PHANTOM ARCADE", COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
    // Green host LED
    draw_rect(fb, safeX + safeW - 58, headY + 3, 5, 5, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
    draw_text(fb, safeX + safeW - 50, headY, "15kHz", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

    // Separator line
    for (int i = safeX + 4; i < safeX + safeW - 4; i++) {
        put_pixel(fb, i, headY + 14, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B);
    }

    // 4. Categories bar (tabs)
    int catY = headY + 17;
    const char *cats[] = { "ALL", "MAME", "NEOGEO", "CPS", "NAOMI", "PS2" };
    int catX = safeX + 6;
    for (int c = 0; c < 6; c++) {
        if (c == current_cat) {
            int tw = (int)strlen(cats[c]) * 8 + 4;
            draw_rect(fb, catX - 2, catY - 1, tw, 13, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
            draw_text(fb, catX, catY, cats[c], 0, 0, 0); // Black text on amber pill
            catX += tw + 4;
        } else {
            draw_text(fb, catX, catY, cats[c], COLOR_GRAY_R, COLOR_GRAY_G, COLOR_GRAY_B);
            catX += (int)strlen(cats[c]) * 8 + 6;
        }
    }

    // 5. Game Selection List (8 visible rows)
    int listY = catY + 16;
    int visibleRows = 8;
    int rowH = 14;

    int startRow = selected_idx - 3;
    if (startRow < 0) startRow = 0;
    if (startRow > game_count - visibleRows) startRow = game_count - visibleRows;
    if (startRow < 0) startRow = 0;

    for (int r = 0; r < visibleRows; r++) {
        int gIdx = startRow + r;
        if (gIdx >= game_count) break;

        int ry = listY + (r * rowH);
        int is_sel = (gIdx == selected_idx);

        if (is_sel) {
            // Selected row: Amber-tinted background with glowing cursor border
            draw_rect(fb, safeX + 4, ry - 1, safeW - 8, rowH - 1, 0x3A, 0x2A, 0x10);
            draw_box(fb, safeX + 4, ry - 1, safeW - 8, rowH - 1, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
            draw_char(fb, safeX + 6, ry, '>', COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
            // Marquee scrolling for selected title
            draw_text_marquee(fb, safeX + 16, ry, 24, games[gIdx].title, COLOR_WHITE_R, COLOR_WHITE_G, COLOR_WHITE_B, 1);
        } else {
            // Non-selected row
            if (r % 2 == 1) {
                draw_rect(fb, safeX + 4, ry - 1, safeW - 8, rowH - 1, 0x11, 0x15, 0x22);
            }
            draw_text_marquee(fb, safeX + 16, ry, 24, games[gIdx].title, COLOR_GRAY_R, COLOR_GRAY_G, COLOR_GRAY_B, 0);
        }
    }

    // 6. Game Metadata & Video Modeline Box
    int infoY = listY + (visibleRows * rowH) + 4;
    int infoH = 26;
    draw_rect(fb, safeX + 4, infoY, safeW - 8, infoH, 0x0E, 0x12, 0x1E);
    draw_box(fb, safeX + 4, infoY, safeW - 8, infoH, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B);

    if (selected_idx >= 0 && selected_idx < game_count) {
        char line1[64];
        char line2[64];
        snprintf(line1, sizeof(line1), "[%s] %s", games[selected_idx].systemName, games[selected_idx].res);
        snprintf(line2, sizeof(line2), "SYNC: %dHz  %s", games[selected_idx].vSync / 100, games[selected_idx].mode);
        draw_text(fb, safeX + 8, infoY + 2, line1, COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);
        draw_text(fb, safeX + 8, infoY + 13, line2, COLOR_GRAY_R, COLOR_GRAY_G, COLOR_GRAY_B);
    }

    // 7. Footer / Controller Legend
    int footY = safeY + safeH - 15;
    draw_text(fb, safeX + 6, footY, "[1]LAUNCH  [ESC]MENU", COLOR_DARK_R, COLOR_DARK_G, COLOR_DARK_B);
    draw_text(fb, safeX + safeW - 74, footY, "PC:ACTIVE", COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);

    // 8. Launching Overlay Splash
    if (is_launching) {
        uint32_t now = get_time_ms();
        int elapsed_s = (now - launch_start_time) / 1000;

        int modalW = 200;
        int modalH = 50;
        int modalX = (FB_WIDTH - modalW) / 2;
        int modalY = (FB_HEIGHT - modalH) / 2;

        draw_rect(fb, modalX, modalY, modalW, modalH, 0x10, 0x14, 0x22);
        draw_box(fb, modalX, modalY, modalW, modalH, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);

        draw_text(fb, modalX + 16, modalY + 8,  "LAUNCHING STREAM...", COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);

        char launch_msg[64];
        if (selected_idx >= 0 && selected_idx < game_count) {
            snprintf(launch_msg, sizeof(launch_msg), "Starting: %s", games[selected_idx].id);
        } else {
            snprintf(launch_msg, sizeof(launch_msg), "Starting game...");
        }
        draw_text(fb, modalX + 16, modalY + 22, launch_msg, COLOR_WHITE_R, COLOR_WHITE_G, COLOR_WHITE_B);
        draw_text(fb, modalX + 16, modalY + 34, "Connecting to PC MAME...", COLOR_CYAN_R, COLOR_CYAN_G, COLOR_CYAN_B);

        // Auto timeout launching splash after 10s if PC did not connect
        if (elapsed_s > 10) {
            is_launching = 0;
        }
    }
}

static void trigger_launch() {
    if (selected_idx >= 0 && selected_idx < game_count) {
        is_launching = 1;
        launch_start_time = get_time_ms();
        save_state();
        send_udp_launch(games[selected_idx].id);
    }
}

void phantom_frontend_input_joystick(unsigned char joystick, uint32_t map) {
    if (is_launching) return;
    (void)joystick;

    // Standard MiSTer joystick bitmask:
    // bit 0: Right, bit 1: Left, bit 2: Down, bit 3: Up
    // bit 4: Btn 1 (A), bit 5: Btn 2 (B), bit 6: Btn 3 (X), bit 7: Btn 4 (Y)
    // bit 8: Select / Coin, bit 9: Start
    int up    = (map & (1 << 3)) != 0;
    int down  = (map & (1 << 2)) != 0;
    int left  = (map & (1 << 1)) != 0;
    int right = (map & (1 << 0)) != 0;
    int btn1  = (map & (1 << 4)) != 0;
    int start = (map & (1 << 9)) != 0;

    uint32_t now = get_time_ms();

    // Directional navigation with 0.5s hold delay and dynamic acceleration
    if (up || down) {
        int dir = up ? -1 : 1;
        if (last_direction != dir) {
            // Initial press
            last_direction = dir;
            hold_start_time = now;
            last_scroll_time = now;
            selected_idx += dir;
            if (selected_idx < 0) selected_idx = 0;
            if (selected_idx >= game_count) selected_idx = game_count - 1;
        } else {
            // Held down: check 500ms initial delay
            uint32_t hold_duration = now - hold_start_time;
            if (hold_duration >= 500) {
                // Accelerate: start at 160ms interval, reduce to 40ms
                uint32_t interval = 160;
                if (hold_duration > 1500) interval = 50;
                else if (hold_duration > 1000) interval = 90;

                if (now - last_scroll_time >= interval) {
                    last_scroll_time = now;
                    selected_idx += dir;
                    if (selected_idx < 0) selected_idx = 0;
                    if (selected_idx >= game_count) selected_idx = game_count - 1;
                }
            }
        }
    } else {
        last_direction = 0;
        hold_start_time = 0;
    }

    // Category switching: Left / Right
    static int prev_left = 0, prev_right = 0;
    if (left && !prev_left) {
        current_cat--;
        if (current_cat < 0) current_cat = 5;
    }
    if (right && !prev_right) {
        current_cat++;
        if (current_cat > 5) current_cat = 0;
    }
    prev_left = left;
    prev_right = right;

    // Launch: Button 1 or Start
    static int prev_btn1 = 0, prev_start = 0;
    if ((btn1 && !prev_btn1) || (start && !prev_start)) {
        trigger_launch();
    }
    prev_btn1 = btn1;
    prev_start = start;
}

void phantom_frontend_input_analog(unsigned char joystick, unsigned char analog, char valX, char valY) {
    if (is_launching || analog != 0) return; // Only listen to Left Stick
    (void)joystick;

    uint32_t map = 0;
    if (valY < -50) map |= (1 << 3); // Up
    if (valY > 50)  map |= (1 << 2); // Down
    if (valX < -50) map |= (1 << 1); // Left
    if (valX > 50)  map |= (1 << 0); // Right

    phantom_frontend_input_joystick(0, map);
}

void phantom_frontend_input_keyboard(uint16_t key, int press) {
    if (is_launching || !press) return;

    // Key codes from Linux / MiSTer:
    // Up: 103, Down: 108, Left: 105, Right: 106, Enter: 28, Space: 57, ESC: 1
    if (key == 103) { // Up
        selected_idx--;
        if (selected_idx < 0) selected_idx = 0;
    } else if (key == 108) { // Down
        selected_idx++;
        if (selected_idx >= game_count) selected_idx = game_count - 1;
    } else if (key == 105) { // Left
        current_cat--;
        if (current_cat < 0) current_cat = 5;
    } else if (key == 106) { // Right
        current_cat++;
        if (current_cat > 5) current_cat = 0;
    } else if (key == 28 || key == 57) { // Enter or Space
        trigger_launch();
    }
}
