/**
 * ============================================================================
 * Phantom Arcade - High-Fidelity CRT Framebuffer Arcade Frontend for MiSTer FPGA
 * Target: ARMv7 Linux (DE10-Nano / MiSTer FPGA) - Statically Linked
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
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
#define COLOR_BLACK          0x000000
#define COLOR_SCREEN_BG      0x0B0C12  // Deep dark backdrop
#define COLOR_PANEL_BG       0x151722  // Outer dark grey panel
#define COLOR_CARD_BG        0x1B1D2A  // Selection menu card background
#define COLOR_CARD_BORDER    0x2E3246  // Crisp menu border
#define COLOR_CARD_BORDER_HI 0x474E6B  // Highlighted border
#define COLOR_ROW_EVEN       0x1E202E  // Distinct dark grey row
#define COLOR_ROW_ODD        0x171924  // Alternating row
#define COLOR_ROW_SEL_BG     0x2D2513  // Amber tinted selection background
#define COLOR_AMBER_BRIGHT   0xF59E0B  // Gold neon accent
#define COLOR_AMBER_DARK     0x78350F  // Deep amber
#define COLOR_AMBER_GLOW     0xD97706  // Cursor glow
#define COLOR_CYAN_NEON      0x06B6D4  // Modeline cyan
#define COLOR_CYAN_DARK      0x0E3A47  // Panel header
#define COLOR_GREEN_LED      0x10B981  // Host status LED
#define COLOR_RED_BTN        0xEF4444  // Launch button
#define COLOR_BLUE_BTN       0x3B82F6  // Nav button
#define COLOR_YELLOW_BTN     0xEAB308  // Exit button
#define COLOR_WHITE          0xF8FAFC  // Primary text
#define COLOR_GRAY_LIGHT     0xCBD5E1  // Secondary text
#define COLOR_GRAY_MID       0x717F9A  // Muted labels
#define COLOR_GRAY_DARK      0x252838  // Inactive tab

typedef struct {
    char id[64];
    char title[64];
    char system[32];
    char systemName[32];
    char res[32];
    char mode[32];
    int  hSync;
    int  vSync;
} GameEntry;

static GameEntry games[256];
static int gameCount = 0;

static int fbfd = -1;
static char *fbp = NULL;
static uint32_t *backbuffer = NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static long screensize = 0;
static char pc_ip[64] = DEFAULT_PC_IP;
static int udp_port = DEFAULT_UDP_PORT;
static int core_launch_delay = 4; // Seconds to let Cyclone V FPGA reconfigure before triggering emulator

static int inputFds[16];
static int inputCount = 0;
static struct termios orig_termios;
static int tty_fd = -1;

#include "font_arcade.h"
#include "arcade_bitmaps.h"

// Terminal Raw Mode & Input Grab to completely eliminate ^[[A ^[[B echo
static void setup_terminal() {
    tty_fd = open("/dev/tty0", O_RDWR);
    if (tty_fd < 0) tty_fd = open("/dev/tty", O_RDWR);
    if (tty_fd >= 0) {
        tcgetattr(tty_fd, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        raw.c_iflag &= ~(IXON | ICRNL);
        tcsetattr(tty_fd, TCSANOW, &raw);
        write(tty_fd, "\033[?25l", 6); // Hide cursor
    }
    // Shell fallback
    system("setterm -cursor off > /dev/tty0 2>/dev/null");
    system("stty -echo < /dev/tty0 2>/dev/null");
}

static void restore_terminal() {
    if (tty_fd >= 0) {
        write(tty_fd, "\033[?25h", 6); // Show cursor
        tcsetattr(tty_fd, TCSANOW, &orig_termios);
        close(tty_fd);
        tty_fd = -1;
    }
    system("setterm -cursor on > /dev/tty0 2>/dev/null");
    system("stty echo < /dev/tty0 2>/dev/null");
}

static inline void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)vinfo.xres || y < 0 || y >= (int)vinfo.yres) return;
    backbuffer[y * vinfo.xres + x] = color;
}

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

static void draw_beveled_box(int x, int y, int w, int h, uint32_t bg, uint32_t border, uint32_t hilight) {
    fill_rect(x, y, w, h, bg);
    fill_rect(x, y, w, 1, border);
    fill_rect(x, y + h - 1, w, 1, border);
    fill_rect(x, y, 1, h, border);
    fill_rect(x + w - 1, y, 1, h, border);
    if (hilight != 0 && w > 4 && h > 4) {
        fill_rect(x + 1, y + 1, w - 2, 1, hilight);
        fill_rect(x + 1, y + 1, 1, h - 2, hilight);
    }
}

static void draw_char_8x12(int x, int y, char c, uint32_t fg, int scale) {
    int idx = (unsigned char)c - 32;
    if (idx < 0 || idx >= 95) idx = 31; // '?'
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

static void draw_text_shadowed(int x, int y, const char *str, uint32_t fg, uint32_t shadowCol, int scale) {
    if (!str) return;
    int cur_x = x;
    int shadowOffset = (scale > 1) ? 2 : 1;

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

    while (*str) {
        draw_char_8x12(cur_x, y, *str, fg, scale);
        cur_x += 8 * scale;
        str++;
    }
}

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

static void draw_icon_16x16(int x, int y, const uint32_t *icon, int scale) {
    for (int r = 0; r < 16; r++) {
        for (int c = 0; c < 16; c++) {
            uint32_t col = icon[r * 16 + c];
            if (col != 0) {
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
        } else if (strncmp(line, "CORE_LAUNCH_DELAY=", 18) == 0) {
            core_launch_delay = atoi(line + 18);
            if (core_launch_delay < 1) core_launch_delay = 1;
            if (core_launch_delay > 20) core_launch_delay = 20;
        }
    }
    fclose(f);
}

static void init_default_catalog() {
    gameCount = 0;
    #define ADD_ARCADE(gid, gname, gsys, gsysname, gres, gmode, gh, gv) do { \
        strncpy(games[gameCount].id, gid, 63); \
        strncpy(games[gameCount].title, gname, 63); \
        strncpy(games[gameCount].system, gsys, 31); \
        strncpy(games[gameCount].systemName, gsysname, 31); \
        strncpy(games[gameCount].res, gres, 31); \
        strncpy(games[gameCount].mode, gmode, 31); \
        games[gameCount].hSync = gh; \
        games[gameCount].vSync = gv; \
        gameCount++; \
    } while (0)

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

static void fetch_catalog_from_pc() {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
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
        char *p = buf;
        int parsed = 0;
        while ((p = strstr(p, "\"id\":")) != NULL && parsed < 250) {
            char *idStart = strchr(p + 5, '\"');
            if (!idStart) break;
            char *idEnd = strchr(idStart + 1, '\"');
            if (!idEnd) break;

            char gid[64];
            int idLen = idEnd - (idStart + 1);
            if (idLen > 63) idLen = 63;
            strncpy(gid, idStart + 1, idLen);
            gid[idLen] = 0;

            char title[64] = "";
            char *tPos = strstr(p, "\"title\":");
            if (tPos && tPos < strstr(p, "}")) {
                char *tStart = strchr(tPos + 8, '\"');
                if (tStart) {
                    char *tEnd = strchr(tStart + 1, '\"');
                    if (tEnd) {
                        int tLen = tEnd - (tStart + 1);
                        if (tLen > 63) tLen = 63;
                        strncpy(title, tStart + 1, tLen);
                        title[tLen] = 0;
                    }
                }
            }

            char sys[32] = "groovymame";
            char *sPos = strstr(p, "\"system\":");
            if (sPos && sPos < strstr(p, "}")) {
                char *sStart = strchr(sPos + 9, '\"');
                if (sStart) {
                    char *sEnd = strchr(sStart + 1, '\"');
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

// Compute visible games for a given category
static int get_visible_games(int category, int *indices) {
    int count = 0;
    for (int i = 0; i < gameCount; i++) {
        if (category == 0) { // ALL
            indices[count++] = i;
        } else if (category == 1) { // GROOVYMAME
            if (strcasecmp(games[i].system, "groovymame") == 0 ||
                strcasecmp(games[i].system, "mame") == 0 ||
                strcasecmp(games[i].system, "arcade") == 0) {
                indices[count++] = i;
            }
        } else if (category == 2) { // NAOMI / DC
            if (strcasecmp(games[i].system, "flycast") == 0 ||
                strcasecmp(games[i].system, "naomi") == 0 ||
                strcasecmp(games[i].system, "dreamcast") == 0) {
                indices[count++] = i;
            }
        } else if (category == 3) { // PS2
            if (strcasecmp(games[i].system, "pcsx2") == 0 ||
                strcasecmp(games[i].system, "ps2") == 0) {
                indices[count++] = i;
            }
        } else if (category == 4) { // GAMECUBE
            if (strcasecmp(games[i].system, "dolphin") == 0 ||
                strcasecmp(games[i].system, "gamecube") == 0 ||
                strcasecmp(games[i].system, "gc") == 0) {
                indices[count++] = i;
            }
        }
    }
    return count;
}

// Render Complete High-Impact Arcade CRT Interface
static void render_gui(int selIdx, int currentCategory) {
    int w = vinfo.xres;
    int h = vinfo.yres;

    // 1. CRT Base Canvas
    fill_rect(0, 0, w, h, COLOR_SCREEN_BG);

    // 2. Marquee Header (Illuminated Arcade Header)
    int headerH = (h >= 400) ? 44 : 26;
    fill_hgradient(0, 0, w, headerH, 0x1F172C, 0x121422);
    fill_rect(0, headerH - 2, w, 2, COLOR_AMBER_BRIGHT);

    // Cabinet Icon + Marquee Title
    int titleScale = (h >= 400) ? 2 : 1;
    draw_icon_16x16(10, (headerH - 16 * titleScale) / 2, icon_cabinet, titleScale);
    draw_text_shadowed(14 + 20 * titleScale, (headerH - 12 * titleScale) / 2, "PHANTOM ARCADE", COLOR_AMBER_BRIGHT, COLOR_AMBER_DARK, titleScale);

    // Host status indicator on the right
    char hostStatus[64];
    snprintf(hostStatus, sizeof(hostStatus), "HOST: %s:%d", pc_ip, udp_port);
    int statusW = (int)strlen(hostStatus) * 8 + 24;
    int statusX = w - statusW - 8;
    if (statusX > 200) {
        draw_beveled_box(statusX, 4, statusW, headerH - 8, 0x171A26, COLOR_CARD_BORDER, 0);
        fill_rect(statusX + 6, headerH / 2 - 3, 6, 6, COLOR_GREEN_LED);
        draw_text_shadowed(statusX + 16, headerH / 2 - 5, hostStatus, COLOR_GRAY_LIGHT, 0, 1);
    }

    // 3. Platform Category Filter Tabs
    const char *tabs[] = { "ALL SYSTEMS", "GROOVYMAME", "SEGA NAOMI", "SONY PS2", "GAMECUBE" };
    int tabCount = 5;
    int tabY = headerH + 3;
    int tabH = (h >= 400) ? 24 : 17;
    fill_rect(0, tabY, w, tabH, 0x11121B);

    int curTabX = 8;
    for (int t = 0; t < tabCount; t++) {
        int tLen = (int)strlen(tabs[t]);
        int tWidth = tLen * 8 + 14;
        if (t == currentCategory) {
            // Active Tab (Glowing Amber with high contrast text)
            draw_beveled_box(curTabX, tabY + 1, tWidth, tabH - 2, COLOR_AMBER_BRIGHT, 0xFBBF24, 0xFEF08A);
            draw_text_shadowed(curTabX + 7, tabY + (tabH - 12) / 2, tabs[t], COLOR_BLACK, 0, 1);
        } else {
            // Inactive Tab (Dark grey card tab)
            draw_beveled_box(curTabX, tabY + 1, tWidth, tabH - 2, COLOR_GRAY_DARK, COLOR_CARD_BORDER, 0);
            draw_text_shadowed(curTabX + 7, tabY + (tabH - 12) / 2, tabs[t], COLOR_GRAY_MID, 0, 1);
        }
        curTabX += tWidth + 4;
    }
    fill_rect(0, tabY + tabH, w, 1, COLOR_CARD_BORDER);

    // 4. Layout Dimensions
    int isTwoColumn = (w >= 500 && h >= 400);
    int footerH = (h >= 400) ? 30 : 20;

    int listY = tabY + tabH + 6;
    int listH = h - listY - footerH - 6;
    int listW = isTwoColumn ? (w * 58 / 100) : (w - 16);

    // DARK GREY BACKGROUND CONTAINER FOR THE ITEM SELECTION MENU
    // Clearly defines where the selection menu starts and ends!
    draw_beveled_box(8, listY, listW, listH, COLOR_CARD_BG, COLOR_CARD_BORDER, COLOR_CARD_BORDER_HI);

    // Inner Menu Header Line
    fill_rect(9, listY + 1, listW - 2, 18, 0x141620);
    draw_text_shadowed(16, listY + 4, "GAME TITLE", COLOR_GRAY_MID, 0, 1);
    int tagHeaderX = 8 + listW - (isTwoColumn ? 110 : 80) - 12;
    draw_text_shadowed(tagHeaderX, listY + 4, "SYSTEM", COLOR_GRAY_MID, 0, 1);
    fill_rect(9, listY + 19, listW - 2, 1, COLOR_CARD_BORDER);

    int innerListY = listY + 20;
    int innerListH = listH - 21;

    int rowH = (h >= 400) ? 26 : 18;
    int maxVisibleRows = innerListH / rowH;
    if (maxVisibleRows < 1) maxVisibleRows = 1;

    // Filter games
    int visibleIndices[256];
    int visibleCount = get_visible_games(currentCategory, visibleIndices);

    if (visibleCount == 0) {
        // Show clean empty state message instead of jumbling
        int msgY = innerListY + 40;
        draw_text_shadowed(24, msgY, "[ No titles configured in this category ]", COLOR_AMBER_BRIGHT, 0, 1);
        draw_text_shadowed(24, msgY + 20, "Scan ROMs on Windows Manager to populate.", COLOR_GRAY_LIGHT, 0, 1);
        draw_text_shadowed(24, msgY + 40, "Press LEFT / RIGHT to switch platforms.", COLOR_CYAN_NEON, 0, 1);
    } else {
        if (selIdx >= visibleCount) selIdx = visibleCount - 1;
        if (selIdx < 0) selIdx = 0;

        int scrollOffset = 0;
        if (selIdx >= maxVisibleRows) {
            scrollOffset = selIdx - maxVisibleRows + 1;
        }

        // Render Game Rows
        for (int r = 0; r < maxVisibleRows; r++) {
            int vIdx = r + scrollOffset;
            if (vIdx >= visibleCount) break;

            int gIdx = visibleIndices[vIdx];
            int rowY = innerListY + r * rowH;
            int isSelected = (vIdx == selIdx);

            if (isSelected) {
                // High-Contrast Selected Row with Amber/Gold Bar
                fill_rect(9, rowY, listW - 2, rowH, COLOR_ROW_SEL_BG);
                fill_rect(9, rowY, 5, rowH, COLOR_AMBER_BRIGHT);
                fill_rect(9, rowY, listW - 2, 1, COLOR_AMBER_GLOW);
                fill_rect(9, rowY + rowH - 1, listW - 2, 1, COLOR_AMBER_GLOW);

                // Distinct Gold Arrow Cursor
                draw_char_8x12(17, rowY + (rowH - 12) / 2, '>', COLOR_AMBER_BRIGHT, 1);
                draw_char_8x12(23, rowY + (rowH - 12) / 2, '>', COLOR_AMBER_BRIGHT, 1);
            } else {
                // Alternating dark grey rows
                uint32_t rowBg = (r % 2 == 0) ? COLOR_ROW_EVEN : COLOR_ROW_ODD;
                fill_rect(9, rowY, listW - 2, rowH, rowBg);
                fill_rect(9, rowY + rowH - 1, listW - 2, 1, 0x141520);
            }

            // Title
            int titleX = 36;
            int tagW = isTwoColumn ? 110 : 80;
            int titleMaxW = listW - titleX - tagW - 14;
            uint32_t textCol = isSelected ? COLOR_WHITE : COLOR_GRAY_LIGHT;
            uint32_t shadowCol = isSelected ? 0x000000 : 0;
            draw_text_clipped(titleX, rowY + (rowH - 12) / 2, games[gIdx].title, textCol, shadowCol, titleMaxW, 1);

            // System pill tag on right
            int pillX = 8 + listW - tagW - 8;
            int pillY = rowY + (rowH - 14) / 2;
            uint32_t pillBg = isSelected ? COLOR_AMBER_DARK : 0x222638;
            uint32_t pillFg = isSelected ? COLOR_AMBER_BRIGHT : COLOR_CYAN_NEON;
            fill_rect(pillX, pillY, tagW, 14, pillBg);
            fill_rect(pillX, pillY, tagW, 1, isSelected ? COLOR_AMBER_BRIGHT : COLOR_CARD_BORDER);
            draw_text_clipped(pillX + 5, pillY + 1, games[gIdx].systemName, pillFg, 0, tagW - 10, 1);
        }
    }

    // 5. Right Column: Arcade Marquee Flyer & Modeline Timings (When 480i/480p+)
    if (isTwoColumn) {
        int cardX = listW + 16;
        int cardW = w - cardX - 8;
        int cardY = listY;
        int cardH = listH;

        int activeGame = (visibleCount > 0 && selIdx < visibleCount) ? visibleIndices[selIdx] : 0;

        // Dark Grey Card Enclosure
        draw_beveled_box(cardX, cardY, cardW, cardH, COLOR_CARD_BG, COLOR_CARD_BORDER, COLOR_CARD_BORDER_HI);

        // Card Header Bar
        fill_hgradient(cardX + 1, cardY + 1, cardW - 2, 28, COLOR_CYAN_DARK, 0x152233);
        fill_rect(cardX + 1, cardY + 28, cardW - 2, 1, COLOR_CYAN_NEON);
        draw_icon_16x16(cardX + 8, cardY + 6, icon_crt, 1);
        draw_text_shadowed(cardX + 30, cardY + 8, "CRT HARDWARE FLYER", COLOR_CYAN_NEON, 0, 1);

        // Recessed CRT Phosphor Screen Frame
        int crtBoxX = cardX + 12;
        int crtBoxY = cardY + 36;
        int crtBoxW = cardW - 24;
        int crtBoxH = 88;
        draw_beveled_box(crtBoxX, crtBoxY, crtBoxW, crtBoxH, 0x05070A, 0x1B2A2E, 0);

        for (int sy = crtBoxY + 1; sy < crtBoxY + crtBoxH; sy += 2) {
            for (int sx = crtBoxX + 1; sx < crtBoxX + crtBoxW; sx += 4) {
                put_pixel(sx, sy, 0x0C1813);
            }
        }

        if (visibleCount > 0) {
            draw_text_clipped(crtBoxX + 10, crtBoxY + 16, games[activeGame].title, COLOR_AMBER_BRIGHT, 0, crtBoxW - 20, 1);
            char romTag[64];
            snprintf(romTag, sizeof(romTag), "ROM ID: %s.zip", games[activeGame].id);
            draw_text_shadowed(crtBoxX + 10, crtBoxY + 34, romTag, COLOR_GRAY_LIGHT, 0, 1);

            char videoTag[64];
            snprintf(videoTag, sizeof(videoTag), "TIMING: %s", games[activeGame].res);
            draw_text_shadowed(crtBoxX + 10, crtBoxY + 52, videoTag, COLOR_CYAN_NEON, 0, 1);
        }

        // Modeline Timings Technical Specs
        int techY = crtBoxY + crtBoxH + 12;
        draw_text_shadowed(cardX + 12, techY, "SWITCHRES CRT MODELINE:", COLOR_AMBER_BRIGHT, 0, 1);
        techY += 16;

        if (visibleCount > 0) {
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
        }

        // Launch Action Banner
        draw_beveled_box(cardX + 12, techY, cardW - 24, 30, COLOR_AMBER_DARK, COLOR_AMBER_BRIGHT, 0);
        draw_text_shadowed(cardX + 22, techY + 9, "ENTER / BTN 1: LAUNCH STREAM", COLOR_AMBER_BRIGHT, 0, 1);
    }

    // 6. Footer Legend Bar
    int footY = h - footerH;
    draw_beveled_box(0, footY, w, footerH, 0x10111A, COLOR_CARD_BORDER, 0);

    int btnX = 14;
    // Joystick
    draw_char_8x12(btnX, footY + (footerH - 12) / 2, '^', COLOR_AMBER_BRIGHT, 1);
    draw_text_shadowed(btnX + 10, footY + (footerH - 12) / 2, "SELECT", COLOR_GRAY_LIGHT, 0, 1);
    btnX += 78;

    // Category
    draw_char_8x12(btnX, footY + (footerH - 12) / 2, '<', COLOR_CYAN_NEON, 1);
    draw_char_8x12(btnX + 6, footY + (footerH - 12) / 2, '>', COLOR_CYAN_NEON, 1);
    draw_text_shadowed(btnX + 16, footY + (footerH - 12) / 2, "PLATFORM", COLOR_GRAY_LIGHT, 0, 1);
    btnX += 96;

    // Button 1 (Red)
    fill_rect(btnX, footY + (footerH - 8) / 2, 8, 8, COLOR_RED_BTN);
    draw_text_shadowed(btnX + 12, footY + (footerH - 12) / 2, "LAUNCH", COLOR_WHITE, 0, 1);
    btnX += 75;

    // Button 3 (Yellow)
    fill_rect(btnX, footY + (footerH - 8) / 2, 8, 8, COLOR_YELLOW_BTN);
    draw_text_shadowed(btnX + 12, footY + (footerH - 12) / 2, "EXIT MISTER", COLOR_GRAY_MID, 0, 1);

    flip_framebuffer();
}

// Display Animated Launch Loading Splash with Countdown
static void show_launch_splash(const char *gameTitle, const char *status) {
    int w = vinfo.xres;
    int h = vinfo.yres;

    int boxW = (w > 400) ? 400 : (w - 20);
    int boxH = 110;
    int boxX = (w - boxW) / 2;
    int boxY = (h - boxH) / 2;

    draw_beveled_box(boxX, boxY, boxW, boxH, 0x161826, COLOR_AMBER_BRIGHT, COLOR_AMBER_GLOW);

    draw_text_shadowed(boxX + 20, boxY + 16, "INITIALIZING 15.7kHz CRT STREAM", COLOR_AMBER_BRIGHT, 0, 1);
    draw_text_clipped(boxX + 20, boxY + 38, gameTitle, COLOR_WHITE, 0, boxW - 40, 1);
    draw_text_shadowed(boxX + 20, boxY + 60, status, COLOR_CYAN_NEON, 0, 1);

    // Animated loading bar
    fill_rect(boxX + 20, boxY + 84, boxW - 40, 6, 0x0A0B12);
    fill_rect(boxX + 20, boxY + 84, (boxW - 40) * 80 / 100, 6, COLOR_AMBER_BRIGHT);

    flip_framebuffer();
}

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
    // Redundant retry datagram after 100ms in case of switch buffer latency
    usleep(100000);
    sendto(sockfd, packet, strlen(packet), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    close(sockfd);
    printf("[+] Sent '%s' to %s:%d\n", packet, pc_ip, udp_port);
}

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

    setup_terminal();

    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open /dev/fb0");
        restore_terminal();
        return 1;
    }

    ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);

    screensize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((intptr_t)fbp == -1) {
        perror("Error: cannot mmap framebuffer");
        close(fbfd);
        restore_terminal();
        return 1;
    }

    backbuffer = (uint32_t *)calloc(vinfo.xres * vinfo.yres, sizeof(uint32_t));
    if (!backbuffer) {
        perror("Error: cannot allocate backbuffer");
        restore_terminal();
        return 1;
    }

    int selected = 0;
    int currentCategory = 0;
    render_gui(selected, currentCategory);

    // Open input devices and grab them exclusively to prevent ^[[A ^[[B echo to console!
    inputCount = 0;
    for (int i = 0; i < 16; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            ioctl(fd, EVIOCGRAB, 1); // EXCLUSIVE GRAB
            inputFds[inputCount++] = fd;
        }
    }

    int running = 1;
    int axisX = 0, axisY = 0;

    while (running) {
        int need_redraw = 0;

        int visibleIndices[256];
        int visibleCount = get_visible_games(currentCategory, visibleIndices);

        for (int i = 0; i < inputCount; i++) {
            struct input_event ev;
            while (read(inputFds[i], &ev, sizeof(ev)) > 0) {
                // Gamepad Analog / HAT stick axes
                if (ev.type == EV_ABS) {
                    if (ev.code == ABS_Y || ev.code == ABS_HAT0Y) {
                        if (ev.value < -10000 || ev.value == -1) {
                            if (axisY != -1) {
                                axisY = -1;
                                if (selected > 0) { selected--; need_redraw = 1; }
                            }
                        } else if (ev.value > 10000 || ev.value == 1) {
                            if (axisY != 1) {
                                axisY = 1;
                                if (selected < visibleCount - 1) { selected++; need_redraw = 1; }
                            }
                        } else {
                            axisY = 0;
                        }
                    } else if (ev.code == ABS_X || ev.code == ABS_HAT0X) {
                        if (ev.value < -10000 || ev.value == -1) {
                            if (axisX != -1) {
                                axisX = -1;
                                if (currentCategory > 0) { currentCategory--; selected = 0; need_redraw = 1; }
                            }
                        } else if (ev.value > 10000 || ev.value == 1) {
                            if (axisX != 1) {
                                axisX = 1;
                                if (currentCategory < 4) { currentCategory++; selected = 0; need_redraw = 1; }
                            }
                        } else {
                            axisX = 0;
                        }
                    }
                }

                // Keyboard & Gamepad Buttons
                if (ev.type == EV_KEY && ev.value == 1) {
                    if (ev.code == KEY_UP || ev.code == KEY_W || ev.code == BTN_DPAD_UP) {
                        if (selected > 0) { selected--; need_redraw = 1; }
                    } else if (ev.code == KEY_DOWN || ev.code == KEY_S || ev.code == BTN_DPAD_DOWN) {
                        if (selected < visibleCount - 1) { selected++; need_redraw = 1; }
                    } else if (ev.code == KEY_LEFT || ev.code == KEY_A || ev.code == BTN_DPAD_LEFT) {
                        if (currentCategory > 0) { currentCategory--; selected = 0; need_redraw = 1; }
                    } else if (ev.code == KEY_RIGHT || ev.code == KEY_D || ev.code == KEY_TAB || ev.code == BTN_DPAD_RIGHT) {
                        if (currentCategory < 4) { currentCategory++; selected = 0; need_redraw = 1; }
                    } else if (ev.code == KEY_ENTER || ev.code == KEY_SPACE ||
                               ev.code == BTN_TRIGGER || ev.code == BTN_A || ev.code == BTN_B ||
                               ev.code == BTN_SOUTH || ev.code == BTN_START || ev.code == BTN_1) {
                        if (visibleCount > 0) {
                            int activeGame = visibleIndices[selected];
                            const char *coreToLoad = (access(GROOVY_CORE_ARCADE, F_OK) == 0) ? GROOVY_CORE_ARCADE : GROOVY_CORE_UTILITY;

                            char statusMsg[128];
                            snprintf(statusMsg, sizeof(statusMsg), "Switching FPGA to Groovy.rbf... PC starting in 3s");
                            show_launch_splash(games[activeGame].title, statusMsg);

                            char targetGameId[64];
                            strncpy(targetGameId, games[activeGame].id, sizeof(targetGameId) - 1);
                            targetGameId[sizeof(targetGameId) - 1] = 0;

                            // FORK DETACHED DAEMON:
                            // Because MiSTer blocks while Phantom_Arcade.sh is running,
                            // Phantom_Arcade.sh MUST exit immediately so MiSTer unblocks and
                            // reprograms the Cyclone V FPGA with Groovy.rbf!
                            // The detached child sends the launch datagram with 3s PC-side delay.
                            pid_t pid = fork();
                            if (pid == 0) {
                                setsid();
                                for (int fd = 0; fd < 64; fd++) {
                                    close(fd);
                                }
                                usleep(250000); // 250ms grace period so parent unblocks MiSTer FIFO
                                send_udp_launch(targetGameId);
                                _exit(0);
                            }

                            // In parent: write load_core command to MiSTer FIFO
                            switch_fpga_core(coreToLoad);

                            // Exit immediately so MiSTer unblocks and starts FPGA programming
                            running = 0;
                            break;
                        }
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
        usleep(16000);
    }

    // Release input grabs
    for (int i = 0; i < inputCount; i++) {
        ioctl(inputFds[i], EVIOCGRAB, 0);
        close(inputFds[i]);
    }

    free(backbuffer);
    munmap(fbp, screensize);
    close(fbfd);
    restore_terminal();

    return 0;
}
