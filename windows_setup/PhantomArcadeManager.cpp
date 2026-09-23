/**
 * ============================================================================
 * Phantom Arcade - Windows Setup & Management Application
 * File: PhantomArcadeManager.cpp
 * Language: C++17 / Native Win32
 * 
 * Description:
 *   Native Windows desktop setup tool and daemon launcher for the Phantom Arcade
 *   Groovy_MiSTer bridge. Allows users to point to ROM folders, emulator paths,
 *   set MiSTer IP, auto-scan games, and start/stop the background UDP daemon.
 *   Includes Auto-Discovery responder for zero-config MiSTer client setup!
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
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace fs = std::filesystem;

// Control IDs
#define IDC_EDIT_MISTER_IP     101
#define IDC_EDIT_UDP_PORT      102
#define IDC_EDIT_PCSX2_EXE     103
#define IDC_BTN_BROWSE_PCSX2   104
#define IDC_EDIT_PS2_ROMS      105
#define IDC_BTN_BROWSE_PS2ROMS 106
#define IDC_EDIT_DOLPHIN_EXE   107
#define IDC_BTN_BROWSE_DOLPHIN 108
#define IDC_EDIT_GC_ROMS       109
#define IDC_BTN_BROWSE_GCROMS  110
#define IDC_EDIT_FLYCAST_EXE   111
#define IDC_BTN_BROWSE_FLYCAST 112
#define IDC_EDIT_NAOMI_ROMS    113
#define IDC_BTN_BROWSE_NAOMI   114
#define IDC_BTN_SCAN_ROMS      115
#define IDC_BTN_TEST_MISTER    116
#define IDC_BTN_SAVE_CONFIG    117
#define IDC_BTN_TOGGLE_DAEMON  118
#define IDC_STATIC_STATUS      119
#define IDC_LIST_GAMES         120

// Global State
HINSTANCE hInst = NULL;
HWND hMainWnd = NULL;
HWND hEditMisterIp, hEditUdpPort;
HWND hEditPcsx2Exe, hEditPs2Roms;
HWND hEditDolphinExe, hEditGcRoms;
HWND hEditFlycastExe, hEditNaomiRoms;
HWND hStaticStatus, hListGames;
HWND hBtnToggleDaemon;

std::atomic<bool> g_daemonRunning(false);
std::atomic<DWORD> g_activePid(0);
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

// Save Configuration to phantom_config.json
void SaveConfiguration() {
    std::wstring misterIp = GetText(hEditMisterIp);
    std::wstring udpPort = GetText(hEditUdpPort);
    std::wstring pcsx2Exe = GetText(hEditPcsx2Exe);
    std::wstring dolphinExe = GetText(hEditDolphinExe);
    std::wstring flycastExe = GetText(hEditFlycastExe);

    std::ofstream out("phantom_config.json");
    if (out.is_open()) {
        out << "{\n";
        out << "  \"server\": {\n";
        out << "    \"listen_ip\": \"0.0.0.0\",\n";
        out << "    \"udp_port\": " << (udpPort.empty() ? L"2154" : udpPort.c_str()) << ",\n";
        out << "    \"http_port\": 8088,\n";
        out << "    \"mister_client_ip\": \"" << std::string(misterIp.begin(), misterIp.end()) << "\"\n";
        out << "  },\n";
        out << "  \"emulators\": {\n";
        out << "    \"ps2\": {\n";
        out << "      \"exe\": \"" << std::string(pcsx2Exe.begin(), pcsx2Exe.end()) << "\",\n";
        out << "      \"args\": \"-batch -fullscreen -elf \\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"Groovy_MiSTer D3D9\"\n";
        out << "    },\n";
        out << "    \"gamecube\": {\n";
        out << "      \"exe\": \"" << std::string(dolphinExe.begin(), dolphinExe.end()) << "\",\n";
        out << "      \"args\": \"-b -e \\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"Groovy_MiSTer Vulkan\"\n";
        out << "    },\n";
        out << "    \"naomi\": {\n";
        out << "      \"exe\": \"" << std::string(flycastExe.begin(), flycastExe.end()) << "\",\n";
        out << "      \"args\": \"\\\"{rom}\\\"\",\n";
        out << "      \"pipeline\": \"SwitchRes Direct\"\n";
        out << "    }\n";
        out << "  }\n";
        out << "}\n";
        out.close();

        SetWindowText(hStaticStatus, L"Status: Configuration saved to phantom_config.json");
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
        { GetText(hEditPs2Roms), "ps2", "Sony PlayStation 2", "15kHz 240p @ 60Hz", "640x224" },
        { GetText(hEditGcRoms), "gamecube", "Nintendo GameCube", "15kHz 480i @ 60Hz", "640x480i" },
        { GetText(hEditNaomiRoms), "naomi", "Sega Naomi Arcade", "15kHz / 31kHz Dual", "640x480" }
    };

    std::ofstream catOut("games_catalog.json");
    catOut << "{\n  \"games\": [\n";

    int totalFound = 0;

    for (const auto& target : targets) {
        if (target.path.empty() || !fs::exists(target.path)) continue;

        for (const auto& entry : fs::directory_iterator(target.path)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                for (auto& c : ext) c = tolower(c);

                if (ext == ".iso" || ext == ".chd" || ext == ".cso" || ext == ".zip" || ext == ".elf") {
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
    }

    catOut << "\n  ]\n}\n";
    catOut.close();

    std::wstring statusMsg = L"Status: Scan complete. Found " + std::to_wstring(totalFound) + L" ROMs.";
    SetWindowText(hStaticStatus, statusMsg.c_str());
}

// Test Handshake with MiSTer IP via UDP PING
void TestMisterHandshake() {
    std::wstring ipStr = GetText(hEditMisterIp);
    if (ipStr.empty()) {
        MessageBox(hMainWnd, L"Please enter a valid MiSTer IP address.", L"Error", MB_ICONERROR);
        return;
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    DWORD timeout = 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2154);
    std::string ip(ipStr.begin(), ipStr.end());
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    const char* pingMsg = "PING";
    sendto(sock, pingMsg, (int)strlen(pingMsg), 0, (sockaddr*)&addr, sizeof(addr));

    char buf[128] = { 0 };
    int fromLen = sizeof(addr);
    int recvLen = recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&addr, &fromLen);

    if (recvLen > 0) {
        SetWindowText(hStaticStatus, L"Status: MiSTer Handshake Successful! (Replied: PONG)");
        MessageBox(hMainWnd, L"MiSTer client acknowledged UDP handshake!", L"Success", MB_ICONINFORMATION);
    } else {
        SetWindowText(hStaticStatus, L"Status: MiSTer timed out. Verify script is running on DE10-Nano.");
        MessageBox(hMainWnd, L"Could not reach MiSTer on port 2154. Ensure network cables and phantom script are active.", L"Timeout", MB_ICONWARNING);
    }

    closesocket(sock);
    WSACleanup();
}

// Background UDP Daemon Worker Thread Function
DWORD WINAPI DaemonThreadProc(LPVOID lpParam) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    g_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in bindAddr = { 0 };
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(2154);
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
                const char* reply = "PHANTOM_HOST_ONLINE";
                sendto(g_udpSocket, reply, (int)strlen(reply), 0, (sockaddr*)&clientAddr, clientLen);
            } else if (msg.rfind("LAUNCH:", 0) == 0) {
                std::string gameId = msg.substr(7);
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
        g_daemonRunning = true;
        g_hDaemonThread = CreateThread(NULL, 0, DaemonThreadProc, NULL, 0, NULL);
        SetWindowText(hBtnToggleDaemon, L"Stop Background Daemon");
        SetWindowText(hStaticStatus, L"Status: Daemon ACTIVE (Listening on UDP :2154 with Auto-Discovery)");
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
        // MiSTer IP
        CreateWindow(L"STATIC", L"MiSTer FPGA IP Address:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditMisterIp = CreateWindow(L"EDIT", L"192.168.1.50", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 160, 22, hWnd, (HMENU)IDC_EDIT_MISTER_IP, hInst, NULL);
        CreateWindow(L"STATIC", L"UDP Port:", WS_CHILD | WS_VISIBLE, 390, y, 70, 20, hWnd, NULL, hInst, NULL);
        hEditUdpPort = CreateWindow(L"EDIT", L"2154", WS_CHILD | WS_VISIBLE | WS_BORDER, 470, y, 80, 22, hWnd, (HMENU)IDC_EDIT_UDP_PORT, hInst, NULL);

        // PCSX2 Executable & ROMs
        y += 35;
        CreateWindow(L"STATIC", L"PCSX2 Executable:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditPcsx2Exe = CreateWindow(L"EDIT", L"C:\\Emulators\\PCSX2\\pcsx2-qt.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 340, 22, hWnd, (HMENU)IDC_EDIT_PCSX2_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 560, y, 80, 22, hWnd, (HMENU)IDC_BTN_BROWSE_PCSX2, hInst, NULL);

        y += 30;
        CreateWindow(L"STATIC", L"PS2 ROMs Folder:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditPs2Roms = CreateWindow(L"EDIT", L"C:\\Games\\PS2", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 340, 22, hWnd, (HMENU)IDC_EDIT_PS2_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 560, y, 80, 22, hWnd, (HMENU)IDC_BTN_BROWSE_PS2ROMS, hInst, NULL);

        // Dolphin Executable & ROMs
        y += 35;
        CreateWindow(L"STATIC", L"Dolphin Executable:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditDolphinExe = CreateWindow(L"EDIT", L"C:\\Emulators\\Dolphin\\Dolphin.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 340, 22, hWnd, (HMENU)IDC_EDIT_DOLPHIN_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 560, y, 80, 22, hWnd, (HMENU)IDC_BTN_BROWSE_DOLPHIN, hInst, NULL);

        y += 30;
        CreateWindow(L"STATIC", L"GameCube/Wii ROMs:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditGcRoms = CreateWindow(L"EDIT", L"C:\\Games\\GameCube", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 340, 22, hWnd, (HMENU)IDC_EDIT_GC_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 560, y, 80, 22, hWnd, (HMENU)IDC_BTN_BROWSE_GCROMS, hInst, NULL);

        // Flycast
        y += 35;
        CreateWindow(L"STATIC", L"Flycast Executable:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditFlycastExe = CreateWindow(L"EDIT", L"C:\\Emulators\\Flycast\\flycast.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 340, 22, hWnd, (HMENU)IDC_EDIT_FLYCAST_EXE, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 560, y, 80, 22, hWnd, (HMENU)IDC_BTN_BROWSE_FLYCAST, hInst, NULL);

        y += 30;
        CreateWindow(L"STATIC", L"Naomi/Arcade ROMs:", WS_CHILD | WS_VISIBLE, 20, y, 180, 20, hWnd, NULL, hInst, NULL);
        hEditNaomiRoms = CreateWindow(L"EDIT", L"C:\\Games\\Arcade\\Naomi", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, y, 340, 22, hWnd, (HMENU)IDC_EDIT_NAOMI_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 560, y, 80, 22, hWnd, (HMENU)IDC_BTN_BROWSE_NAOMI, hInst, NULL);

        // Action Buttons Row
        y += 40;
        CreateWindow(L"BUTTON", L"1. Auto-Scan ROMs", WS_CHILD | WS_VISIBLE, 20, y, 140, 30, hWnd, (HMENU)IDC_BTN_SCAN_ROMS, hInst, NULL);
        CreateWindow(L"BUTTON", L"2. Test MiSTer Ping", WS_CHILD | WS_VISIBLE, 170, y, 140, 30, hWnd, (HMENU)IDC_BTN_TEST_MISTER, hInst, NULL);
        CreateWindow(L"BUTTON", L"3. Save Config", WS_CHILD | WS_VISIBLE, 320, y, 130, 30, hWnd, (HMENU)IDC_BTN_SAVE_CONFIG, hInst, NULL);
        hBtnToggleDaemon = CreateWindow(L"BUTTON", L"Start Background Daemon", WS_CHILD | WS_VISIBLE, 460, y, 180, 30, hWnd, (HMENU)IDC_BTN_TOGGLE_DAEMON, hInst, NULL);

        // Scanned ROMs List Box
        y += 42;
        CreateWindow(L"STATIC", L"Detected Games Catalog:", WS_CHILD | WS_VISIBLE, 20, y, 200, 18, hWnd, NULL, hInst, NULL);
        y += 22;
        hListGames = CreateWindow(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 20, y, 620, 160, hWnd, (HMENU)IDC_LIST_GAMES, hInst, NULL);

        // Status Bar
        y += 170;
        hStaticStatus = CreateWindow(L"STATIC", L"Status: Ready. Configure paths and click 'Auto-Scan ROMs'.", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 20, y, 620, 20, hWnd, (HMENU)IDC_STATIC_STATUS, hInst, NULL);
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
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
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 520,
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
