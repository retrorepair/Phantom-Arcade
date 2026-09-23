/**
 * ============================================================================
 * Phantom Arcade - High-Fidelity CRT Framebuffer Arcade Frontend for MiSTer FPGA
 * File: phantom_mister_frontend.c
 * Target: ARMv7 Linux (DE10-Nano / MiSTer FPGA) - Statically Linked
 *
 * Features:
 *   - Proportional arcade typography with drop-shadowed CRT rendering
 *   - Embedded pixel-art bitmaps (arcade marquee, cabinet badges, CRT icons)
 *   - Auto-adaptive resolution engine (supports 320x240 240p, 640x480 480i, 720p/1080p)
 *   - Double-buffered flicker-free rendering (RGB565 and ARGB8888)
 *   - Dynamic UDP catalog loading from Windows Host & live game launch
 *   - Seamless FPGA core loading via /dev/MiSTer_cmd
 * ============================================================================
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
#include <stdint.h>

#define DEFAULT_PC_IP    "192.168.1.126"
#define DEFAULT_UDP_PORT 1999
#define GROOVY_CORE_UTILITY "/media/fat/_Utility/Groovy.rbf"
#define GROOVY_CORE_ARCADE  "/media/fat/_Arcade/Groovy.rbf"
#define MENU_CORE_PATH      "/media/fat/menu.rbf"

// Palette Constants (32-bit ARGB 0xRRGGBB)
#define COLOR_BLACK        0x000000
#define COLOR_BG_DARK      0x08080C
#define COLOR_HEADER_BG    0x12121B
#define COLOR_CARD_BG      0x13131D
#define COLOR_CARD_BORDER  0x28283C
#define COLOR_AMBER_BRIGHT 0xFBBF24
#define COLOR_AMBER_DARK   0x78350F
#define COLOR_AMBER_GLOW   0xD97706
#define COLOR_GOLD         0xF59E0B
#define COLOR_CYAN_NEON    0x06B6D4
#define COLOR_CYAN_DARK    0x0E3A47
#define COLOR_MAGENTA      0xEC4899
#define COLOR_GREEN_LED    0x10B981
#define COLOR_GREEN_DARK   0x064E3B
#define COLOR_RED_BTN      0xEF4444
#define COLOR_BLUE_BTN     0x3B82F6
#define COLOR_YELLOW_BTN   0xEAB308
#define COLOR_WHITE        0xF8FAFC
#define COLOR_GRAY_LIGHT   0xCBD5E1
#define COLOR_GRAY_MID     0x64748B
#define COLOR_GRAY_DARK    0x1E293B
#define COLOR_ROW_SEL      0x1E1E2E
#define COLOR_ROW_SEL_LINE 0xF59E0B

// Game Entry structure
typedef struct {
    char id[64];
    char title[64];
    char system[32];
    char systemName[32];
    char res[32];
    char mode[32];
    int  hSync; // e.g. 15734 Hz
    int  vSync; // e.g. 6000 (60.00 Hz)
} GameEntry;

static GameEntry games[256];
static int gameCount = 0;

// Framebuffer State
static int fbfd = -1;
static char *fbp = NULL;
static uint32_t *backbuffer = NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static long screensize = 0;
static char pc_ip[64] = DEFAULT_PC_IP;
static int udp_port = DEFAULT_UDP_PORT;

// Standard clean 8x12 font glyphs (ASCII 32 to 126)
#include "font_arcade.h"
// Embedded pixel art icons (Cabinet, CRT, Badges)
#include "arcade_bitmaps.h"

// Put pixel on 32-bit internal backbuffer
static inline void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)vinfo.xres || y < 0 || y >= (int)vinfo.yres) return;
    backbuffer[y * vinfo.xres + x] = color;
}

// Rectangle fill on backbuffer
static void fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)vinfo.xres) w = (int)vinfo.xres - x;
    if (y + h > (int)vinfo.yres) h = (int)vinfo.yres - y;
    if (w <= 0 || h <= 0) return;

    for (int j = y; j < y + h; j++) {
        uint32_t *row = &backbuffer[j * vinfo.xres + x];
        for (int i = 0; i < w; i++) {
            row[i] = color;
        }
    }
}

// Horizontal gradient fill
static void fill_hgradient(int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
    if (w <= 0 || h <= 0) return;
    int r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    int r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;

    for (int i = 0; i < w; i++) {
        int r = r1 + (r2 - r1) * i / w;
        int g = g1 + (g2 - g1) * i / w;
        int b = b1 + (b2 - b1) * i / w;
        uint32_t col = (r << 16) | (g << 8) | b;
        for (int j = y; j < y + h; j++) {
            put_pixel(x + i, j, col);
        }
    }
}

// Draw single 8x12 character
static void draw_char_8x12(int x, int y, char c, uint32_t fg, int scale) {
    int idx = (unsigned char)c - 32;
    if (idx < 0 || idx >= 95) idx = 31; // ?
    const uint8_t *glyph = font8x12[idx];

    for (int r = 0; r < 12; r++) {
        uint8_t row = glyph[r];
        for (int b = 0; b < 8; b++) {
            if ((row >> (7 - b)) & 1) {
                if (scale == 1) {
                    put_pixel(x + b, y + r, fg);
                } else {
                    for (int dy = 0; dy < scale; dy++) {
                        for (int dx = 0; dx < scale; dx++) {
                            put_pixel(x + b * scale + dx, y + r * scale + dy, fg);
                        }
                    }
                }
            }
        }
    }
}

// Draw text with professional arcade drop shadow
static void draw_text_shadowed(int x, int y, const char *str, uint32_t fg, uint32_t shadowCol, int scale) {
    if (!str) return;
    int cur_x = x;
    int shadowOffset = (scale > 1) ? 2 : 1;

    // Draw shadow first
    if (shadowCol != 0) {
        int sx = x + shadowOffset;
        int sy = y + shadowOffset;
        const char *p = str;
        while (*p) {
            draw_char_8x12(sx, sy, *p, shadowCol, scale);
            sx += 8 * scale;
            p++;
        }
    }

    // Draw main text
    while (*str) {
        draw_char_8x12(cur_x, y, *str, fg, scale);
        cur_x += 8 * scale;
        str++;
    }
}

// Draw clipped text so it never overruns a maximum pixel width
static void draw_text_clipped(int x, int y, const char *str, uint32_t fg, uint32_t shadowCol, int max_w, int scale) {
    if (!str || max_w <= 0) return;
    int char_w = 8 * scale;
    int max_chars = max_w / char_w;
    if (max_chars <= 0) return;

    int len = (int)strlen(str);
    if (len <= max_chars) {
        draw_text_shadowed(x, y, str, fg, shadowCol, scale);
    } else {
        char buf[128];
        int copy_len = max_chars - 3;
        if (copy_len < 1) copy_len = 1;
        if (copy_len > 120) copy_len = 120;
        strncpy(buf, str, copy_len);
        buf[copy_len] = 0;
        strcat(buf, "...");
        draw_text_shadowed(x, y, buf, fg, shadowCol, scale);
    }
}

// Blit 16x16 icon bitmap with scale
static void draw_icon_16x16(int x, int y, const uint32_t *icon, int scale) {
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c++) {
            uint32_t col = icon[r * 16 + c];
            if (col != 0) { // 0 is transparent
                if (scale == 1) {
                    put_pixel(x + c, y + r, col);
                } else {
                    for (int dy = 0; dy < scale; dy++) {
                        for (int dx = 0; dx < scale; dx++) {
                            put_pixel(x + c * scale + dx, y + r * scale + dy, col);
                        }
                    }
                }
            }
        }
    }
}

// Copy backbuffer to real Linux framebuffer
static void flip_framebuffer() {
    int w = vinfo.xres;
    int h = vinfo.yres;

    if (vinfo.bits_per_pixel == 32) {
        for (int y = 0; y < h; y++) {
            long location = (vinfo.xoffset * 4) + (y + vinfo.yoffset) * finfo.line_length;
            memcpy(fbp + location, &backbuffer[y * w], w * 4);
        }
    } else if (vinfo.bits_per_pixel == 16) {
        for (int y = 0; y < h; y++) {
            long location = (vinfo.xoffset * 2) + (y + vinfo.yoffset) * finfo.line_length;
            uint16_t *dst = (uint16_t*)(fbp + location);
            uint32_t *src = &backbuffer[y * w];
            for (int x = 0; x < w; x++) {
                uint32_t c = src[x];
                uint16_t r = (c >> 19) & 0x1F;
                uint16_t g = (c >> 10) & 0x3F;
                uint16_t b = (c >> 3) & 0x1F;
                dst[x] = (r << 11) | (g << 5) | b;
            }
        }
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

// Initialize rich default game catalog
static void init_default_catalog() {
    gameCount = 0;
    #define ADD_ARCADE(gid, gname, gsys, gsysname, gres, gmode, gh, gv) do {         strncpy(games[gameCount].id, gid, 63);         strncpy(games[gameCount].title, gname, 63);         strncpy(games[gameCount].system, gsys, 31);         strncpy(games[gameCount].systemName, gsysname, 31);         strncpy(games[gameCount].res, gres, 31);         strncpy(games[gameCount].mode, gmode, 31);         games[gameCount].hSync = gh;         games[gameCount].vSync = gv;         gameCount++;     } while (0)

    ADD_ARCADE("kinst", "Killer Instinct", "groovymame", "GroovyMAME", "240p @ 60.0Hz", "15.7kHz Native", 15734, 6000);
    ADD_ARCADE("kinst2", "Killer Instinct 2", "groovymame", "GroovyMAME", "240p @ 60.0Hz", "15.7kHz Native", 15734, 6000);
    ADD_ARCADE("sfiii3", "Street Fighter III: 3rd Strike", "groovymame", "Capcom CPS-3", "224p @ 59.6Hz", "15.7kHz Native", 15600, 5963);
    ADD_ARCADE("mk2", "Mortal Kombat II", "groovymame", "Midway Y-Unit", "254p @ 54.7Hz", "15.2kHz Native", 15200, 5471);
    ADD_ARCADE("umk3", "Ultimate Mortal Kombat 3", "groovymame", "Midway Wolf", "254p @ 54.7Hz", "15.2kHz Native", 15200, 5471);
    ADD_ARCADE("mvsc", "Marvel vs. Capcom", "groovymame", "Capcom CPS-2", "224p @ 59.6Hz", "15.7kHz Native", 15600, 5963);
    ADD_ARCADE("garou", "Garou: Mark of the Wolves", "groovymame", "SNK Neo-Geo", "224p @ 59.2Hz", "15.7kHz Native", 15625, 5918);
    ADD_ARCADE("mslug3", "Metal Slug 3", "groovymame", "SNK Neo-Geo", "224p @ 59.2Hz", "15.7kHz Native", 15625, 5918);
    ADD_ARCADE("naomi_vf4ft", "Virtua Fighter 4 Final Tuned", "flycast", "Sega NAOMI", "480i / 240p", "15.7kHz CRT", 15734, 6000);
    ADD_ARCADE("naomi_cvs2", "Capcom vs. SNK 2", "flycast", "Sega NAOMI", "240p @ 60.0Hz", "15.7kHz CRT", 15734, 6000);
    ADD_ARCADE("naomi_ikaruga", "Ikaruga (Arcade)", "flycast", "Sega NAOMI", "240p Tate CRT", "15.7kHz CRT", 15734, 6000);
    ADD_ARCADE("ps2_3rdstrike", "Street Fighter III Anniversary", "pcsx2", "PlayStation 2", "240p @ 60.0Hz", "15.7kHz CRT", 15734, 6000);
    ADD_ARCADE("ps2_arcana", "Arcana Heart", "pcsx2", "PlayStation 2", "240p @ 60.0Hz", "15.7kHz CRT", 15734, 6000);
    ADD_ARCADE("ps2_espgaluda", "Espgaluda", "pcsx2", "PlayStation 2", "240p @ 60.0Hz", "15.7kHz CRT", 15734, 6000);
    ADD_ARCADE("gc_melee", "Super Smash Bros. Melee", "dolphin", "GameCube", "480i @ 60.0Hz", "15kHz Interlaced", 15734, 6000);
    ADD_ARCADE("gc_fzero_gx", "F-Zero GX", "dolphin", "GameCube", "480i @ 60.0Hz", "15kHz Interlaced", 15734, 6000);
    ADD_ARCADE("model2_daytona", "Daytona USA", "model2", "Sega Model 2", "384p @ 60.0Hz", "24.8kHz Med-Res", 24800, 6000);
}

// Fetch live game catalog from PC via UDP
static void fetch_catalog_from_pc() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000; // 250ms timeout
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(udp_port);
    serv.sin_addr.s_addr = inet_addr(pc_ip);

    sendto(s, "GET_CATALOG", 11, 0, (struct sockaddr*)&serv, sizeof(serv));

    char buf[65535];
    int n = recv(s, buf, sizeof(buf) - 1, 0);
    close(s);

    if (n > 20) {
        buf[n] = 0;
        // Simple fast JSON catalog parser
        char *p = buf;
        int parsed = 0;
        while ((p = strstr(p, "\"id\":")) != NULL && parsed < 250) {
            char *idStart = strchr(p + 5, '"');
            if (!idStart) break;
            char *idEnd = strchr(idStart + 1, '"');
            if (!idEnd) break;

            char gid[64];
            int idLen = idEnd - (idStart + 1);
            if (idLen > 63) idLen = 63;
            strncpy(gid, idStart + 1, idLen);
            gid[idLen] = 0;

            // Title
            char title[64] = "";
            char *tPos = strstr(p, "\"title\":");
            if (tPos && tPos < strstr(p, "}")) {
                char *tStart = strchr(tPos + 8, '"');
                if (tStart) {
                    char *tEnd = strchr(tStart + 1, '"');
                    if (tEnd) {
                        int tLen = tEnd - (tStart + 1);
                        if (tLen > 63) tLen = 63;
                        strncpy(title, tStart + 1, tLen);
                        title[tLen] = 0;
                    }
                }
            }

            // System
            char sys[32] = "groovymame";
            char *sPos = strstr(p, "\"system\":");
            if (sPos && sPos < strstr(p, "}")) {
                char *sStart = strchr(sPos + 9, '"');
                if (sStart) {
                    char *sEnd = strchr(sStart + 1, '"');
                    if (sEnd) {
                        int sLen = sEnd - (sStart + 1);
                        if (sLen > 31) sLen = 31;
                        strncpy(sys, sStart + 1, sLen);
                        sys[sLen] = 0;
                    }
                }
            }

            if (strlen(title) > 0) {
                strncpy(games[parsed].id, gid, 63);
                strncpy(games[parsed].title, title, 63);
                strncpy(games[parsed].system, sys, 31);
                strncpy(games[parsed].systemName, sys, 31);
                strncpy(games[parsed].res, "15.7kHz Native", 31);
                strncpy(games[parsed].mode, "Dynamic SwitchRes", 31);
                games[parsed].hSync = 15734;
                games[parsed].vSync = 6000;
                parsed++;
            }
            p = idEnd + 1;
        }

        if (parsed > 0) {
            gameCount = parsed;
        }
    }
}

// Render Complete High-Impact Arcade CRT Interface
static void render_gui(int selIdx, int currentCategory) {
    int w = vinfo.xres;
    int h = vinfo.yres;

    // 1. Dark CRT Arcade Background
    fill_rect(0, 0, w, h, COLOR_BG_DARK);

    // 2. Marquee Header (Illuminated Arcade Header)
    int headerH = (h >= 400) ? 44 : 26;
    fill_hgradient(0, 0, w, headerH, 0x1A1424, 0x0F111E);
    fill_rect(0, headerH - 2, w, 2, COLOR_AMBER_BRIGHT);

    // Cabinet Icon + Marquee Title
    int titleScale = (h >= 400) ? 2 : 1;
    draw_icon_16x16(10, (headerH - 16 * titleScale) / 2, icon_cabinet, titleScale);
    draw_text_shadowed(14 + 20 * titleScale, (headerH - 12 * titleScale) / 2, "PHANTOM ARCADE", COLOR_AMBER_BRIGHT, COLOR_AMBER_DARK, titleScale);

    // Status pill on the right
    char hostStatus[64];
    snprintf(hostStatus, sizeof(hostStatus), "HOST: %s:%d", pc_ip, udp_port);
    int statusW = (int)strlen(hostStatus) * 8 + 24;
    int statusX = w - statusW - 8;
    if (statusX > 200) {
        fill_rect(statusX, 4, statusW, headerH - 8, 0x1B212D);
        fill_rect(statusX + 6, headerH / 2 - 3, 6, 6, COLOR_GREEN_LED);
        draw_text_shadowed(statusX + 16, headerH / 2 - 5, hostStatus, COLOR_GRAY_LIGHT, 0, 1);
    }

    // 3. Platform Category Filter Tabs (e.g. ALL, GROOVYMAME, NAOMI, PS2, GC)
    const char *tabs[] = { "ALL", "MAME 15K", "NAOMI", "PS2", "GAMECUBE" };
    int tabCount = 5;
    int tabY = headerH + 2;
    int tabH = (h >= 400) ? 24 : 16;
    fill_rect(0, tabY, w, tabH, 0x111118);

    int curTabX = 10;
    for (int t = 0; t < tabCount; t++) {
        int tLen = (int)strlen(tabs[t]);
        int tWidth = tLen * 8 + 16;
        if (t == currentCategory) {
            fill_rect(curTabX, tabY + 2, tWidth, tabH - 4, COLOR_AMBER_BRIGHT);
            draw_text_shadowed(curTabX + 8, tabY + (tabH - 12) / 2, tabs[t], COLOR_BLACK, 0, 1);
        } else {
            fill_rect(curTabX, tabY + 2, tWidth, tabH - 4, 0x1D1D28);
            draw_text_shadowed(curTabX + 8, tabY + (tabH - 12) / 2, tabs[t], COLOR_GRAY_MID, 0, 1);
        }
        curTabX += tWidth + 6;
    }
    fill_rect(0, tabY + tabH, w, 1, COLOR_CARD_BORDER);

    // 4. Layout: Check if 240p (Single Column) or 480i/480p+ (Two Column Split)
    int isTwoColumn = (w >= 500 && h >= 400);
    int footerH = (h >= 400) ? 30 : 20;

    int listY = tabY + tabH + 4;
    int listH = h - listY - footerH - 4;
    int listW = isTwoColumn ? (w * 58 / 100) : (w - 16);

    int rowH = (h >= 400) ? 28 : 18;
    int maxVisibleRows = listH / rowH;
    if (maxVisibleRows < 1) maxVisibleRows = 1;

    // Filter games by category
    int visibleIndices[256];
    int visibleCount = 0;
    for (int i = 0; i < gameCount; i++) {
        if (currentCategory == 0) {
            visibleIndices[visibleCount++] = i;
        } else if (currentCategory == 1 && strcmp(games[i].system, "groovymame") == 0) {
            visibleIndices[visibleCount++] = i;
        } else if (currentCategory == 2 && strcmp(games[i].system, "flycast") == 0) {
            visibleIndices[visibleCount++] = i;
        } else if (currentCategory == 3 && strcmp(games[i].system, "pcsx2") == 0) {
            visibleIndices[visibleCount++] = i;
        } else if (currentCategory == 4 && strcmp(games[i].system, "dolphin") == 0) {
            visibleIndices[visibleCount++] = i;
        }
    }
    if (visibleCount == 0) {
        for (int i = 0; i < gameCount; i++) visibleIndices[visibleCount++] = i;
        visibleCount = gameCount;
    }

    if (selIdx >= visibleCount) selIdx = visibleCount - 1;
    if (selIdx < 0) selIdx = 0;

    // Scroll calculation
    int scrollOffset = 0;
    if (selIdx >= maxVisibleRows) {
        scrollOffset = selIdx - maxVisibleRows + 1;
    }

    // Render Game Rows
    for (int r = 0; r < maxVisibleRows; r++) {
        int vIdx = r + scrollOffset;
        if (vIdx >= visibleCount) break;

        int gIdx = visibleIndices[vIdx];
        int rowY = listY + r * rowH;
        int isSelected = (vIdx == selIdx);

        if (isSelected) {
            // Selected glowing row
            fill_rect(8, rowY, listW, rowH - 2, COLOR_ROW_SEL);
            fill_rect(8, rowY, 4, rowH - 2, COLOR_AMBER_BRIGHT);
            fill_rect(8 + listW - 2, rowY, 2, rowH - 2, COLOR_AMBER_BRIGHT);

            // Selection indicator arrow
            draw_char_8x12(16, rowY + (rowH - 12) / 2, '>' , COLOR_AMBER_BRIGHT, 1);
            draw_char_8x12(22, rowY + (rowH - 12) / 2, '>', COLOR_AMBER_BRIGHT, 1);
        } else {
            fill_rect(8, rowY, listW, rowH - 2, (r % 2 == 0) ? 0x0E0E16 : 0x12121B);
        }

        // Title clipped to available column width
        int titleX = 34;
        int tagW = isTwoColumn ? 110 : 80;
        int titleMaxW = listW - titleX - tagW - 10;
        uint32_t textCol = isSelected ? COLOR_WHITE : COLOR_GRAY_LIGHT;
        uint32_t shadowCol = isSelected ? 0x000000 : 0;
        draw_text_clipped(titleX, rowY + (rowH - 12) / 2, games[gIdx].title, textCol, shadowCol, titleMaxW, 1);

        // System pill tag on right of row
        int pillX = 8 + listW - tagW - 4;
        int pillY = rowY + (rowH - 14) / 2;
        uint32_t pillBg = isSelected ? COLOR_AMBER_DARK : 0x1E1E2C;
        uint32_t pillFg = isSelected ? COLOR_AMBER_BRIGHT : COLOR_CYAN_NEON;
        fill_rect(pillX, pillY, tagW, 14, pillBg);
        draw_text_clipped(pillX + 4, pillY + 1, games[gIdx].systemName, pillFg, 0, tagW - 8, 1);
    }

    // 5. Right Column: Arcade Marquee Flyer & Modeline Timings (When resolution permits)
    if (isTwoColumn) {
        int cardX = listW + 16;
        int cardW = w - cardX - 8;
        int cardY = listY;
        int cardH = listH;

        int activeGame = visibleIndices[selIdx];

        // Beveled Cabinet Panel Card
        fill_rect(cardX, cardY, cardW, cardH, COLOR_CARD_BG);
        fill_rect(cardX, cardY, cardW, 1, COLOR_CARD_BORDER);
        fill_rect(cardX, cardY, 1, cardH, COLOR_CARD_BORDER);
        fill_rect(cardX + cardW - 1, cardY, 1, cardH, COLOR_CARD_BORDER);
        fill_rect(cardX, cardY + cardH - 1, cardW, 1, COLOR_CARD_BORDER);

        // Card Header Bar
        fill_hgradient(cardX + 1, cardY + 1, cardW - 2, 28, COLOR_CYAN_DARK, 0x152233);
        draw_icon_16x16(cardX + 8, cardY + 6, icon_crt, 1);
        draw_text_shadowed(cardX + 30, cardY + 8, "CRT HARDWARE FLYER", COLOR_CYAN_NEON, 0, 1);

        // Simulated CRT Phosphor Frame
        int crtBoxX = cardX + 12;
        int crtBoxY = cardY + 36;
        int crtBoxW = cardW - 24;
        int crtBoxH = 90;
        fill_rect(crtBoxX, crtBoxY, crtBoxW, crtBoxH, 0x05070A);
        fill_rect(crtBoxX, crtBoxY, crtBoxW, 1, COLOR_AMBER_GLOW);

        // CRT scanline pattern inside preview box
        for (int sy = crtBoxY + 1; sy < crtBoxY + crtBoxH; sy += 2) {
            for (int sx = crtBoxX + 1; sx < crtBoxX + crtBoxW; sx += 4) {
                put_pixel(sx, sy, 0x0D1812);
            }
        }

        // Preview box contents: Game title & ROM
        draw_text_clipped(crtBoxX + 10, crtBoxY + 18, games[activeGame].title, COLOR_AMBER_BRIGHT, 0, crtBoxW - 20, 1);
        char romTag[64];
        snprintf(romTag, sizeof(romTag), "ROM ID: %s.zip", games[activeGame].id);
        draw_text_shadowed(crtBoxX + 10, crtBoxY + 36, romTag, COLOR_GRAY_LIGHT, 0, 1);

        char videoTag[64];
        snprintf(videoTag, sizeof(videoTag), "TIMING: %s", games[activeGame].res);
        draw_text_shadowed(crtBoxX + 10, crtBoxY + 54, videoTag, COLOR_CYAN_NEON, 0, 1);

        // Modeline Timings Technical Specs
        int techY = crtBoxY + crtBoxH + 12;
        draw_text_shadowed(cardX + 12, techY, "SWITCHRES CRT MODELINE:", COLOR_AMBER_BRIGHT, 0, 1);
        techY += 16;

        char hSyncStr[64];
        snprintf(hSyncStr, sizeof(hSyncStr), "  H-Sync Freq : %d Hz (15.7kHz CRT)", games[activeGame].hSync);
        draw_text_shadowed(cardX + 12, techY, hSyncStr, COLOR_GRAY_LIGHT, 0, 1);
        techY += 15;

        char vSyncStr[64];
        snprintf(vSyncStr, sizeof(vSyncStr), "  V-Sync Rate : %.2f Hz Native Clock", games[activeGame].vSync / 100.0f);
        draw_text_shadowed(cardX + 12, techY, vSyncStr, COLOR_GRAY_LIGHT, 0, 1);
        techY += 15;

        draw_text_shadowed(cardX + 12, techY, "  Pixel Clock : Hardware Dynamic Locked", COLOR_GRAY_LIGHT, 0, 1);
        techY += 15;

        draw_text_shadowed(cardX + 12, techY, "  Streaming   : Groovy_MiSTer UDP Sub-Frame", COLOR_GRAY_LIGHT, 0, 1);
        techY += 22;

        // Launch Action Banner
        fill_rect(cardX + 12, techY, cardW - 24, 30, COLOR_AMBER_DARK);
        draw_text_shadowed(cardX + 20, techY + 9, "ENTER / BTN 1: LAUNCH STREAM", COLOR_AMBER_BRIGHT, 0, 1);
    }

    // 6. Footer Bar
    int footY = h - footerH;
    fill_rect(0, footY, w, footerH, 0x08080C);
    fill_rect(0, footY, w, 1, COLOR_CARD_BORDER);

    // Arcade Button Legend
    int btnX = 14;
    // Joystick
    draw_char_8x12(btnX, footY + (footerH - 12) / 2, '^', COLOR_AMBER_BRIGHT, 1);
    draw_text_shadowed(btnX + 10, footY + (footerH - 12) / 2, "NAVIGATE", COLOR_GRAY_MID, 0, 1);
    btnX += 90;

    // Button 1 (Red)
    fill_rect(btnX, footY + (footerH - 8) / 2, 8, 8, COLOR_RED_BTN);
    draw_text_shadowed(btnX + 12, footY + (footerH - 12) / 2, "LAUNCH", COLOR_GRAY_LIGHT, 0, 1);
    btnX += 75;

    // Button 2 (Blue)
    fill_rect(btnX, footY + (footerH - 8) / 2, 8, 8, COLOR_BLUE_BTN);
    draw_text_shadowed(btnX + 12, footY + (footerH - 12) / 2, "CATEGORY", COLOR_GRAY_MID, 0, 1);
    btnX += 90;

    // Button 3 (Yellow)
    fill_rect(btnX, footY + (footerH - 8) / 2, 8, 8, COLOR_YELLOW_BTN);
    draw_text_shadowed(btnX + 12, footY + (footerH - 12) / 2, "EXIT MISTER", COLOR_GRAY_MID, 0, 1);

    // Blit entire rendered frame to Linux framebuffer
    flip_framebuffer();
}

// Display Animated Launch Loading Splash
static void show_launch_splash(const char *gameTitle) {
    int w = vinfo.xres;
    int h = vinfo.yres;

    int boxW = (w > 360) ? 360 : (w - 20);
    int boxH = 100;
    int boxX = (w - boxW) / 2;
    int boxY = (h - boxH) / 2;

    fill_rect(boxX, boxY, boxW, boxH, 0x141422);
    fill_rect(boxX, boxY, boxW, 2, COLOR_AMBER_BRIGHT);
    fill_rect(boxX, boxY + boxH - 2, boxW, 2, COLOR_AMBER_BRIGHT);

    draw_text_shadowed(boxX + 20, boxY + 20, "INITIALIZING 15.7kHz CRT STREAM...", COLOR_AMBER_BRIGHT, 0, 1);
    draw_text_clipped(boxX + 20, boxY + 42, gameTitle, COLOR_WHITE, 0, boxW - 40, 1);
    draw_text_shadowed(boxX + 20, boxY + 65, "Switching FPGA to Groovy.rbf core...", COLOR_CYAN_NEON, 0, 1);

    flip_framebuffer();
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
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "echo 'load_core %s' > /dev/MiSTer_cmd 2>/dev/null", core_path);
        system(cmd);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) strncpy(pc_ip, argv[1], sizeof(pc_ip) - 1);
    load_config();
    init_default_catalog();
    fetch_catalog_from_pc();

    // Open Framebuffer
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open /dev/fb0");
        return 1;
    }

    ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);

    screensize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((intptr_t)fbp == -1) {
        perror("Error: cannot mmap framebuffer");
        close(fbfd);
        return 1;
    }

    // Allocate 32-bit RGBA backbuffer for crisp, double-buffered graphics
    backbuffer = (uint32_t *)calloc(vinfo.xres * vinfo.yres, sizeof(uint32_t));
    if (!backbuffer) {
        perror("Error: cannot allocate backbuffer");
        return 1;
    }

    int selected = 0;
    int currentCategory = 0;
    render_gui(selected, currentCategory);

    // Open input devices
    int inputFds[16];
    int inputCount = 0;
    for (int i = 0; i < 16; i++) {
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
                if (ev.type == EV_KEY && ev.value == 1) { // Key down
                    if (ev.code == KEY_UP) {
                        if (selected > 0) { selected--; need_redraw = 1; }
                    } else if (ev.code == KEY_DOWN) {
                        if (selected < gameCount - 1) { selected++; need_redraw = 1; }
                    } else if (ev.code == KEY_LEFT) {
                        if (currentCategory > 0) { currentCategory--; selected = 0; need_redraw = 1; }
                    } else if (ev.code == KEY_RIGHT || ev.code == KEY_TAB) {
                        if (currentCategory < 4) { currentCategory++; selected = 0; need_redraw = 1; }
                    } else if (ev.code == KEY_ENTER || ev.code == BTN_TRIGGER || ev.code == BTN_A || ev.code == BTN_1) {
                        // 1. Send UDP packet to PC to launch GroovyMAME with Calamity 15kHz streaming
                        show_launch_splash(games[selected].title);
                        send_udp_launch(games[selected].id);
                        usleep(300000); // 300ms pause for network packet

                        // 2. Switch FPGA to Groovy.rbf
                        if (access(GROOVY_CORE_ARCADE, F_OK) == 0) {
                            switch_fpga_core(GROOVY_CORE_ARCADE);
                        } else {
                            switch_fpga_core(GROOVY_CORE_UTILITY);
                        }
                        running = 0;
                        break;
                    } else if (ev.code == KEY_ESC || ev.code == KEY_Q || ev.code == BTN_SELECT) {
                        switch_fpga_core(MENU_CORE_PATH);
                        running = 0;
                        break;
                    }
                }
            }
        }

        if (need_redraw) {
            render_gui(selected, currentCategory);
        }
        usleep(16000); // ~60 FPS polling
    }

    free(backbuffer);
    munmap(fbp, screensize);
    close(fbfd);
    for (int i = 0; i < inputCount; i++) close(inputFds[i]);

    return 0;
}
