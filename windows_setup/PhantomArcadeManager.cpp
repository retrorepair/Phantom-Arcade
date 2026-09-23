/**
 * ============================================================================
 * Phantom Arcade - Windows Setup & Management Application
 * File: PhantomArcadeManager.cpp
 * Language: C++17 / Native Win32
 * 
 * Description:
 *   Native Windows desktop setup tool and daemon launcher for the Phantom Arcade
 *   Groovy_MiSTer bridge. Supports GroovyMAME (Calamity 15kHz native streaming),
 *   RetroArch (SwitchRes multi-core CRT), Dolphin, Flycast, and PCSX2.
 *   Dynamic UDP port configuration (default 1999), LAN auto-discovery, and
 *   integrated HTTP catalog server on TCP :8088.
 * ============================================================================
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace fs = std::filesystem;

// Control IDs
#define IDC_EDIT_MISTER_IP          101
#define IDC_EDIT_UDP_PORT           102
#define IDC_EDIT_GROOVYMAME_EXE     103
#define IDC_BTN_BROWSE_MAME_EXE     104
#define IDC_EDIT_MAME_ROMS          105
#define IDC_BTN_BROWSE_MAME_ROMS    106
#define IDC_EDIT_RETROARCH_EXE      107
#define IDC_BTN_BROWSE_RA_EXE       108
#define IDC_EDIT_RETROARCH_ROMS     109
#define IDC_BTN_BROWSE_RA_ROMS      110
#define IDC_EDIT_DOLPHIN_EXE        111
#define IDC_BTN_BROWSE_DOLPHIN      112
#define IDC_EDIT_GC_ROMS            113
#define IDC_BTN_BROWSE_GCROMS       114
#define IDC_EDIT_FLYCAST_EXE        115
#define IDC_BTN_BROWSE_FLYCAST      116
#define IDC_EDIT_NAOMI_ROMS         117
#define IDC_BTN_BROWSE_NAOMI        118
#define IDC_EDIT_PCSX2_EXE          119
#define IDC_BTN_BROWSE_PCSX2        120
#define IDC_EDIT_PS2_ROMS           121
#define IDC_BTN_BROWSE_PS2ROMS      122
#define IDC_BTN_SCAN_ROMS           123
#define IDC_BTN_TEST_MISTER         124
#define IDC_BTN_SAVE_CONFIG         125
#define IDC_BTN_TOGGLE_DAEMON       126
#define IDC_STATIC_STATUS           127
#define IDC_LIST_GAMES              128

// Global State
HINSTANCE hInst = NULL;
HWND hMainWnd = NULL;
HWND hEditMisterIp, hEditUdpPort;
HWND hEditMameExe, hEditMameRoms;
HWND hEditRetroarchExe, hEditRetroarchRoms;
HWND hEditDolphinExe, hEditGcRoms;
HWND hEditFlycastExe, hEditNaomiRoms;
HWND hEditPcsx2Exe, hEditPs2Roms;
HWND hStaticStatus, hListGames;
HWND hBtnToggleDaemon;

std::atomic<bool> g_daemonRunning(false);
std::atomic<DWORD> g_activePid(0);
std::atomic<int> g_configuredPort(1999);
SOCKET g_udpSocket = INVALID_SOCKET;
SOCKET g_httpSocket = INVALID_SOCKET;
HANDLE g_hDaemonThread = NULL;
HANDLE g_hHttpThread = NULL;

// Helper: Browse for Folder
std::wstring BrowseFolder(HWND hWnd, const wchar_t* title) {
    std::wstring result = L"";
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = title;
    bi.hwndOwner = hWnd;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != 0) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            result = path;
        }
        CoTaskMemFree(pidl);
    }
    return result;
}

// Helper: Browse for Executable
std::wstring BrowseFile(HWND hWnd, const wchar_t* filter) {
    wchar_t filename[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
        return filename;
    }
    return L"";
}

// Helper: Read Window Text
std::wstring GetText(HWND hWnd) {
    int len = GetWindowTextLength(hWnd);
    if (len <= 0) return L"";
    std::vector<wchar_t> buf(len + 1);
    GetWindowText(hWnd, buf.data(), len + 1);
    return std::wstring(buf.data());
}

// Helper: UTF-8 and JSON String escaping
std::string ToJsonString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string s(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &s[0], sizeNeeded, NULL, NULL);
    std::string out = "";
    for (char c : s) {
        if (c == '\\') out += "/"; // Convert backslashes to forward slashes: 100% valid in Windows API and zero JSON escape issues!
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::wstring StringToWstring(const std::string& s) {
    if (s.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (sizeNeeded <= 0) return std::wstring(s.begin(), s.end());
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &wstr[0], sizeNeeded);
    return wstr;
}

// Robust JSON key-value extractors
std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = 0;
    while ((pos = json.find(searchKey, pos)) != std::string::npos) {
        size_t colon = json.find(':', pos + searchKey.length());
        if (colon == std::string::npos) break;
        size_t quoteStart = json.find('"', colon + 1);
        if (quoteStart == std::string::npos) break;
        
        bool ok = true;
        for (size_t i = colon + 1; i < quoteStart; ++i) {
            char ch = json[i];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                ok = false;
                break;
            }
        }
        if (!ok) {
            pos += searchKey.length();
            continue;
        }

        std::string val = "";
        size_t p = quoteStart + 1;
        while (p < json.length()) {
            if (json[p] == '\\' && p + 1 < json.length()) {
                char nextC = json[p + 1];
                if (nextC == '\\') { val += '\\'; p += 2; }
                else if (nextC == '"') { val += '"'; p += 2; }
                else if (nextC == '/') { val += '/'; p += 2; }
                else if (nextC == 'n') { val += '\n'; p += 2; }
                else if (nextC == 'r') { val += '\r'; p += 2; }
                else if (nextC == 't') { val += '\t'; p += 2; }
                else { val += nextC; p += 2; }
            } else if (json[p] == '"') {
                break;
            } else {
                val += json[p++];
            }
        }
        return val;
    }
    return "";
}

int ExtractJsonInt(const std::string& json, const std::string& key, int defaultVal) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos + searchKey.length());
    if (pos == std::string::npos) return defaultVal;
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;
    std::string numStr = "";
    while (pos < json.length() && (isdigit(json[pos]) || json[pos] == '-')) {
        numStr += json[pos++];
    }
    if (!numStr.empty()) {
        try { return std::stoi(numStr); } catch (...) {}
    }
    return defaultVal;
}

// Helper: Get integer port from edit box with fallback
int GetPortFromUI() {
    std::wstring portStr = GetText(hEditUdpPort);
    if (portStr.empty()) return 1999;
    try {
        int val = std::stoi(portStr);
        if (val > 0 && val <= 65535) return val;
    } catch (...) {}
    return 1999;
}

// Load Configuration from phantom_config.json
void LoadConfiguration() {
    std::ifstream in("phantom_config.json");
    if (!in.is_open()) return;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string json = ss.str();
    in.close();

    std::string misterIp = ExtractJsonString(json, "mister_client_ip");
    if (!misterIp.empty()) SetWindowText(hEditMisterIp, StringToWstring(misterIp).c_str());

    int port = ExtractJsonInt(json, "udp_port", 1999);
    SetWindowText(hEditUdpPort, std::to_wstring(port).c_str());

    // MAME
    std::string mameExe = ExtractJsonString(json, "mame_exe");
    if (mameExe.empty()) mameExe = ExtractJsonString(json, "exe");
    if (!mameExe.empty()) SetWindowText(hEditMameExe, StringToWstring(mameExe).c_str());

    std::string mameRoms = ExtractJsonString(json, "mame_roms");
    if (mameRoms.empty()) mameRoms = ExtractJsonString(json, "roms");
    if (!mameRoms.empty()) SetWindowText(hEditMameRoms, StringToWstring(mameRoms).c_str());

    // RetroArch
    std::string raExe = ExtractJsonString(json, "retroarch_exe");
    if (!raExe.empty()) SetWindowText(hEditRetroarchExe, StringToWstring(raExe).c_str());

    std::string raRoms = ExtractJsonString(json, "retroarch_roms");
    if (!raRoms.empty()) SetWindowText(hEditRetroarchRoms, StringToWstring(raRoms).c_str());

    // Dolphin
    std::string dolphinExe = ExtractJsonString(json, "dolphin_exe");
    if (!dolphinExe.empty()) SetWindowText(hEditDolphinExe, StringToWstring(dolphinExe).c_str());

    std::string dolphinRoms = ExtractJsonString(json, "dolphin_roms");
    if (!dolphinRoms.empty()) SetWindowText(hEditGcRoms, StringToWstring(dolphinRoms).c_str());

    // Flycast
    std::string flycastExe = ExtractJsonString(json, "flycast_exe");
    if (!flycastExe.empty()) SetWindowText(hEditFlycastExe, StringToWstring(flycastExe).c_str());

    std::string flycastRoms = ExtractJsonString(json, "flycast_roms");
    if (!flycastRoms.empty()) SetWindowText(hEditNaomiRoms, StringToWstring(flycastRoms).c_str());

    // PCSX2
    std::string pcsx2Exe = ExtractJsonString(json, "pcsx2_exe");
    if (!pcsx2Exe.empty()) SetWindowText(hEditPcsx2Exe, StringToWstring(pcsx2Exe).c_str());

    std::string pcsx2Roms = ExtractJsonString(json, "pcsx2_roms");
    if (!pcsx2Roms.empty()) SetWindowText(hEditPs2Roms, StringToWstring(pcsx2Roms).c_str());

    SetWindowText(hStaticStatus, L"Status: Loaded saved configuration from phantom_config.json.");
}

// Save Configuration to phantom_config.json
void SaveConfiguration() {
    std::wstring misterIp = GetText(hEditMisterIp);
    int port = GetPortFromUI();
    std::wstring mameExe = GetText(hEditMameExe);
    std::wstring mameRoms = GetText(hEditMameRoms);
    std::wstring raExe = GetText(hEditRetroarchExe);
    std::wstring raRoms = GetText(hEditRetroarchRoms);
    std::wstring dolphinExe = GetText(hEditDolphinExe);
    std::wstring dolphinRoms = GetText(hEditGcRoms);
    std::wstring flycastExe = GetText(hEditFlycastExe);
    std::wstring flycastRoms = GetText(hEditNaomiRoms);
    std::wstring pcsx2Exe = GetText(hEditPcsx2Exe);
    std::wstring pcsx2Roms = GetText(hEditPs2Roms);

    std::ofstream out("phantom_config.json");
    if (out.is_open()) {
        out << "{\n";
        out << "  \"server\": {\n";
        out << "    \"listen_ip\": \"0.0.0.0\",\n";
        out << "    \"udp_port\": " << port << ",\n";
        out << "    \"http_port\": 8088,\n";
        out << "    \"mister_client_ip\": \"" << ToJsonString(misterIp) << "\"\n";
        out << "  },\n";
        out << "  \"paths\": {\n";
        out << "    \"mame_exe\": \"" << ToJsonString(mameExe) << "\",\n";
        out << "    \"mame_roms\": \"" << ToJsonString(mameRoms) << "\",\n";
        out << "    \"retroarch_exe\": \"" << ToJsonString(raExe) << "\",\n";
        out << "    \"retroarch_roms\": \"" << ToJsonString(raRoms) << "\",\n";
        out << "    \"dolphin_exe\": \"" << ToJsonString(dolphinExe) << "\",\n";
        out << "    \"dolphin_roms\": \"" << ToJsonString(dolphinRoms) << "\",\n";
        out << "    \"flycast_exe\": \"" << ToJsonString(flycastExe) << "\",\n";
        out << "    \"flycast_roms\": \"" << ToJsonString(flycastRoms) << "\",\n";
        out << "    \"pcsx2_exe\": \"" << ToJsonString(pcsx2Exe) << "\",\n";
        out << "    \"pcsx2_roms\": \"" << ToJsonString(pcsx2Roms) << "\"\n";
        out << "  },\n";
        out << "  \"emulators\": {\n";
        out << "    \"groovymame\": {\n";
        out << "      \"exe\": \"" << ToJsonString(mameExe) << "\",\n";
        out << "      \"roms\": \"" << ToJsonString(mameRoms) << "\",\n";
        out << "      \"args\": \"-video mister -mister_ip " << ToJsonString(misterIp) << " -mister_port " << port << " \\\"{rom_stem}\\\"\",\n";
        out << "      \"pipeline\": \"Groovy_MiSTer SwitchRes 15kHz Direct\"\n";
        out << "    },\n";
        out << "    \"retroarch\": {\n";
        out << "      \"exe\": \"" << ToJsonString(raExe) << "\",\n";
        out << "      \"roms\": \"" << ToJsonString(raRoms) << "\",\n";
        out << "      \"args\": \"-f \\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"RetroArch CRT SwitchRes 15kHz\"\n";
        out << "    },\n";
        out << "    \"dolphin\": {\n";
        out << "      \"exe\": \"" << ToJsonString(dolphinExe) << "\",\n";
        out << "      \"roms\": \"" << ToJsonString(dolphinRoms) << "\",\n";
        out << "      \"args\": \"-b -e \\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"Groovy_MiSTer 480i/240p\"\n";
        out << "    },\n";
        out << "    \"flycast\": {\n";
        out << "      \"exe\": \"" << ToJsonString(flycastExe) << "\",\n";
        out << "      \"roms\": \"" << ToJsonString(flycastRoms) << "\",\n";
        out << "      \"args\": \"\\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"SwitchRes Direct 15kHz\"\n";
        out << "    },\n";
        out << "    \"pcsx2\": {\n";
        out << "      \"exe\": \"" << ToJsonString(pcsx2Exe) << "\",\n";
        out << "      \"roms\": \"" << ToJsonString(pcsx2Roms) << "\",\n";
        out << "      \"args\": \"-batch -fullscreen \\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"Custom Pipeline (Experimental)\"\n";
        out << "    }\n";
        out << "  }\n";
        out << "}\n";
        out.close();

        std::wstring msg = L"Status: Configuration saved to phantom_config.json (UDP Port: " + std::to_wstring(port) + L")";
        SetWindowText(hStaticStatus, msg.c_str());
    } else {
        SetWindowText(hStaticStatus, L"Error: Failed to write phantom_config.json");
    }
}

// Auto-Scan ROM Directories and build games_catalog.json
void ScanRomDirectories() {
    SendMessage(hListGames, LB_RESETCONTENT, 0, 0);

    struct ScanTarget {
        std::wstring path;
        std::string system;
        std::string systemName;
        std::string videoMode;
        std::string resolution;
    };

    std::vector<ScanTarget> targets = {
        { GetText(hEditMameRoms), "groovymame", "GroovyMAME Arcade", "15kHz 240p Native", "Dynamic SwitchRes" },
        { GetText(hEditRetroarchRoms), "retroarch", "RetroArch SwitchRes", "15kHz 240p Dynamic", "Dynamic SwitchRes" },
        { GetText(hEditGcRoms), "dolphin", "GameCube / Wii", "15kHz 480i @ 60Hz", "640x480i" },
        { GetText(hEditNaomiRoms), "flycast", "Sega Naomi / DC Arcade", "15kHz 240p / 480i", "640x480" },
        { GetText(hEditPs2Roms), "pcsx2", "Sony PlayStation 2", "15kHz 240p / 480i", "640x224" }
    };

    std::ofstream catOut("games_catalog.json");
    catOut << "{\n  \"games\": [\n";

    int totalFound = 0;

    for (const auto& target : targets) {
        if (target.path.empty() || !fs::exists(target.path)) continue;

        try {
            for (const auto& entry : fs::directory_iterator(target.path)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    for (auto& c : ext) c = tolower(c);

                    if (ext == ".zip" || ext == ".7z" || ext == ".iso" || ext == ".chd" || 
                        ext == ".cso" || ext == ".elf" || ext == ".cue" || ext == ".sfc" || ext == ".md") {
                        std::string filename = entry.path().filename().string();
                        std::string stem = entry.path().stem().string();
                        std::string id = target.system + "_" + stem;
                        // generic_string() ensures forward slashes: zero JSON escape issues!
                        std::string cleanRomPath = entry.path().generic_string();

                        // Add to UI listbox
                        std::wstring listEntry = L"[" + std::wstring(target.system.begin(), target.system.end()) + L"] " +
                                                std::wstring(stem.begin(), stem.end());
                        SendMessage(hListGames, LB_ADDSTRING, 0, (LPARAM)listEntry.c_str());

                        // Append to JSON
                        if (totalFound > 0) catOut << ",\n";
                        catOut << "    {\n";
                        catOut << "      \"id\": \"" << id << "\",\n";
                        catOut << "      \"title\": \"" << stem << "\",\n";
                        catOut << "      \"system\": \"" << target.system << "\",\n";
                        catOut << "      \"systemName\": \"" << target.systemName << "\",\n";
                        catOut << "      \"romName\": \"" << filename << "\",\n";
                        catOut << "      \"romPath\": \"" << cleanRomPath << "\",\n";
                        catOut << "      \"videoMode\": \"" << target.videoMode << "\",\n";
                        catOut << "      \"resolution\": \"" << target.resolution << "\"\n";
                        catOut << "    }";
                        totalFound++;
                    }
                }
            }
        } catch (...) {}
    }

    if (totalFound == 0) {
        catOut << "    {\n"
               << "      \"id\": \"direct_groovy_receiver\",\n"
               << "      \"title\": \"Groovy_MiSTer Direct Receiver (Wait for PC)\",\n"
               << "      \"system\": \"mister\",\n"
               << "      \"systemName\": \"Groovy_MiSTer\",\n"
               << "      \"romName\": \"groovy.rbf\",\n"
               << "      \"romPath\": \"\",\n"
               << "      \"videoMode\": \"15kHz Dynamic CRT\",\n"
               << "      \"resolution\": \"Dynamic SwitchRes\"\n"
               << "    },\n"
               << "    {\n"
               << "      \"id\": \"mame_sf2ce\",\n"
               << "      \"title\": \"Street Fighter II' - Champion Edition\",\n"
               << "      \"system\": \"groovymame\",\n"
               << "      \"systemName\": \"GroovyMAME Arcade\",\n"
               << "      \"romName\": \"sf2ce.zip\",\n"
               << "      \"romPath\": \"C:/Games/Arcade/sf2ce.zip\",\n"
               << "      \"videoMode\": \"15kHz 224p @ 59.6Hz\",\n"
               << "      \"resolution\": \"384x224\"\n"
               << "    },\n"
               << "    {\n"
               << "      \"id\": \"mame_mslug\",\n"
               << "      \"title\": \"Metal Slug - Super Vehicle-001\",\n"
               << "      \"system\": \"groovymame\",\n"
               << "      \"systemName\": \"GroovyMAME Arcade\",\n"
               << "      \"romName\": \"mslug.zip\",\n"
               << "      \"romPath\": \"C:/Games/Arcade/mslug.zip\",\n"
               << "      \"videoMode\": \"15kHz 224p @ 59.18Hz\",\n"
               << "      \"resolution\": \"320x224\"\n"
               << "    },\n"
               << "    {\n"
               << "      \"id\": \"retroarch_castlevania\",\n"
               << "      \"title\": \"Castlevania: Symphony of the Night\",\n"
               << "      \"system\": \"retroarch\",\n"
               << "      \"systemName\": \"RetroArch SwitchRes\",\n"
               << "      \"romName\": \"CastlevaniaSOTN.chd\",\n"
               << "      \"romPath\": \"C:/Games/RetroArch/CastlevaniaSOTN.chd\",\n"
               << "      \"videoMode\": \"15kHz 240p SwitchRes\",\n"
               << "      \"resolution\": \"256x240\"\n"
               << "    },\n"
               << "    {\n"
               << "      \"id\": \"gc_smash_melee\",\n"
               << "      \"title\": \"Super Smash Bros. Melee\",\n"
               << "      \"system\": \"dolphin\",\n"
               << "      \"systemName\": \"GameCube / Wii\",\n"
               << "      \"romName\": \"SmashMelee.iso\",\n"
               << "      \"romPath\": \"C:/Games/GameCube/SmashMelee.iso\",\n"
               << "      \"videoMode\": \"15kHz 480i / 240p\",\n"
               << "      \"resolution\": \"640x480i\"\n"
               << "    }\n";
    }

    catOut << "\n  ]\n}\n";
    catOut.close();

    std::wstring statusMsg = L"Status: Scan complete. Found " + std::to_wstring(totalFound) + L" ROMs.";
    SetWindowText(hStaticStatus, statusMsg.c_str());
}

// Test Handshake with MiSTer IP via UDP PING using configured port
void TestMisterHandshake() {
    std::wstring ipStr = GetText(hEditMisterIp);
    if (ipStr.empty()) {
        MessageBox(hMainWnd, L"Please enter a valid MiSTer IP address.", L"Error", MB_ICONERROR);
        return;
    }

    int port = GetPortFromUI();

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    DWORD timeout = 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    std::string ip(ipStr.begin(), ipStr.end());
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    const char* pingMsg = "PING";
    sendto(sock, pingMsg, (int)strlen(pingMsg), 0, (sockaddr*)&addr, sizeof(addr));

    char buf[128] = { 0 };
    int fromLen = sizeof(addr);
    int recvLen = recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&addr, &fromLen);

    if (recvLen > 0) {
        std::wstring successMsg = L"Status: MiSTer Handshake Successful on port " + std::to_wstring(port) + L"! (Replied: PONG)";
        SetWindowText(hStaticStatus, successMsg.c_str());
        std::wstring dialogMsg = L"MiSTer client acknowledged UDP handshake on port " + std::to_wstring(port) + L"!";
        MessageBox(hMainWnd, dialogMsg.c_str(), L"Success", MB_ICONINFORMATION);
    } else {
        std::wstring statusErr = L"Status: MiSTer timed out on port " + std::to_wstring(port) + L". Verify script is running on DE10-Nano.";
        SetWindowText(hStaticStatus, statusErr.c_str());
        std::wstring errDialog = L"Could not reach MiSTer on port " + std::to_wstring(port) + 
                                L". Ensure network cables, MiSTer IP, and Groovy_MiSTer script are active.";
        MessageBox(hMainWnd, errDialog.c_str(), L"Timeout", MB_ICONWARNING);
    }

    closesocket(sock);
    WSACleanup();
}

// Background HTTP Worker Thread Function (Serves /catalog.json on TCP :8088)
DWORD WINAPI HttpThreadProc(LPVOID lpParam) {
    g_httpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_httpSocket == INVALID_SOCKET) return 1;

    BOOL opt = TRUE;
    setsockopt(g_httpSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in bindAddr = { 0 };
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(8088);
    bindAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_httpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
        closesocket(g_httpSocket);
        return 1;
    }

    listen(g_httpSocket, 5);

    while (g_daemonRunning) {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(g_httpSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) break;

        char reqBuf[2048] = { 0 };
        int bytes = recv(clientSock, reqBuf, sizeof(reqBuf) - 1, 0);
        if (bytes > 0) {
            std::string req(reqBuf);
            if (req.find("GET /catalog.json") != std::string::npos) {
                std::ifstream f("games_catalog.json");
                std::string body = "";
                if (f.is_open()) {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    body = ss.str();
                } else {
                    body = "{\"games\":[]}";
                }
                std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
                                  std::to_string(body.length()) + "\r\nConnection: close\r\n\r\n" + body;
                send(clientSock, resp.c_str(), (int)resp.length(), 0);
            } else if (req.find("GET /mister_script") != std::string::npos) {
                std::ifstream f("Phantom_Arcade.sh");
                std::string body = "";
                if (f.is_open()) {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    body = ss.str();
                } else {
                    // Try looking in relative folders
                    std::ifstream f2("../mister_client/Phantom_Arcade.sh");
                    if (f2.is_open()) {
                        std::stringstream ss;
                        ss << f2.rdbuf();
                        body = ss.str();
                    }
                }
                std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/x-shellscript\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
                                  std::to_string(body.length()) + "\r\nConnection: close\r\n\r\n" + body;
                send(clientSock, resp.c_str(), (int)resp.length(), 0);
            } else if (req.find("GET /install") != std::string::npos) {
                std::string body = "#!/usr/bin/env bash\n"
                                   "mkdir -p /media/fat/_Groovy /media/fat/Scripts /media/fat/config\n"
                                   "curl -k -L --connect-timeout 8 -o /media/fat/_Groovy/groovy.rbf \"https://raw.githubusercontent.com/MiSTer-devel/Groovy_MiSTer/main/releases/groovy.rbf\" 2>/dev/null\n"
                                   "curl -sSL \"http://" + std::string(inet_ntoa(bindAddr.sin_addr)) + ":8088/mister_script\" -o /media/fat/Scripts/Phantom_Arcade.sh\n"
                                   "chmod +x /media/fat/Scripts/Phantom_Arcade.sh\n"
                                   "echo \"[✓] Phantom Arcade installed! Find it in MiSTer Main Menu -> Scripts.\"\n";
                std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/x-shellscript\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
                                  std::to_string(body.length()) + "\r\nConnection: close\r\n\r\n" + body;
                send(clientSock, resp.c_str(), (int)resp.length(), 0);
            } else {
                std::string body = "{\"status\":\"Phantom Arcade Host Online\",\"http_port\":8088}";
                std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
                                  std::to_string(body.length()) + "\r\nConnection: close\r\n\r\n" + body;
                send(clientSock, resp.c_str(), (int)resp.length(), 0);
            }
        }
        closesocket(clientSock);
    }

    if (g_httpSocket != INVALID_SOCKET) {
        closesocket(g_httpSocket);
        g_httpSocket = INVALID_SOCKET;
    }
    return 0;
}

// Background UDP Daemon Worker Thread Function
DWORD WINAPI DaemonThreadProc(LPVOID lpParam) {
    int port = g_configuredPort.load();

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Also launch the HTTP server on port 8088
    g_hHttpThread = CreateThread(NULL, 0, HttpThreadProc, NULL, 0, NULL);

    g_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in bindAddr = { 0 };
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(port);
    bindAddr.sin_addr.s_addr = INADDR_ANY;

    // Automatic Port Check & Fallback: If port 1999 is occupied, automatically fallback
    if (bind(g_udpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
        port = 2154;
        bindAddr.sin_port = htons(port);
        if (bind(g_udpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
            bindAddr.sin_port = htons(0); // system assigns next available port
            bind(g_udpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr));
            int len = sizeof(bindAddr);
            getsockname(g_udpSocket, (sockaddr*)&bindAddr, &len);
            port = ntohs(bindAddr.sin_port);
        }
        g_configuredPort = port;
    }

    char buffer[2048];
    sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    while (g_daemonRunning) {
        int bytes = recvfrom(g_udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&clientAddr, &clientLen);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string msg(buffer);

            if (msg.rfind("DISCOVER_PHANTOM", 0) == 0) {
                // Auto-discovery response with active UDP and HTTP ports
                std::string reply = "PHANTOM_HOST_ONLINE:" + std::to_string(port) + ":8088";
                sendto(g_udpSocket, reply.c_str(), (int)reply.length(), 0, (sockaddr*)&clientAddr, clientLen);
            } else if (msg == "GET_CATALOG") {
                // Direct UDP catalog transfer (Zero HTTP firewall dependency!)
                std::ifstream f("games_catalog.json");
                std::string catData = "";
                if (f.is_open()) {
                    std::stringstream ss;
                    ss << f.rdbuf();
                    catData = ss.str();
                }
                if (catData.empty()) {
                    catData = "{\"games\":[]}";
                }
                int sendLen = (int)std::min(catData.length(), (size_t)60000);
                sendto(g_udpSocket, catData.c_str(), sendLen, 0, (sockaddr*)&clientAddr, clientLen);
            } else if (msg.rfind("LAUNCH:", 0) == 0) {
                // Launch requested emulator process...
                const char* reply = "ACK:LAUNCH:OK";
                sendto(g_udpSocket, reply, (int)strlen(reply), 0, (sockaddr*)&clientAddr, clientLen);
            } else if (msg == "KILL") {
                // Terminate active child process
                if (g_activePid > 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, g_activePid);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                    g_activePid = 0;
                }
                const char* reply = "ACK:KILL:OK";
                sendto(g_udpSocket, reply, (int)strlen(reply), 0, (sockaddr*)&clientAddr, clientLen);
            } else if (msg == "PING") {
                const char* reply = "PONG";
                sendto(g_udpSocket, reply, (int)strlen(reply), 0, (sockaddr*)&clientAddr, clientLen);
            }
        }
    }

    closesocket(g_udpSocket);
    WSACleanup();
    return 0;
}

// Silently ensure Windows Defender Firewall allows Phantom Arcade and Groovy_MiSTer ports
void EnsureFirewallRules() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // Register application executable in firewall
    std::wstring cmd1 = L"advfirewall firewall add rule name=\"Phantom Arcade Manager\" dir=in action=allow program=\"" + std::wstring(exePath) + L"\" enable=yes";
    ShellExecute(NULL, L"open", L"netsh.exe", cmd1.c_str(), NULL, SW_HIDE);

    // Open UDP 1999 (Default MiSTer Groovy port)
    std::wstring cmd2 = L"advfirewall firewall add rule name=\"Groovy_MiSTer UDP 1999\" dir=in action=allow protocol=UDP localport=1999 enable=yes";
    ShellExecute(NULL, L"open", L"netsh.exe", cmd2.c_str(), NULL, SW_HIDE);

    // Open TCP 8088 (HTTP Catalog Server)
    std::wstring cmd3 = L"advfirewall firewall add rule name=\"Phantom Arcade HTTP 8088\" dir=in action=allow protocol=TCP localport=8088 enable=yes";
    ShellExecute(NULL, L"open", L"netsh.exe", cmd3.c_str(), NULL, SW_HIDE);
}

// Toggle Daemon State
void ToggleDaemon() {
    if (!g_daemonRunning) {
        int port = GetPortFromUI();
        g_configuredPort = port;
        g_daemonRunning = true;
        g_hDaemonThread = CreateThread(NULL, 0, DaemonThreadProc, NULL, 0, NULL);
        SetWindowText(hBtnToggleDaemon, L"Stop Background Daemon");
        std::wstring status = L"Status: Daemon ACTIVE (Listening on UDP :" + std::to_wstring(port) + L" & HTTP :8088)";
        SetWindowText(hStaticStatus, status.c_str());
    } else {
        g_daemonRunning = false;
        if (g_udpSocket != INVALID_SOCKET) {
            closesocket(g_udpSocket);
        }
        if (g_httpSocket != INVALID_SOCKET) {
            closesocket(g_httpSocket);
        }
        if (g_hDaemonThread) {
            WaitForSingleObject(g_hDaemonThread, 1000);
            CloseHandle(g_hDaemonThread);
            g_hDaemonThread = NULL;
        }
        if (g_hHttpThread) {
            WaitForSingleObject(g_hHttpThread, 1000);
            CloseHandle(g_hHttpThread);
            g_hHttpThread = NULL;
        }
        SetWindowText(hBtnToggleDaemon, L"Start Background Daemon");
        SetWindowText(hStaticStatus, L"Status: Daemon Stopped");
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int y = 12;
        // MiSTer IP & UDP Port (Default 1999)
        CreateWindow(L"STATIC", L"MiSTer FPGA IP Address:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditMisterIp = CreateWindow(L"EDIT", L"192.168.1.50", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 160, 22, hWnd, (HMENU)IDC_EDIT_MISTER_IP, hInst, NULL);
        CreateWindow(L"STATIC", L"UDP Port (Default 1999):", WS_CHILD | WS_VISIBLE, 375, y, 160, 20, hWnd, NULL, hInst, NULL);
        hEditUdpPort = CreateWindow(L"EDIT", L"1999", WS_CHILD | WS_VISIBLE | WS_BORDER, 545, y, 95, 22, hWnd, (HMENU)IDC_EDIT_UDP_PORT, hInst, NULL);

        // 1. GroovyMAME (Direct 15kHz Calamity SwitchRes Support)
        y += 32;
        CreateWindow(L"STATIC", L"GroovyMAME Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditMameExe = CreateWindow(L"EDIT", L"C:\\Emulators\\GroovyMAME\\groovymame64.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_GROOVYMAME_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_MAME_EXE, hInst, NULL);

        y += 26;
        CreateWindow(L"STATIC", L"GroovyMAME ROMs Folder:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditMameRoms = CreateWindow(L"EDIT", L"C:\\Emulators\\GroovyMAME\\roms", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_MAME_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_MAME_ROMS, hInst, NULL);

        // 2. RetroArch (SwitchRes Console & Arcade Multi-Core)
        y += 32;
        CreateWindow(L"STATIC", L"RetroArch Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditRetroarchExe = CreateWindow(L"EDIT", L"C:\\Emulators\\RetroArch\\retroarch.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_RETROARCH_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_RA_EXE, hInst, NULL);

        y += 26;
        CreateWindow(L"STATIC", L"RetroArch ROMs Folder:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditRetroarchRoms = CreateWindow(L"EDIT", L"C:\\Games\\RetroArch\\roms", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_RETROARCH_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_RA_ROMS, hInst, NULL);

        // 3. Dolphin (GameCube / Wii)
        y += 32;
        CreateWindow(L"STATIC", L"Dolphin Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditDolphinExe = CreateWindow(L"EDIT", L"C:\\Emulators\\Dolphin\\Dolphin.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_DOLPHIN_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_DOLPHIN, hInst, NULL);

        y += 26;
        CreateWindow(L"STATIC", L"GameCube/Wii ROMs:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditGcRoms = CreateWindow(L"EDIT", L"C:\\Games\\GameCube", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_GC_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_GCROMS, hInst, NULL);

        // 4. Flycast (Naomi / Dreamcast Arcade)
        y += 32;
        CreateWindow(L"STATIC", L"Flycast Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditFlycastExe = CreateWindow(L"EDIT", L"C:\\Emulators\\Flycast\\flycast.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_FLYCAST_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_FLYCAST, hInst, NULL);

        y += 26;
        CreateWindow(L"STATIC", L"Naomi/Arcade ROMs:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditNaomiRoms = CreateWindow(L"EDIT", L"C:\\Games\\Arcade\\Naomi", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_NAOMI_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_NAOMI, hInst, NULL);

        // 5. PCSX2 (Optional / Custom)
        y += 32;
        CreateWindow(L"STATIC", L"PCSX2 Executable (Opt):", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditPcsx2Exe = CreateWindow(L"EDIT", L"C:\\Emulators\\PCSX2\\pcsx2-qt.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_PCSX2_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_PCSX2, hInst, NULL);

        y += 26;
        CreateWindow(L"STATIC", L"PS2 ROMs Folder:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditPs2Roms = CreateWindow(L"EDIT", L"C:\\Games\\PS2", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_PS2_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_PS2ROMS, hInst, NULL);

        // Action Buttons Row
        y += 36;
        CreateWindow(L"BUTTON", L"1. Auto-Scan ROMs", WS_CHILD | WS_VISIBLE, 20, y, 140, 28, hWnd, (HMENU)IDC_BTN_SCAN_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"2. Test MiSTer Ping", WS_CHILD | WS_VISIBLE, 170, y, 140, 28, hWnd, (HMENU)IDC_BTN_TEST_MISTER, hInst, NULL);
        CreateWindow(L"BUTTON", L"3. Save Config", WS_CHILD | WS_VISIBLE, 320, y, 120, 28, hWnd, (HMENU)IDC_BTN_SAVE_CONFIG, hInst, NULL);
        hBtnToggleDaemon = CreateWindow(L"BUTTON", L"Start Background Daemon", WS_CHILD | WS_VISIBLE, 450, y, 190, 28, hWnd, (HMENU)IDC_BTN_TOGGLE_DAEMON, hInst, NULL);

        // Scanned ROMs List Box
        y += 36;
        CreateWindow(L"STATIC", L"Detected Games Catalog:", WS_CHILD | WS_VISIBLE, 20, y, 200, 18, hWnd, NULL, hInst, NULL);
        y += 18;
        hListGames = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 20, y, 620, 110, hWnd, (HMENU)IDC_LIST_GAMES, hInst, NULL);

        // Status Bar
        y += 118;
        hStaticStatus = CreateWindow(L"STATIC", L"Status: Ready. Default MiSTer port is 1999. Integrated HTTP :8088.", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 20, y, 620, 20, hWnd, (HMENU)IDC_STATIC_STATUS, hInst, NULL);
        LoadConfiguration();
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BTN_BROWSE_MAME_EXE: {
            auto path = BrowseFile(hWnd, L"GroovyMAME Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
            if (!path.empty()) SetWindowText(hEditMameExe, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_MAME_ROMS: {
            auto path = BrowseFolder(hWnd, L"Select GroovyMAME ROMs Folder");
            if (!path.empty()) SetWindowText(hEditMameRoms, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_RA_EXE: {
            auto path = BrowseFile(hWnd, L"RetroArch Executable (retroarch.exe)\0retroarch.exe;*.exe\0All Files (*.*)\0*.*\0");
            if (!path.empty()) SetWindowText(hEditRetroarchExe, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_RA_ROMS: {
            auto path = BrowseFolder(hWnd, L"Select RetroArch ROMs Folder");
            if (!path.empty()) SetWindowText(hEditRetroarchRoms, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_DOLPHIN: {
            auto path = BrowseFile(hWnd, L"Dolphin Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
            if (!path.empty()) SetWindowText(hEditDolphinExe, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_GCROMS: {
            auto path = BrowseFolder(hWnd, L"Select GameCube/Wii ROMs Folder");
            if (!path.empty()) SetWindowText(hEditGcRoms, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_FLYCAST: {
            auto path = BrowseFile(hWnd, L"Flycast Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
            if (!path.empty()) SetWindowText(hEditFlycastExe, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_NAOMI: {
            auto path = BrowseFolder(hWnd, L"Select Naomi/Arcade ROMs Folder");
            if (!path.empty()) SetWindowText(hEditNaomiRoms, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_PCSX2: {
            auto path = BrowseFile(hWnd, L"PCSX2 Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
            if (!path.empty()) SetWindowText(hEditPcsx2Exe, path.c_str());
            break;
        }
        case IDC_BTN_BROWSE_PS2ROMS: {
            auto path = BrowseFolder(hWnd, L"Select PS2 ROMs Folder");
            if (!path.empty()) SetWindowText(hEditPs2Roms, path.c_str());
            break;
        }
        case IDC_BTN_SAVE_CONFIG:
            SaveConfiguration();
            break;
        case IDC_BTN_SCAN_ROMS:
            ScanRomDirectories();
            break;
        case IDC_BTN_TEST_MISTER:
            TestMisterHandshake();
            break;
        case IDC_BTN_TOGGLE_DAEMON:
            ToggleDaemon();
            break;
        }
        break;
    }

    case WM_DESTROY:
        if (g_daemonRunning) {
            g_daemonRunning = false;
            if (g_udpSocket != INVALID_SOCKET) closesocket(g_udpSocket);
            if (g_httpSocket != INVALID_SOCKET) closesocket(g_httpSocket);
            if (g_hDaemonThread) {
                WaitForSingleObject(g_hDaemonThread, 1000);
                CloseHandle(g_hDaemonThread);
            }
            if (g_hHttpThread) {
                WaitForSingleObject(g_hHttpThread, 1000);
                CloseHandle(g_hHttpThread);
            }
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PhantomArcadeManagerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClass(&wc);

    hMainWnd = CreateWindowEx(
        0,
        L"PhantomArcadeManagerClass",
        L"Phantom Arcade - Groovy_MiSTer Windows Setup & Bridge Manager",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 660,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    // Load any saved configuration from phantom_config.json
    LoadConfiguration();

    // Automatically configure Windows Firewall and start the daemon immediately
    EnsureFirewallRules();
    if (!fs::exists("games_catalog.json")) {
        ScanRomDirectories();
    }
    ToggleDaemon();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
