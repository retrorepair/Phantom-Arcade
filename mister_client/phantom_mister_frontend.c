/**
 * Phantom Arcade - MiSTer FPGA Native CRT Framebuffer Graphical Launcher
 * Target: ARMv7 Linux (DE10-Nano / MiSTer FPGA)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>

// Embedded 8x8 standard font (ASCII 32..127)
static const unsigned char font8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 space
    {0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00}, // 33 !
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 "
    {0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00}, // 35 #
    {0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00}, // 36 $
    {0x00,0x63,0x66,0x0c,0x18,0x33,0x63,0x00}, // 37 %
    {0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0x00}, // 38 &
    {0x30,0x30,0x10,0x20,0x00,0x00,0x00,0x00}, // 39 '
    {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00}, // 40 (
    {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00}, // 41 )
    {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, // 42 *
    {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00}, // 43 +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ,
    {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00}, // 45 -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 .
    {0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00}, // 47 /
    {0x7c,0xc6,0xce,0xd6,0xe6,0xc6,0x7c,0x00}, // 48 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7e,0x00}, // 49 1
    {0x7c,0xc6,0x06,0x1c,0x30,0x66,0xfe,0x00}, // 50 2
    {0x7c,0xc6,0x06,0x3c,0x06,0xc6,0x7c,0x00}, // 51 3
    {0x1c,0x3c,0x6c,0xcc,0xfe,0x0c,0x1e,0x00}, // 52 4
    {0xfe,0xc0,0xfc,0x06,0x06,0xc6,0x7c,0x00}, // 53 5
    {0x78,0x0c,0xc0,0xfc,0xc6,0xc6,0x7c,0x00}, // 54 6
    {0xfe,0xc6,0x0c,0x18,0x30,0x30,0x30,0x00}, // 55 7
    {0x7c,0xc6,0xc6,0x7c,0xc6,0xc6,0x7c,0x00}, // 56 8
    {0x7c,0xc6,0xc6,0x7e,0x06,0x0c,0x78,0x00}, // 57 9
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 58 :
    {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // 59 ;
    {0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00}, // 60 <
    {0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00}, // 61 =
    {0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x00}, // 62 >
    {0x7c,0xc6,0x0c,0x18,0x18,0x00,0x18,0x00}, // 63 ?
    {0x7c,0xc6,0xde,0xde,0xdc,0xc0,0x7c,0x00}, // 64 @
    {0x38,0x6c,0xc6,0xc6,0xfe,0xc6,0xc6,0x00}, // 65 A
    {0xfc,0x66,0x66,0x7c,0x66,0x66,0xfc,0x00}, // 66 B
    {0x3c,0x66,0xc0,0xc0,0xc0,0x66,0x3c,0x00}, // 67 C
    {0xf8,0x6c,0x66,0x66,0x66,0x6c,0xf8,0x00}, // 68 D
    {0xfe,0x62,0x68,0x78,0x68,0x62,0xfe,0x00}, // 69 E
    {0xfe,0x62,0x68,0x78,0x68,0x60,0xf0,0x00}, // 70 F
    {0x3c,0x66,0xc0,0xc0,0xce,0x66,0x3e,0x00}, // 71 G
    {0xc6,0xc6,0xc6,0xfe,0xc6,0xc6,0xc6,0x00}, // 72 H
    {0x7e,0x18,0x18,0x18,0x18,0x18,0x7e,0x00}, // 73 I
    {0x1e,0x0c,0x0c,0x0c,0xcc,0xcc,0x78,0x00}, // 74 J
    {0xe6,0x66,0x6c,0x78,0x6c,0x66,0xe6,0x00}, // 75 K
    {0xf0,0x60,0x60,0x60,0x62,0x66,0xfe,0x00}, // 76 L
    {0xc6,0xee,0xfe,0xfe,0xd6,0xc6,0xc6,0x00}, // 77 M
    {0xc6,0xe6,0xf6,0xde,0xce,0xc6,0xc6,0x00}, // 78 N
    {0x7c,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x00}, // 79 O
    {0xfc,0x66,0x66,0x7c,0x60,0x60,0xf0,0x00}, // 80 P
    {0x7c,0xc6,0xc6,0xc6,0xd6,0xcc,0x7a,0x00}, // 81 Q
    {0xfc,0x66,0x66,0x7c,0x6c,0x66,0xe6,0x00}, // 82 R
    {0x7c,0xc6,0x60,0x3c,0x06,0xc6,0x7c,0x00}, // 83 S
    {0x7e,0x7e,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 T
    {0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x00}, // 85 U
    {0xc6,0xc6,0xc6,0xc6,0x6c,0x38,0x10,0x00}, // 86 V
    {0xc6,0xc6,0xd6,0xfe,0xfe,0xee,0xc6,0x00}, // 87 W
    {0xc6,0x6c,0x38,0x10,0x38,0x6c,0xc6,0x00}, // 88 X
    {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00}, // 89 Y
    {0xfe,0xc6,0x8c,0x18,0x32,0x66,0xfe,0x00}, // 90 Z
    {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00}, // 91 [
    {0xc0,0x60,0x30,0x18,0x0c,0x06,0x02,0x00}, // 92 \
    {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}, // 93 ]
    {0x10,0x38,0x6c,0xc6,0x00,0x00,0x00,0x00}, // 94 ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff}, // 95 _
    {0x30,0x18,0x0c,0x00,0x00,0x00,0x00,0x00}, // 96 `
    {0x00,0x00,0x78,0x0c,0x7c,0xcc,0x76,0x00}, // 97 a
    {0xe0,0x60,0x7c,0x66,0x66,0x66,0xdc,0x00}, // 98 b
    {0x00,0x00,0x7c,0xc6,0xc0,0xc6,0x7c,0x00}, // 99 c
    {0x1c,0x0c,0x7c,0xcc,0xcc,0xcc,0x76,0x00}, // 100 d
    {0x00,0x00,0x7c,0xc6,0xfe,0xc0,0x7c,0x00}, // 101 e
    {0x1c,0x36,0x30,0x78,0x30,0x30,0x78,0x00}, // 102 f
    {0x00,0x00,0x76,0xcc,0xcc,0x7c,0x0c,0xf8}, // 103 g
    {0xe0,0x60,0x6c,0x76,0x66,0x66,0xe6,0x00}, // 104 h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3c,0x00}, // 105 i
    {0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3c}, // 106 j
    {0xe0,0x60,0x66,0x6c,0x78,0x6c,0xe6,0x00}, // 107 k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // 108 l
    {0x00,0x00,0xec,0xfe,0xd6,0xc6,0xc6,0x00}, // 109 m
    {0x00,0x00,0xdc,0x66,0x66,0x66,0x66,0x00}, // 110 n
    {0x00,0x00,0x7c,0xc6,0xc6,0xc6,0x7c,0x00}, // 111 o
    {0x00,0x00,0xdc,0x66,0x66,0x7c,0x60,0xf0}, // 112 p
    {0x00,0x00,0x76,0xcc,0xcc,0x7c,0x0c,0x1e}, // 113 q
    {0x00,0x00,0xdc,0x76,0x60,0x60,0xf0,0x00}, // 114 r
    {0x00,0x00,0x7e,0xc0,0x7c,0x06,0xfc,0x00}, // 115 s
    {0x30,0x30,0xfc,0x30,0x30,0x34,0x18,0x00}, // 116 t
    {0x00,0x00,0xcc,0xcc,0xcc,0xcc,0x76,0x00}, // 117 u
    {0x00,0x00,0xc6,0xc6,0xc6,0x6c,0x38,0x00}, // 118 v
    {0x00,0x00,0xc6,0xd6,0xfe,0xee,0xc6,0x00}, // 119 w
    {0x00,0x00,0xc6,0x6c,0x38,0x6c,0xc6,0x00}, // 120 x
    {0x00,0x00,0xc6,0xc6,0xc6,0x7e,0x06,0xfc}, // 121 y
    {0x00,0x00,0x7e,0x4c,0x18,0x32,0x7e,0x00}, // 122 z
    {0x0e,0x18,0x18,0x70,0x18,0x18,0x0e,0x00}, // 123 {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 |
    {0x70,0x18,0x18,0x0e,0x18,0x18,0x70,0x00}, // 125 }
    {0x76,0xdc,0x00,0x00,0x00,0x00,0x00,0x00}, // 126 ~
    {0x00,0x10,0x38,0x7c,0xfe,0x7c,0x38,0x10}  // 127 ▶ cursor
};


#define DEFAULT_PC_IP    "192.168.1.126"
#define DEFAULT_UDP_PORT 1999
#define GROOVY_CORE_PATH "/media/fat/_Utility/Groovy.rbf"
#define PHANTOM_CORE_PATH "/media/fat/_Arcade/Phantom_Arcade.rbf"
#define MENU_CORE_PATH   "/media/fat/menu.rbf"

// Palette (32-bit ARGB / RGB888)
#define COLOR_BG           0x0A0A0E // Deep arcade CRT black
#define COLOR_HEADER_BG    0x141419
#define COLOR_CARD_BG      0x181820
#define COLOR_AMBER_BRIGHT 0xFBBF24 // Amber 400
#define COLOR_AMBER_DARK   0x78350F // Amber 900
#define COLOR_AMBER_GLOW   0xD97706 // Amber 600
#define COLOR_WHITE        0xF4F4F5
#define COLOR_GRAY_LIGHT   0xA1A1AA
#define COLOR_GRAY_DARK    0x3F3F46
#define COLOR_ROW_SEL      0x272732
#define COLOR_BORDER       0x2E2E38
#define COLOR_GREEN_LED    0x22C55E
#define COLOR_CYAN_BADGE   0x06B6D4

typedef struct {
    char id[64];
    char title[64];
    char system[32];
    char res[32];
    char tag[32];
} GameEntry;

static GameEntry games[128];
static int gameCount = 0;

static int fbfd = -1;
static char *fbp = NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static long screensize = 0;
static char pc_ip[64] = DEFAULT_PC_IP;
static int udp_port = DEFAULT_UDP_PORT;

// Pixel drawing primitive
static inline void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)vinfo.xres || y < 0 || y >= (int)vinfo.yres) return;
    long location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                    (y + vinfo.yoffset) * finfo.line_length;

    if (vinfo.bits_per_pixel == 32) {
        *((uint32_t*)(fbp + location)) = color;
    } else if (vinfo.bits_per_pixel == 16) {
        uint16_t r = (color >> 19) & 0x1F;
        uint16_t g = (color >> 10) & 0x3F;
        uint16_t b = (color >> 3) & 0x1F;
        *((uint16_t*)(fbp + location)) = (r << 11) | (g << 5) | b;
    }
}

// Rectangle fill primitive
static void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            put_pixel(i, j, color);
        }
    }
}

// Draw single 8x8 character with scaling
static void draw_char(int x, int y, char c, uint32_t fg, uint32_t bg, int scale) {
    int idx = (unsigned char)c - 32;
    if (idx < 0 || idx >= 96) idx = 31; // '?' fallback
    const unsigned char *glyph = font8x8[idx];

    for (int r = 0; r < 8; r++) {
        unsigned char row = glyph[r];
        for (int c_idx = 0; c_idx < 8; c_idx++) {
            int bit = (row >> (7 - c_idx)) & 1;
            uint32_t color = bit ? fg : bg;
            if (color != 0) {
                if (scale == 1) {
                    put_pixel(x + c_idx, y + r, color);
                } else {
                    for (int dy = 0; dy < scale; dy++) {
                        for (int dx = 0; dx < scale; dx++) {
                            put_pixel(x + c_idx * scale + dx, y + r * scale + dy, color);
                        }
                    }
                }
            }
        }
    }
}

// Draw text string
static void draw_text(int x, int y, const char *str, uint32_t fg, uint32_t bg, int scale) {
    int cur_x = x;
    while (*str) {
        draw_char(cur_x, y, *str, fg, bg, scale);
        cur_x += 8 * scale;
        str++;
    }
}

// Load config from /media/fat/config/phantom.ini
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
            if (strlen(val) > 0) strncpy(pc_ip, val, sizeof(pc_ip) - 1);
        } else if (strncmp(line, "UDP_PORT=", 9) == 0) {
            udp_port = atoi(line + 9);
            if (udp_port <= 0) udp_port = DEFAULT_UDP_PORT;
        }
    }
    fclose(f);
}

// Initialize default game catalog
static void init_default_catalog() {
    gameCount = 0;
    #define ADD_G(gid, gname, gsys, gres, gtag) do { \
        strncpy(games[gameCount].id, gid, 63); \
        strncpy(games[gameCount].title, gname, 63); \
        strncpy(games[gameCount].system, gsys, 31); \
        strncpy(games[gameCount].res, gres, 31); \
        strncpy(games[gameCount].tag, gtag, 31); \
        gameCount++; \
    } while (0)

    ADD_G("kinst", "Killer Instinct", "GroovyMAME", "240p @ 60Hz", "15.7kHz Native");
    ADD_G("kinst2", "Killer Instinct 2", "GroovyMAME", "240p @ 60Hz", "15.7kHz Native");
    ADD_G("sfiii3", "Street Fighter III: 3rd Strike", "GroovyMAME", "224p @ 59.6Hz", "15.7kHz Native");
    ADD_G("mk2", "Mortal Kombat II", "GroovyMAME", "254p @ 54.7Hz", "Midway 15kHz");
    ADD_G("ps2_cvs2", "Capcom vs. SNK 2 (EO)", "PS2 / PCSX2", "240p @ 60Hz", "15.7kHz CRT");
    ADD_G("ps2_3rdstrike", "Street Fighter III Anniversary", "PS2 / PCSX2", "240p @ 60Hz", "15.7kHz CRT");
    ADD_G("ps2_arcana", "Arcana Heart", "PS2 / PCSX2", "240p @ 60Hz", "15.7kHz CRT");
    ADD_G("gc_melee", "Super Smash Bros. Melee", "GameCube", "480i @ 60Hz", "15kHz Interlaced");
    ADD_G("naomi_vf4ft", "Virtua Fighter 4 Final Tuned", "Naomi / Flycast", "480i / 240p", "15kHz CRT");
    ADD_G("model2_daytona", "Daytona USA", "Model 2 / Emu", "384p @ 60Hz", "24kHz Med-Res");
}

// Render the complete CRT GUI
static void render_gui(int selIdx, int currentTab) {
    int w = vinfo.xres;
    int h = vinfo.yres;

    // Background
    fill_rect(0, 0, w, h, COLOR_BG);

    // Header bar (34px)
    fill_rect(0, 0, w, 34, COLOR_HEADER_BG);
    fill_rect(0, 34, w, 2, COLOR_AMBER_BRIGHT);

    // Glowing LED
    fill_rect(14, 13, 8, 8, COLOR_GREEN_LED);
    fill_rect(13, 12, 10, 10, 0x114422);

    // Header text
    draw_text(30, 9, "PHANTOM ARCADE", COLOR_AMBER_BRIGHT, 0, 2);
    char subheader[128];
    snprintf(subheader, sizeof(subheader), "15.7kHz CRT LAUNCHER  |  HOST: %s:%d", pc_ip, udp_port);
    draw_text(240, 14, subheader, COLOR_GRAY_LIGHT, 0, 1);

    // Tab Bar (26px)
    fill_rect(0, 36, w, 26, 0x121218);
    const char *tabs[] = { "ALL GAMES", "GROOVYMAME", "PLAYSTATION 2", "GAMECUBE", "NAOMI" };
    int tabX = 14;
    for (int t = 0; t < 5; t++) {
        int tabW = strlen(tabs[t]) * 8 + 16;
        if (t == currentTab) {
            fill_rect(tabX, 40, tabW, 18, COLOR_AMBER_BRIGHT);
            draw_text(tabX + 8, 45, tabs[t], 0x000000, 0, 1);
        } else {
            fill_rect(tabX, 40, tabW, 18, 0x1E1E28);
            draw_text(tabX + 8, 45, tabs[t], COLOR_GRAY_LIGHT, 0, 1);
        }
        tabX += tabW + 6;
    }
    fill_rect(0, 62, w, 1, COLOR_BORDER);

    // Game rows
    int startY = 72;
    int rowH = 26;
    int maxRows = (h - 170) / rowH;
    if (maxRows > gameCount) maxRows = gameCount;

    int scrollOffset = 0;
    if (selIdx >= maxRows) {
        scrollOffset = selIdx - maxRows + 1;
    }

    for (int i = 0; i < maxRows; i++) {
        int gIdx = i + scrollOffset;
        if (gIdx >= gameCount) break;

        int rowY = startY + i * rowH;
        int isSel = (gIdx == selIdx);

        if (isSel) {
            fill_rect(12, rowY, w - 24, rowH - 2, COLOR_ROW_SEL);
            fill_rect(12, rowY, 4, rowH - 2, COLOR_AMBER_BRIGHT);
            draw_char(20, rowY + 5, (char)127, COLOR_AMBER_BRIGHT, 0, 2); // '▶'
            draw_text(42, rowY + 7, games[gIdx].title, COLOR_WHITE, 0, 1);
        } else {
            fill_rect(12, rowY, w - 24, rowH - 2, 0x111116);
            draw_text(42, rowY + 7, games[gIdx].title, COLOR_GRAY_LIGHT, 0, 1);
        }

        // System pill badge
        int sysX = w - 310;
        fill_rect(sysX, rowY + 4, 110, 16, 0x222230);
        draw_text(sysX + 6, rowY + 8, games[gIdx].system, COLOR_CYAN_BADGE, 0, 1);

        // Resolution badge
        int resX = w - 190;
        fill_rect(resX, rowY + 4, 166, 16, isSel ? COLOR_AMBER_DARK : 0x181822);
        draw_text(resX + 6, rowY + 8, games[gIdx].tag, isSel ? COLOR_AMBER_BRIGHT : COLOR_GRAY_LIGHT, 0, 1);
    }

    // Selected game info box (Bottom area)
    int boxY = h - 90;
    fill_rect(12, boxY, w - 24, 52, COLOR_CARD_BG);
    fill_rect(12, boxY, w - 24, 1, COLOR_AMBER_GLOW);

    char selInfo[128];
    snprintf(selInfo, sizeof(selInfo), "READY TO LAUNCH: %s [%s]", games[selIdx].title, games[selIdx].id);
    draw_text(24, boxY + 10, selInfo, COLOR_AMBER_BRIGHT, 0, 1);

    char resInfo[128];
    snprintf(resInfo, sizeof(resInfo), "VIDEO TIMINGS: %s  |  MODE: SwitchRes CRT Pixel-Clock Synced", games[selIdx].res);
    draw_text(24, boxY + 28, resInfo, COLOR_GRAY_LIGHT, 0, 1);

    // Help Footer (26px)
    fill_rect(0, h - 28, w, 28, 0x0D0D12);
    fill_rect(0, h - 28, w, 1, COLOR_BORDER);
    draw_text(20, h - 20, "UP/DOWN: Browse | ENTER / P1 BTN: Launch RBF Core | Q / ESC: Exit to MiSTer Menu", COLOR_GRAY_LIGHT, 0, 1);
}

// Dispatch UDP launch command to host PC
static void send_udp_launch(const char *game_id) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return;

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(udp_port);
    servaddr.sin_addr.s_addr = inet_addr(pc_ip);

    char packet[128];
    snprintf(packet, sizeof(packet), "LAUNCH:%s", game_id);
    sendto(sockfd, packet, strlen(packet), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    close(sockfd);
    printf("[+] Sent '%s' to %s:%d\n", packet, pc_ip, udp_port);
}

// Load FPGA core via /dev/MiSTer_cmd
static void switch_fpga_core(const char *core_path) {
    printf("[+] Requesting FPGA core load: %s\n", core_path);
    int cmdFd = open("/dev/MiSTer_cmd", O_WRONLY);
    if (cmdFd >= 0) {
        dprintf(cmdFd, "load_core %s\n", core_path);
        close(cmdFd);
    } else {
        // Fallback: execute mister core loader if command device not available
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "echo 'load_core %s' > /dev/MiSTer_cmd 2>/dev/null", core_path);
        system(cmd);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) strncpy(pc_ip, argv[1], sizeof(pc_ip) - 1);
    load_config();
    init_default_catalog();

    // Open Framebuffer
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open /dev/fb0");
        return 1;
    }

    ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((intptr_t)fbp == -1) {
        perror("Error: cannot mmap framebuffer");
        close(fbfd);
        return 1;
    }

    int selected = 0;
    int currentTab = 0;
    render_gui(selected, currentTab);

    // Open input devices
    int inputFds[8];
    int inputCount = 0;
    for (int i = 0; i < 8; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            inputFds[inputCount++] = fd;
        }
    }

    int running = 1;
    while (running) {
        int need_redraw = 0;

        for (int i = 0; i < inputCount; i++) {
            struct input_event ev;
            while (read(inputFds[i], &ev, sizeof(ev)) > 0) {
                if (ev.type == EV_KEY && ev.value == 1) {
                    if (ev.code == KEY_UP) {
                        if (selected > 0) { selected--; need_redraw = 1; }
                    } else if (ev.code == KEY_DOWN) {
                        if (selected < gameCount - 1) { selected++; need_redraw = 1; }
                    } else if (ev.code == KEY_LEFT) {
                        if (currentTab > 0) { currentTab--; need_redraw = 1; }
                    } else if (ev.code == KEY_RIGHT) {
                        if (currentTab < 4) { currentTab++; need_redraw = 1; }
                    } else if (ev.code == KEY_ENTER || ev.code == BTN_TRIGGER || ev.code == BTN_A || ev.code == BTN_SOUTH) {
                        // Launch game
                        send_udp_launch(games[selected].id);

                        // Check which RBF core exists and switch to it
                        if (access(PHANTOM_CORE_PATH, F_OK) == 0) {
                            switch_fpga_core(PHANTOM_CORE_PATH);
                        } else {
                            switch_fpga_core(GROOVY_CORE_PATH);
                        }
                        running = 0;
                        break;
                    } else if (ev.code == KEY_Q || ev.code == KEY_ESC || ev.code == KEY_F12) {
                        switch_fpga_core(MENU_CORE_PATH);
                        running = 0;
                        break;
                    }
                }
            }
        }

        if (need_redraw) {
            render_gui(selected, currentTab);
        }

        usleep(16666); // ~60 FPS polling
    }

    munmap(fbp, screensize);
    close(fbfd);
    for (int i = 0; i < inputCount; i++) close(inputFds[i]);
    return 0;
}
