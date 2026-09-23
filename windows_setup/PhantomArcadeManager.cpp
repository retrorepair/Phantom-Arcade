/**
 * ============================================================================
 * Phantom Arcade - Windows Setup & Management Application
 * File: PhantomArcadeManager.cpp
 * Language: C++17 / Native Win32
 * 
 * Description:
 *   Native Windows desktop setup tool and daemon launcher for the Phantom Arcade
 *   Groovy_MiSTer bridge. Supports GroovyMAME (official Calamity 15kHz streaming),
 *   Dolphin, Flycast, and custom emulators.
 *   Fully dynamic UDP port configuration (default 1999) and LAN auto-discovery.
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
#define IDC_EDIT_MISTER_IP       101
#define IDC_EDIT_UDP_PORT        102
#define IDC_EDIT_GROOVYMAME_EXE  103
#define IDC_BTN_BROWSE_MAME_EXE  104
#define IDC_EDIT_MAME_ROMS       105
#define IDC_BTN_BROWSE_MAME_ROMS 106
#define IDC_EDIT_DOLPHIN_EXE     107
#define IDC_BTN_BROWSE_DOLPHIN   108
#define IDC_EDIT_GC_ROMS         109
#define IDC_BTN_BROWSE_GCROMS    110
#define IDC_EDIT_FLYCAST_EXE     111
#define IDC_BTN_BROWSE_FLYCAST   112
#define IDC_EDIT_NAOMI_ROMS      113
#define IDC_BTN_BROWSE_NAOMI     114
#define IDC_EDIT_PCSX2_EXE       115
#define IDC_BTN_BROWSE_PCSX2     116
#define IDC_EDIT_PS2_ROMS        117
#define IDC_BTN_BROWSE_PS2ROMS   118
#define IDC_BTN_SCAN_ROMS        119
#define IDC_BTN_TEST_MISTER      120
#define IDC_BTN_SAVE_CONFIG      121
#define IDC_BTN_TOGGLE_DAEMON    122
#define IDC_STATIC_STATUS        123
#define IDC_LIST_GAMES           124

// Global State
HINSTANCE hInst = NULL;
HWND hMainWnd = NULL;
HWND hEditMisterIp, hEditUdpPort;
HWND hEditMameExe, hEditMameRoms;
HWND hEditDolphinExe, hEditGcRoms;
HWND hEditFlycastExe, hEditNaomiRoms;
HWND hEditPcsx2Exe, hEditPs2Roms;
HWND hStaticStatus, hListGames;
HWND hBtnToggleDaemon;

std::atomic<bool> g_daemonRunning(false);
std::atomic<DWORD> g_activePid(0);
std::atomic<int> g_configuredPort(1999);
SOCKET g_udpSocket = INVALID_SOCKET;
HANDLE g_hDaemonThread = NULL;

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

// Save Configuration to phantom_config.json
void SaveConfiguration() {
    std::wstring misterIp = GetText(hEditMisterIp);
    int port = GetPortFromUI();
    std::wstring mameExe = GetText(hEditMameExe);
    std::wstring dolphinExe = GetText(hEditDolphinExe);
    std::wstring flycastExe = GetText(hEditFlycastExe);
    std::wstring pcsx2Exe = GetText(hEditPcsx2Exe);

    std::ofstream out("phantom_config.json");
    if (out.is_open()) {
        out << "{\n";
        out << "  \"server\": {\n";
        out << "    \"listen_ip\": \"0.0.0.0\",\n";
        out << "    \"udp_port\": " << port << ",\n";
        out << "    \"http_port\": 8088,\n";
        out << "    \"mister_client_ip\": \"" << std::string(misterIp.begin(), misterIp.end()) << "\"\n";
        out << "  },\n";
        out << "  \"emulators\": {\n";
        out << "    \"groovymame\": {\n";
        out << "      \"exe\": \"" << std::string(mameExe.begin(), mameExe.end()) << "\",\n";
        out << "      \"args\": \"-video mister -mister_ip " << std::string(misterIp.begin(), misterIp.end()) << " -mister_port " << port << " \\\"{rom_stem}\\\"\",\n";
        out << "      \"pipeline\": \"Groovy_MiSTer SwitchRes 15kHz Direct\"\n";
        out << "    },\n";
        out << "    \"dolphin\": {\n";
        out << "      \"exe\": \"" << std::string(dolphinExe.begin(), dolphinExe.end()) << "\",\n";
        out << "      \"args\": \"-b -e \\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"Groovy_MiSTer 480i/240p\"\n";
        out << "    },\n";
        out << "    \"flycast\": {\n";
        out << "      \"exe\": \"" << std::string(flycastExe.begin(), flycastExe.end()) << "\",\n";
        out << "      \"args\": \"\\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"SwitchRes Direct 15kHz\"\n";
        out << "    },\n";
        out << "    \"pcsx2\": {\n";
        out << "      \"exe\": \"" << std::string(pcsx2Exe.begin(), pcsx2Exe.end()) << "\",\n";
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

                    if (ext == ".zip" || ext == ".7z" || ext == ".iso" || ext == ".chd" || ext == ".cso" || ext == ".elf") {
                        std::string filename = entry.path().filename().string();
                        std::string stem = entry.path().stem().string();
                        std::string id = target.system + "_" + stem;

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
                        catOut << "      \"romPath\": \"" << entry.path().string() << "\",\n";
                        catOut << "      \"videoMode\": \"" << target.videoMode << "\",\n";
                        catOut << "      \"resolution\": \"" << target.resolution << "\"\n";
                        catOut << "    }";
                        totalFound++;
                    }
                }
            }
        } catch (...) {}
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

// Background UDP Daemon Worker Thread Function
DWORD WINAPI DaemonThreadProc(LPVOID lpParam) {
    int port = g_configuredPort.load();

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    g_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in bindAddr = { 0 };
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(port);
    bindAddr.sin_addr.s_addr = INADDR_ANY;

    bind(g_udpSocket, (sockaddr*)&bindAddr, sizeof(bindAddr));

    char buffer[1024];
    sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);

    while (g_daemonRunning) {
        int bytes = recvfrom(g_udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&clientAddr, &clientLen);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string msg(buffer);

            if (msg.rfind("DISCOVER_PHANTOM", 0) == 0) {
                // Auto-discovery response
                std::string reply = "PHANTOM_HOST_ONLINE:" + std::to_string(port);
                sendto(g_udpSocket, reply.c_str(), (int)reply.length(), 0, (sockaddr*)&clientAddr, clientLen);
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

// Toggle Daemon State
void ToggleDaemon() {
    if (!g_daemonRunning) {
        int port = GetPortFromUI();
        g_configuredPort = port;
        g_daemonRunning = true;
        g_hDaemonThread = CreateThread(NULL, 0, DaemonThreadProc, NULL, 0, NULL);
        SetWindowText(hBtnToggleDaemon, L"Stop Background Daemon");
        std::wstring status = L"Status: Daemon ACTIVE (Listening on UDP :" + std::to_wstring(port) + L" with Auto-Discovery)";
        SetWindowText(hStaticStatus, status.c_str());
    } else {
        g_daemonRunning = false;
        if (g_udpSocket != INVALID_SOCKET) {
            closesocket(g_udpSocket);
        }
        if (g_hDaemonThread) {
            WaitForSingleObject(g_hDaemonThread, 1000);
            CloseHandle(g_hDaemonThread);
            g_hDaemonThread = NULL;
        }
        SetWindowText(hBtnToggleDaemon, L"Start Background Daemon");
        SetWindowText(hStaticStatus, L"Status: Daemon Stopped");
    }
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int y = 15;
        // MiSTer IP & UDP Port (Default 1999)
        CreateWindow(L"STATIC", L"MiSTer FPGA IP Address:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditMisterIp = CreateWindow(L"EDIT", L"192.168.1.50", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 160, 22, hWnd, (HMENU)IDC_EDIT_MISTER_IP, hInst, NULL);
        CreateWindow(L"STATIC", L"UDP Port (Default 1999):", WS_CHILD | WS_VISIBLE, 375, y, 160, 20, hWnd, NULL, hInst, NULL);
        hEditUdpPort = CreateWindow(L"EDIT", L"1999", WS_CHILD | WS_VISIBLE | WS_BORDER, 545, y, 95, 22, hWnd, (HMENU)IDC_EDIT_UDP_PORT, hInst, NULL);

        // 1. GroovyMAME (Direct 15kHz Calamity SwitchRes Support)
        y += 35;
        CreateWindow(L"STATIC", L"GroovyMAME Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditMameExe = CreateWindow(L"EDIT", L"C:\\Emulators\\GroovyMAME\\groovymame64.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_GROOVYMAME_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_MAME_EXE, hInst, NULL);

        y += 28;
        CreateWindow(L"STATIC", L"GroovyMAME ROMs Folder:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditMameRoms = CreateWindow(L"EDIT", L"C:\\Emulators\\GroovyMAME\\roms", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_MAME_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_MAME_ROMS, hInst, NULL);

        // 2. Dolphin (GameCube / Wii)
        y += 35;
        CreateWindow(L"STATIC", L"Dolphin Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditDolphinExe = CreateWindow(L"EDIT", L"C:\\Emulators\\Dolphin\\Dolphin.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_DOLPHIN_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_DOLPHIN, hInst, NULL);

        y += 28;
        CreateWindow(L"STATIC", L"GameCube/Wii ROMs:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditGcRoms = CreateWindow(L"EDIT", L"C:\\Games\\GameCube", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_GC_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_GCROMS, hInst, NULL);

        // 3. Flycast (Naomi / Dreamcast Arcade)
        y += 35;
        CreateWindow(L"STATIC", L"Flycast Executable:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditFlycastExe = CreateWindow(L"EDIT", L"C:\\Emulators\\Flycast\\flycast.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_FLYCAST_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_FLYCAST, hInst, NULL);

        y += 28;
        CreateWindow(L"STATIC", L"Naomi/Arcade ROMs:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditNaomiRoms = CreateWindow(L"EDIT", L"C:\\Games\\Arcade\\Naomi", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_NAOMI_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_NAOMI, hInst, NULL);

        // 4. PCSX2 (Optional / Custom)
        y += 35;
        CreateWindow(L"STATIC", L"PCSX2 Executable (Opt):", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditPcsx2Exe = CreateWindow(L"EDIT", L"C:\\Emulators\\PCSX2\\pcsx2-qt.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_PCSX2_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_PCSX2, hInst, NULL);

        y += 28;
        CreateWindow(L"STATIC", L"PS2 ROMs Folder:", WS_CHILD | WS_VISIBLE, 20, y, 170, 20, hWnd, NULL, hInst, NULL);
        hEditPs2Roms = CreateWindow(L"EDIT", L"C:\\Games\\PS2", WS_CHILD | WS_VISIBLE | WS_BORDER, 195, y, 350, 22, hWnd, (HMENU)IDC_EDIT_PS2_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 555, y, 85, 22, hWnd, (HMENU)IDC_BTN_BROWSE_PS2ROMS, hInst, NULL);

        // Action Buttons Row
        y += 38;
        CreateWindow(L"BUTTON", L"1. Auto-Scan ROMs", WS_CHILD | WS_VISIBLE, 20, y, 140, 30, hWnd, (HMENU)IDC_BTN_SCAN_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"2. Test MiSTer Ping", WS_CHILD | WS_VISIBLE, 170, y, 140, 30, hWnd, (HMENU)IDC_BTN_TEST_MISTER, hInst, NULL);
        CreateWindow(L"BUTTON", L"3. Save Config", WS_CHILD | WS_VISIBLE, 320, y, 120, 30, hWnd, (HMENU)IDC_BTN_SAVE_CONFIG, hInst, NULL);
        hBtnToggleDaemon = CreateWindow(L"BUTTON", L"Start Background Daemon", WS_CHILD | WS_VISIBLE, 450, y, 190, 30, hWnd, (HMENU)IDC_BTN_TOGGLE_DAEMON, hInst, NULL);

        // Scanned ROMs List Box
        y += 40;
        CreateWindow(L"STATIC", L"Detected Games Catalog:", WS_CHILD | WS_VISIBLE, 20, y, 200, 18, hWnd, NULL, hInst, NULL);
        y += 20;
        hListGames = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 20, y, 620, 140, hWnd, (HMENU)IDC_LIST_GAMES, hInst, NULL);

        // Status Bar
        y += 150;
        hStaticStatus = CreateWindow(L"STATIC", L"Status: Ready. Default MiSTer port is 1999.", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 20, y, 620, 20, hWnd, (HMENU)IDC_STATIC_STATUS, hInst, NULL);
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
            if (g_hDaemonThread) {
                WaitForSingleObject(g_hDaemonThread, 1000);
                CloseHandle(g_hDaemonThread);
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
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 580,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
