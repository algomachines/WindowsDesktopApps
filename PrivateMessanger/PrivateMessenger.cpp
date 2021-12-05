// Copyright (c) AlgoMachines
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// PrivateMessanger.cpp : Defines the entry point for the application.
//
// Tasks:
// Rename a conversation - done 2021/11/27
// Delete a conversation - done 2021/11/27
// Need word wrap for messages longer than a single line - got rid of auto window width, good enough for now - done 2021/11/27
// Scroll through a conversation Up/Down/Top/Bottom - done 2021/11/27
// Indicator when new messages have not be read - done 2021/11/27
// Ability to turn on/off "Reveal new messages as they arrive" - done 2021/11/27
// Search a conversation - nice to have but, have copy to clipboard, don't need this now - 2021/11/27
// Exe for running protected code? - No, need to open source the whole client app, need eyes looking for bugs / security holes - 2021/11/27
// Check pending messages slows down, 5s,10s,20s,30s,1m,2m,5m - resets on mouse over, if messages are received or sent - done 2021/11/27 
//
// semaphore locking for CGI component - done 2021/11/28
// improved random number generate for the CGI component - done 2021/11/28
//
// DONE 2021/11/28
// robust confirmation of hashed_instance  - Involves a small modification to the RetrievePendingMessages() return buffer processing and several changes to the CGI component
//  1) Server increments the sequence number when receiveing a messge from the client and returns the NEW instance_hash to the client
//  2) For some reason the client fails to update the intance_hash, server has no way of knowing that this has happened at this point.
//  3) Client sends request to RetrievePendingMessages() using the OLD instance_hash.
//  4) Server recognizes the OLD instance_hash and returns NO messages and a copy of the NEW instance_hash
//  5) Client recognizes that the server is returning a copy of a NEW instance_hash and therefore updates the instance_hash in the config.bin file, restoration is complete
// 
// Ability to send multiple messages, all of which will be seen by the receipient - done 2021/11/30
// Word wrap when rendering messages - done 2021/12/01
// Send / edit box fixed at the bottom of the client window - done 2021/12/02
// <Enter> in the IDC_Message edit control sends the message, <Shift> <Enter> creates a line break in the message - done 2021/12/02
// Draw border around the IDD_SendMessage dialog interior - done 2021/12/02
// Pending messages indicator - for conversations which are not open - done 2021/12/02
// Fix bug - double entry of sent text when sending empty lines - done 2021/12/02
// 
// Restrict the client to a single instance per InstallID - done 2021/12/03
// No prompt for app password when bringing in a new conversation - default password the app password - done 2021/12/03
// Ability to change app password - done 2021/12/03
// Conversation password always matches app password - done 2021/12/03
// Ability to change app url, conv URL always matchs and is also updated - done 2021/12/03
// 
// Pending tasks
// 
// CGI component send database stores only the minimum amount to message text
// CGI component - messages left unretrieved on the server deleted after 1 week if there are space constraints
// CGI component databases are encrypted
// 
// Linux build for the compiler
// Linux build for the CGI component
//
#include "OS.h"

#include "resource.h"
#include "PrivateMessanger.h"
#include "TextMessageAPI.h"
#include "DRM_ProgramRecord.h"
#include "SendDataToServer.h"
#include "PrivateMessengerIndex.hpp"
#include "Clipboard.h"
#include "dwmapi.h"

static STRING s_data_dir;

// Exclude the compress_file() function in protected because it is not needed here and 
// forces the inclusion of unwanted dependencies
#define EXCLUDE_compress_file

#include "Compiler\CompiledCodeCommon.h"

#include "Compiler\Info.hpp"
#include "Compiler\RunCompiledCode.hpp"

#include "Compiler\Common.hpp"

#include "Compiler\AssignExternalFunctionAddresses.hpp"

static const char *s_component = "cgi-bin/PrivateMessenger.exe";

using namespace TextMessageAPI;

vector<uint64_t> g_db_handles; // handles to open text message databases

static STRING s_e_url_ownership_status;

static SimpleDB<PrivateMessangerIndexElement> s_index_db;
static STRING s_index_db_file;

static string s_clipboard_txt;

static STRING s_pending_messages_status;
static HFONT  s_pending_messages_font;
static int    s_pending_message_status_font_ht = 0;
static HPEN   white_pen = 0;
static HBRUSH white_brush = 0;

static vector<uint8_t> s_app_password_hash;

static char s_URL[260];             // 256 char max


#include "ClientSource.h"

#define MAX_LOADSTRING 100
#define CLIENT_WIDTH_PIXELS 400


bool AddMeToTheServer(const char* url, string& err_msg);

//bool RetrieveMessage(HWND, string &err_msg);
bool RetrievePendingMessages(int &nmessages_retrieved, string& err_msg);
bool SendTextMessageToServer(const uint8_t* my_id, const uint8_t* NC_other_id, const char* msg, string& err_msg);

bool GetAppPassword(HWND hWnd);

bool compute_modified_guid(const GUID& guid, GUID& modified_guid, string& err_msg);

// remote_hashed_id - hash of id of the remote client for this conversation - 32 bytes
// pwd_hash - hash of password for the new conversation - 32 bytes
// nickname - optional name to assign to this remote client / conversation
// msg_to_send - optional message to send to the remote client
bool InitiateNewConversation(const uint8_t* remote_hashed_id, const uint8_t* pwd_hash, string& err_msg, const char* nickname = 0, const char* msg_to_send = 0);

HINSTANCE instance_handle = 0;
HWND main_hWnd = 0;
HWND send_text_message_hWnd = 0;
HWND send_message_edit_control_hWnd = 0;

#define TIMER_ID_CheckMessages 1000
#define TIMER_ID_MenuStatusUpdate 1001

//5s,10s,20s,30s,1m,2m,5m - check messages this often when there is no activity
static UINT g_check_messages_interval[7] = { 5000, 10000, 20000, 30000, 60000, 120000, 300000 };
static int g_check_messages_interval_idx = 0;

// Called when there is activity - without activity, we eventually get to the 5 minute message check interval
inline void ResetCheckMessagesSequence(void)
{
    if (g_check_messages_interval_idx > 0)
    {
        g_check_messages_interval_idx = 0;
        SetTimer(main_hWnd, TIMER_ID_CheckMessages, g_check_messages_interval[g_check_messages_interval_idx], NULL);
    }
}

inline UINT GetMessageCheckInterval(void)
{
    UINT ms = g_check_messages_interval[g_check_messages_interval_idx];

    if (g_check_messages_interval_idx < sizeof(g_check_messages_interval) / sizeof(UINT))
        g_check_messages_interval_idx++;

    return ms;
}



// Global Variables:
HINSTANCE hInst;                                // current instance
CHAR szTitle[MAX_LOADSTRING];                  // The title bar text
CHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    NewConversation(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SelectConversation(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    URLDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    SendTextMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    ChangePassword(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    ChangeURL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// PreTranslateMessageHandlers

inline bool SendMesageOnEnter_Handler(const MSG& msg)
{
    if (msg.hwnd == send_message_edit_control_hWnd)
    {
        if (msg.message == WM_CHAR)
        {
            if (msg.wParam == VK_RETURN) // Enter key
            {
                uint16_t state = GetKeyState(VK_SHIFT);
                bool bShiftIsDown = state & 0x8000;
                if (bShiftIsDown == false)
                {
                    SendMessage(send_text_message_hWnd, WM_COMMAND, (WPARAM)IDC_Send, NULL);
                    return true;
                }
            }

        }
    }

    return false;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    instance_handle = hInstance;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    char exe_file_name[MAX_PATH + 1];
    GetModuleFileName(NULL, exe_file_name, MAX_PATH);


    STRING dir;
    dir.LoadFileName(exe_file_name, TRUE, FALSE, FALSE);

    dir.AppendSubDir("MSG_DB");
    if (DoesFileExist(dir) == false)
    {
        if (_mkdir(dir))
        {
            char msg[1024];
            sprintf(msg, "Unable to create directory: %s", (const char *)dir);
            MessageBox(NULL, msg, "Error", MB_OK);
            return FALSE;
        }
    }

    s_data_dir = dir;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_PRIVATEMESSANGER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (GetAppPassword(main_hWnd) == false)
        return FALSE;

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    // We now have an application password, the hash is loaded in memory
    string err_msg;
    if (CreatePasswordProtectedConfigFileIfNecessary(s_app_password_hash, err_msg) == false)
    {
        if (err_msg.length())
        {
            MessageBox(main_hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
            return FALSE;
        }
    }

    barray hashed_install_id;
    if (GetHashedInstallID(hashed_install_id, s_app_password_hash, err_msg) == false)
    {
        MessageBox(main_hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
        return FALSE;
    }

    string id_name;
    bin_to_ascii_char(hashed_install_id, id_name);

    HANDLE hMutex = CreateMutex(NULL, TRUE, id_name.c_str());

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        string err_msg;
        err_msg = "Only one instance with ID:\n\n";
        err_msg += id_name.c_str();
        err_msg += "\n\nmay be open at a time.";
        MessageBox(main_hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
        return FALSE;
    }

    string url;
    if (GetURLFromConfigFile(url, s_app_password_hash, err_msg) == false)
    {
        MessageBox(main_hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
        return FALSE;
    }

    if (url.length() == 0)
    {
        // Need to get the server URL
        if (DialogBox(hInst, MAKEINTRESOURCE(IDD_URL), main_hWnd, URLDlg) != IDOK)
        {
            MessageBox(main_hWnd, "Need the URL of the remote server in order to continue.", "Error", MB_ICONERROR);
            return FALSE;
        }

        if (AddMeToTheServer(s_URL, err_msg) == false)
        {
            MessageBox(main_hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
            return FALSE;
        }

        if (SaveRemoteServerURLToConfigFile(s_URL, s_app_password_hash, err_msg) == false)
        {
            ZERO(s_URL);
            MessageBox(main_hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
            return FALSE;
        }

        ZERO(s_URL);
    }

    Zero(url);

    s_index_db_file.SetValidFileName(s_data_dir, "index.bin");

    if (DoesFileExist(s_index_db_file))
    {
        if (s_index_db.LoadFromFile(s_index_db_file, err_msg) == false)
        {
            char msg[1024];
            sprintf(msg, "Unable to load index: %s", err_msg.c_str());
            MessageBox(NULL, msg, "Error", MB_OK);
        }
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PRIVATEMESSANGER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            if (SendMesageOnEnter_Handler(msg) == true)
                continue; // message handled

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

INT_PTR CALLBACK URLDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            HWND hwnd = GetDlgItem(hDlg, IDC_URL);
            GetWindowText(hwnd, s_URL, sizeof(s_URL));

            if (s_URL[sizeof(s_URL) - 1])
                s_URL[sizeof(s_URL) - 1] = 0; // must be null terminated

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
    }
    return (INT_PTR)FALSE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PRIVATEMESSANGER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_PRIVATEMESSANGER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    main_hWnd = hWnd;

    ///////////////////////////////////////////////////
    // Establish the initial window width
    RECT rect;
    GetClientRect(hWnd, &rect);

    int w = rect.right - rect.left;

    int delta = w - CLIENT_WIDTH_PIXELS;

    if (delta)
    {
        WINDOWPLACEMENT wp;
        GetWindowPlacement(hWnd, &wp);

        wp.rcNormalPosition.right -= delta;
        SetWindowPlacement(hWnd, &wp);
    }
    ///////////////////////////////////////////////////

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    string err_msg;
    TextMessageAPI::Initialize(err_msg);

    s_pending_messages_font = TextMessageAPI::GetTimestampFont();

    white_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    white_brush = CreateSolidBrush(RGB(255, 255, 255));

    int nmessages_retrieved = 0;
    bool status = RetrievePendingMessages(nmessages_retrieved, err_msg);

    SetTimer(hWnd, TIMER_ID_MenuStatusUpdate, 1000, NULL);

    return TRUE;
}


static uint32_t g_conversation_top_index = 0xFFFFFFFF;
static uint32_t g_conversation_bottom_index = 0xFFFFFFFF;
static bool g_unread_messages_appended = false;

inline bool GetPendingMessagesStatus(STRING& status)
{
    status = "";

    STRING fspec;
    fspec = s_data_dir;
    fspec.AppendSubDir("pending");
    fspec += "\\*";

    WIN32_FIND_DATA data;
    
    HANDLE h = FindFirstFile(fspec, &data);

    if (h == INVALID_HANDLE_VALUE)
        return false;

    int n = 0;

    while (1)
    {
        if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            if (FindNextFile(h, &data) == FALSE)
                break;

            continue;
        }

        barray b;
        ascii_char_to_bin(data.cFileName, b);

        if (b.size() == 32)
        {
            barray open_conversation_id;
            string err_msg;
            TextMessageAPI::GetHashedRemoteID(0, open_conversation_id, err_msg);

            if (open_conversation_id.size() == 32 && memcmp(&b[0],&open_conversation_id[0], 32)) // only update the msg if there are pending conversations for a conversation which is not open
                n++;
        }

        if (FindNextFile(h, &data) == FALSE)
            break;
    }

    FindClose(h);

    if (n)
    {
        status = "Pending msgs for ";
        status += (UINT)n;
        status += n == 1 ? " conversation" : " conversations";
        return true;
    }

    return false;
}

inline bool LoadCachedPendingMessages(HWND hWnd, string& err_msg)
{
    vector<uint8_t> hashed_id;

    if (TextMessageAPI::GetHashedRemoteID(0, hashed_id, err_msg) == false)
    {
        err_msg = "";
        return true;
    }

    string fname;
    bin_to_ascii_char(&hashed_id[0], hashed_id.size(), fname);

    STRING file_name;
    file_name = s_data_dir;
    file_name.AppendSubDir("pending");
    file_name.AppendSubDir(fname.c_str());

    if (DoesFileExist(file_name) == false)
        return true;

    err_msg = "";
    if (TextMessageAPI::AppendFromFile(0, file_name, s_app_password_hash, err_msg) == false)
    {
        if (err_msg.length() == 0)
        {
            DeleteFileA(file_name);
            return true;
        }

        return false;
    }

    DeleteFileA(file_name);

    if (TextMessageAPI::Save(0, 0, err_msg) == false)
        return false;

    // Restart the sequence of intervals for checking messages if a message arrives
    ResetCheckMessagesSequence();

    g_unread_messages_appended = true;

    BOOL bscroll_to_bottom = GetMenuState(GetMenu(hWnd), ID_RevealNewMesages, MF_BYCOMMAND) & MF_CHECKED;

    if (bscroll_to_bottom)
        g_conversation_top_index = 0xFFFFFFFF;

    InvalidateRect(hWnd, NULL, TRUE);

    return true;
}


bool OnPageDownInConversation(HWND hWnd)
{
    if (TextMessageAPI::IsOpen() == false)
        return false;

    if (g_conversation_top_index == 0xFFFFFFFF)
        return false;

    uint32_t n;
    TextMessageAPI::GetNumMessages(0, n);

    if (g_conversation_bottom_index >= n)
        return false;

    g_conversation_top_index++;

    InvalidateRect(hWnd, 0, TRUE);

    return true;
}

bool OnPageUpInConversation(HWND hWnd)
{
    if (TextMessageAPI::IsOpen() == false)
        return false;

    if (g_conversation_top_index == 0xFFFFFFFF)
        return false;

    if (g_conversation_top_index == 0)
        return false;

    g_conversation_top_index--;

    InvalidateRect(hWnd, 0, TRUE);

    return true;
}

bool OnGotoEndOfConversation(HWND hWnd)
{
    if (TextMessageAPI::IsOpen() == false)
        return false;

    if (g_conversation_top_index == 0xFFFFFFFF)
        return false;

    g_conversation_top_index = 0xFFFFFFFF;

    InvalidateRect(hWnd, 0, TRUE);

    return true;
}

bool OnGotoTopOfConversation(HWND hWnd)
{
    if (TextMessageAPI::IsOpen() == false)
        return false;

    if (TextMessageAPI::IsOpen() == false)
        return false;

    if (g_conversation_top_index == 0xFFFFFFFF)
        return false;

    g_conversation_top_index = 0;

    InvalidateRect(hWnd, 0, TRUE);

    return true;
}

bool OnFindInConversation(HWND hWnd)
{
    if (TextMessageAPI::IsOpen() == false)
        return false;

    return true;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        CreateDialogA(hInst, MAKEINTRESOURCE(IDD_SendMessage), hWnd, SendTextMessage);

        break;
    }

    case WM_MOVE:
    case WM_SIZE:
    {
        if (send_text_message_hWnd)
        {
            WINDOWPLACEMENT wp_send;
            RECT rect, client_rect;

            GetWindowRect(hWnd, &rect);
            GetClientRect(hWnd, &client_rect);

            int dx = width(rect) - width(client_rect);

            GetWindowPlacement(send_text_message_hWnd, &wp_send);

            wp_send.rcNormalPosition.left = rect.left + dx / 2;
            wp_send.rcNormalPosition.right = rect.right - dx / 2;

            int ht = height(wp_send.rcNormalPosition);

            wp_send.rcNormalPosition.top = rect.bottom + 1 - dx / 2;
            wp_send.rcNormalPosition.bottom = wp_send.rcNormalPosition.top + ht;

            if (TextMessageAPI::IsOpen())
                wp_send.showCmd = TRUE;
            else
                wp_send.showCmd = FALSE;

            SetWindowPlacement(send_text_message_hWnd, &wp_send);
        }

        break;
    }

    case WM_COMMAND:
        {
            ResetCheckMessagesSequence();

            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            case ID_ChangePassword:
            {
                KillTimer(hWnd, TIMER_ID_CheckMessages);

                DialogBox(hInst, MAKEINTRESOURCE(IDD_ChangePassword), hWnd, ChangePassword);

                SetTimer(hWnd, TIMER_ID_CheckMessages, 0, 0);
                break;
            }

            case ID_ChangeURL:
            {
                KillTimer(hWnd, TIMER_ID_CheckMessages);

                DialogBox(hInst, MAKEINTRESOURCE(IDD_ChangeURL), hWnd, ChangeURL);

                SetTimer(hWnd, TIMER_ID_CheckMessages, 0, 0);
                break;
            }

            case ID_NewConversation:

                KillTimer(hWnd, TIMER_ID_CheckMessages);
                DialogBox(hInst, MAKEINTRESOURCE(IDD_NewConversation), hWnd, NewConversation);

                InvalidateRect(hWnd, 0, TRUE);

                break;

            case ID_SelectConversation:

                KillTimer(hWnd, TIMER_ID_CheckMessages);
                if (DialogBox(hInst, MAKEINTRESOURCE(IDD_SelectConversation), hWnd, SelectConversation) == IDOK)
                {
                    g_unread_messages_appended = false;
                    g_conversation_top_index = 0xFFFFFFFF; // start at the bottom
                    ShowWindow(send_text_message_hWnd, SW_SHOW);
                }

                InvalidateRect(hWnd, 0, TRUE);

                break;

            case ID_CopyHistory:

                if (TextMessageAPI::IsOpen() == false)
                    break;

                if (MessageBox(hWnd, "Are you sure that you want to copy the history for this conversation to the Windows clipboard?", "Confirmation", MB_YESNO) != IDYES)
                    break;

                {
                    string err_msg;
                    if (TextMessageAPI::CopyHistory(0, main_hWnd, err_msg) == false)
                    {
                        MessageBox(hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
                        break;
                    }
                }

                break;

            case ID_RevealNewMesages: // change the check state
            {
                if (GetMenuState(GetMenu(hWnd), ID_RevealNewMesages, MF_BYCOMMAND) & MF_CHECKED)
                {
                    // need to uncheck
                    CheckMenuItem(GetMenu(hWnd), ID_RevealNewMesages, MF_BYCOMMAND|MF_UNCHECKED);
                }
                else
                {
                    // need to check
                    CheckMenuItem(GetMenu(hWnd), ID_RevealNewMesages, MF_BYCOMMAND|MF_CHECKED);
                }

                break;
            }

            case ID_PageDown: OnPageDownInConversation(hWnd); break;
            case ID_PageUp: OnPageUpInConversation(hWnd); break;
            case ID_End: OnGotoEndOfConversation(hWnd); break;
            case ID_Home: OnGotoTopOfConversation(hWnd); break;
            case ID_Find: OnFindInConversation(hWnd); break;

            case ID_Refresh:
            {
                SetTimer(hWnd, TIMER_ID_CheckMessages, 0, 0);
                break;
            }

            case ID_ClearHistory:

                if (TextMessageAPI::IsOpen() == false)
                    break;

                if (MessageBox(hWnd, "Are you sure that you want to clear the history for this conversation?", "Confirmation", MB_YESNO) != IDYES)
                    break;

                {
                    string err_msg;
                    if (TextMessageAPI::ClearHistory(0, err_msg) == false)
                    {
                        MessageBox(hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
                        break;
                    }

                    STRING file_name = s_data_dir;
                    vector<uint8_t> hashed_remote_id;
                    if (TextMessageAPI::GetHashedRemoteID(0, hashed_remote_id, err_msg) == false)
                    {
                        MessageBox(hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
                        break;
                    }

                    string fname;
                    bin_to_ascii_char(&hashed_remote_id[0], hashed_remote_id.size(), fname);

                    file_name.AppendSubDir(fname.c_str());

                    if (TextMessageAPI::Save(0, 0, err_msg, file_name) == false)
                    {
                        MessageBox(hWnd, err_msg.c_str(), "Error", MB_ICONERROR);
                        break;
                    }

                    InvalidateRect(hWnd, NULL, TRUE);
                }

                break;
                

            case ID_CopyToClipboard:
            
                CopyStringToClipboard(s_clipboard_txt.c_str(), hWnd);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_MOUSEMOVE:
        ResetCheckMessagesSequence();
        break;

    case WM_TIMER:
    {
        if (wParam == TIMER_ID_CheckMessages)
        {
            KillTimer(hWnd, TIMER_ID_CheckMessages);

            string err_msg;
            int nmessages_retrieved = 0;
            bool status = RetrievePendingMessages(nmessages_retrieved, err_msg);
           
            if (status == false)
            {
                //MessageBox(hWnd, "Failed to connect to server", "Error", MB_ICONERROR);
                break;
            }

            LoadCachedPendingMessages(hWnd, err_msg);
            
            SetTimer(hWnd, TIMER_ID_CheckMessages, GetMessageCheckInterval(), NULL);
           
            break;
        }

        if (wParam == TIMER_ID_MenuStatusUpdate)
        {
            bool bOpenConversation = TextMessageAPI::IsOpen();

            EnableMenuItem(GetMenu(hWnd), ID_ClearHistory, bOpenConversation ? MF_ENABLED : MF_DISABLED);
            EnableMenuItem(GetMenu(hWnd), ID_CopyHistory, bOpenConversation ? MF_ENABLED : MF_DISABLED);
            EnableMenuItem(GetMenu(hWnd), ID_PageUp, bOpenConversation ? MF_ENABLED : MF_DISABLED);
            EnableMenuItem(GetMenu(hWnd), ID_End, bOpenConversation ? MF_ENABLED : MF_DISABLED);
            EnableMenuItem(GetMenu(hWnd), ID_Home, bOpenConversation ? MF_ENABLED : MF_DISABLED);

            break;
        }
    }

    case WM_RBUTTONDOWN:
    {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);

        string msg;
        if (TextMessageAPI::GetMessageTextAtPoint(0, pt, msg))
        {
            HMENU hmenu = CreatePopupMenu();

            STRING s;
            s = "Copy \"";
            s += msg.c_str();
            s += "\"";

            s_clipboard_txt = msg;

            InsertMenu(hmenu, -1, MF_BYPOSITION, ID_CopyToClipboard, s);
            SetMenu(hWnd, hmenu);

            ClientToScreen(hWnd, &pt);
            TrackPopupMenu(hmenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
        }

        break;
    }

    case WM_PAINT:
        {
            string tmp;
            const char* nickname = TextMessageAPI::GetNickname(0,tmp);

            if (nickname)
            {
                STRING s;
                s = "Talking to \"";
                s += nickname;
                s += "\"";
                if (SetWindowText(hWnd, s) == FALSE)
                {
                    DWORD err = GetLastError();
                    bool break_here = true;
                }
            }

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);


            uint64_t h = 0; // Use the first handle
            RECT rect;
            GetClientRect(hWnd, &rect);

            // Draw the pending_messages_status at the top of the client area
            {
                SelectObject(hdc, s_pending_messages_font);
                SetTextColor(hdc, RGB(255,0,0));
                SetBkMode(hdc, OPAQUE);
                SelectObject(hdc, white_brush);
                SelectObject(hdc, white_pen);

                SIZE sz;
                if (s_pending_message_status_font_ht == 0) // Determin the size of the top / pending message area and initialize the variable which holds this information
                {
                    GetTextExtentPoint(hdc, "W", 1, &sz);
                    s_pending_message_status_font_ht = sz.cy;
                }
                
                RECT pending_messages_rect = rect;
                pending_messages_rect.bottom = s_pending_message_status_font_ht;

                // Clear the background of the pending message area
                Rectangle(hdc, pending_messages_rect.left, pending_messages_rect.top, pending_messages_rect.right, pending_messages_rect.bottom);

                int len = StrLen(s_pending_messages_status);
                if (len) DrawText(hdc, s_pending_messages_status, len, &pending_messages_rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            }

            string err_msg;

            // Render the text message thread starting at the specified message index
            // h - handle to the thread - may be zero
            // idx - index at the bottom on pass in, 0xFFFFFFFF for the bottom index at the bottom, on return the value is the index of the message rendered at the top
            // wnd_rect - client window area
            // hdc - device context handle
            if (g_conversation_top_index == 0xFFFFFFFF)
            {
                uint32_t index_bottom;
                TextMessageAPI::GetNumMessages(0, index_bottom);
                index_bottom--;
                uint32_t index_top = 0;
                TextMessageAPI::RenderByIndexAtBottom(0, index_top, index_bottom, rect, hdc, err_msg);
                g_conversation_top_index = index_top;
                g_conversation_bottom_index = index_bottom;
            }
            else
            {
                uint32_t index_bottom = 0;
                TextMessageAPI::RenderByIndexAtTop(0, g_conversation_top_index, index_bottom, rect, hdc, g_unread_messages_appended, err_msg);
                g_conversation_bottom_index = index_bottom;
            }

            // Draw a black line at the bottom of the client area
            HPEN hpen = CreatePen(PS_SOLID, 1, RGB(1, 1, 1));
            SelectObject(hdc, hpen);

            MoveToEx(hdc, rect.left, rect.bottom - 1, 0);
            LineTo(hdc, rect.right - 1, rect.bottom - 1);

            DeleteObject(hpen);

            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        char line1[256];
        sprintf_s(line1, sizeof(line1), "PrivateMessanger Build Timestamp: %s %s", __DATE__, __TIME__);

        SetWindowText(GetDlgItem(hDlg, IDC_Line1), line1);

        string s,err_msg;
        GetHashedInstallID_ascii(s, s_app_password_hash, err_msg);

        SetWindowText(GetDlgItem(hDlg, IDC_MyHashedID), s.c_str());

        return (INT_PTR)TRUE;
    }
    

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


INT_PTR CALLBACK SendTextMessage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        send_text_message_hWnd = hDlg;
        send_message_edit_control_hWnd = GetDlgItem(hDlg, IDC_Message);

        return (INT_PTR)TRUE;
    }

    case WM_SHOWWINDOW:
    {
        short break_here = true;
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        RECT rect;
        GetClientRect(hDlg, &rect);
        HPEN hpen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        SelectObject(hdc, hpen);
        //MoveToEx(hdc, rect.left, rect.top, 0);
        MoveToEx(hdc, rect.right - 1, rect.top,0);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, rect.left, rect.bottom - 1);
        LineTo(hdc, rect.left, rect.top);

        DeleteObject(hpen);

        DeleteObject(hpen);

        EndPaint(hDlg, &ps);

        break;
    }

    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hDlg, &rect);
        HWND hwnd_send = GetDlgItem(hDlg, IDC_Send);
        HWND hwnd_message = GetDlgItem(hDlg, IDC_Message);

        RECT send_rect, message_rect, client_rect;

        GetClientRect(hwnd_send, &send_rect);
        GetClientRect(hwnd_message, &message_rect);
        GetClientRect(hDlg, &client_rect);

        int w = width(send_rect) + width(message_rect);
        int w_cl = width(client_rect);

        //if (w == w_cl)
        //    break;

        int dw = w - w_cl;
        dw += 3;

        WINDOWPLACEMENT wp;
        GetWindowPlacement(hwnd_message, &wp);

        // Adjust the width of the IDC_message edit control
        wp.rcNormalPosition.right -= dw;

        SetWindowPlacement(hwnd_message, &wp);

        break;
    }

    case WM_COMMAND:

        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            // Do nothing
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_Send)
        {
            HWND hwnd = GetDlgItem(hDlg, IDC_Message);
            char msg[257];
            ZERO(msg);
            GetWindowText(hwnd, msg, sizeof(msg)-1);

            if (msg[0] == 0)
            {
                return (INT_PTR)TRUE;
            }

            if (TextMessageAPI::IsOpen() == false)
            {
                ShowWindow(hDlg, SW_HIDE);
                SetWindowText(hwnd, "");
                return (INT_PTR)TRUE;
            }

            string URL, err_msg;
            if (TextMessageAPI::GetURL(0, URL, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            vector<uint8_t> hashed_id;
            if (GetHashedInstallID(hashed_id, s_app_password_hash, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            vector<uint8_t> hashed_remote_id;
            if (TextMessageAPI::GetHashedRemoteID(0, hashed_remote_id, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            // Before sending a message retreive any pending messages
            int nmessages_retrieved;
            if (RetrievePendingMessages(nmessages_retrieved, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            // Append any retreived messages to the text message database
            if (LoadCachedPendingMessages(hDlg, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            if (SendTextMessageToServer(&hashed_id[0], &hashed_remote_id[0], msg, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            bool sender = true;
            if (TextMessageAPI::Append(0, sender, msg, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            else
            {
                SetWindowText(hwnd, "");
                InvalidateRect(main_hWnd, NULL, TRUE);
            }

            if (TextMessageAPI::Save(0, 0, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            BOOL bscroll_to_bottom = GetMenuState(GetMenu(main_hWnd), ID_RevealNewMesages, MF_BYCOMMAND) & MF_CHECKED;

            if (bscroll_to_bottom)
                g_conversation_top_index = 0xFFFFFFFF;

            InvalidateRect(main_hWnd, NULL, TRUE);

            ResetCheckMessagesSequence(); // likely to receive a text message after sending one

            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

vector<uint8_t> s_pwd_hash;

INT_PTR CALLBACK PasswordDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char pwd[34];
            ZERO(pwd);
            GetWindowText(GetDlgItem(hDlg, IDC_Password), pwd, sizeof(pwd));

            int len = StrLen(pwd);
            if (len > 32 || len < 4)
            {
                MessageBox(hDlg, "Password may not be more than 32 characters nor less than 4 characters.", "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            hash_id((uint8_t *)pwd);
            hash_id((uint8_t*)pwd);

            s_pwd_hash.resize(32);
            memmove(&s_pwd_hash[0], pwd, 32);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)FALSE;
        }

        break;
    }


    return 0;
}

inline bool GetPassword(HWND hWnd, vector<uint8_t>& pwd_hash)
{
    if (DialogBox(hInst, MAKEINTRESOURCE(IDD_Password), hWnd, PasswordDlg) != IDOK)
        return false;

    pwd_hash.resize(32);
    memmove(&pwd_hash[0], &s_pwd_hash[0], 32);

    s_pwd_hash.resize(0);

    return true;
}

static string g_selected_conversation_nickname;

INT_PTR CALLBACK RenameConversationNicknameDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        SetWindowText(GetDlgItem(hDlg, IDC_OldName), g_selected_conversation_nickname.c_str());
 
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            char s[33];
            ZERO(s);
            GetWindowText(GetDlgItem(hDlg, IDC_NewName), s, sizeof(s) - 1);
            g_selected_conversation_nickname = s;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        // Limit the nickname to 32 characters
        if (LOWORD(wParam) == IDC_NewName)
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                char s[128];
                ZERO(s);
                GetWindowText(GetDlgItem(hDlg, IDC_NewName), s, sizeof(s) - 1);
                if (StrLen(s) > 32)
                {
                    s[32] = 0;
                    SetWindowText(GetDlgItem(hDlg, IDC_NewName), s);
                }
            }

            break;
        }

        break;
    }

    }

    return (INT_PTR)FALSE;
}

bool GetConversationID(HWND hDlg, int item, string& s_id)
{
    // Extract the ID name from the selected item
    int len = SendDlgItemMessage(hDlg, IDC_List, LB_GETTEXTLEN, (WPARAM)item, NULL);
    if (len < 0)
        return false;

    string s;
    s.resize(len + 1);
    
    SendDlgItemMessage(hDlg, IDC_List, LB_GETTEXT, (WPARAM)item, (LPARAM)&s[0]);

    const char* c0 = strchr(&s[0], '[');
    if (!c0)
        return false;

    c0++;

    const char* c1 = strrchr(&s[0], ']');
    if (!c1)
        return false;

    c1--;

    len = c1 - c0 + 1;

    if (len < 40 || len > 63)
        return false;

    char fname[64];
    ZERO(fname);
    int i = 0;
    while (c0 <= c1)
    {
        fname[i] = *c0;
        c0++;
        i++;
    }

    s_id = fname;
    return true;
}

// Used by SelectConversation dialog
void ReloadConversationList(HWND hDlg)
{
    ////////////////////////////////////////////////////////////////
    // Load any messages which are pending on the server
    string err_msg;
    int nmessages_retrieved = 0;
    bool status = RetrievePendingMessages(nmessages_retrieved, err_msg);

    if (status)
        LoadCachedPendingMessages(hDlg, err_msg);

    HANDLE h;
    WIN32_FIND_DATA data;
    STRING filespec;

    PtrSet<STRING> existing_conversations, pending_messages;

    // Parse the pending messages
    STRING pending_dir;
    pending_dir = s_data_dir;
    pending_dir.AppendSubDir("pending");

    filespec.SetValidFileName(pending_dir, "*");

    h = FindFirstFile(filespec, &data);

    while (h != INVALID_HANDLE_VALUE)
    {
        if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            if (FindNextFile(h, &data) == FALSE)
            {
                FindClose(h);
                break;
            }
        }

        if (StrLen(data.cFileName) > 32)
        {
            vector<uint8_t> remote_id;
            ascii_char_to_bin(data.cFileName, remote_id);
            if (remote_id.size() == 32)
                pending_messages.Append(new STRING(data.cFileName));
        }

        if (FindNextFile(h, &data) == FALSE)
        {
            FindClose(h);
            break;
        }
    }

    // Parse the existing conversations
    filespec.SetValidFileName(s_data_dir, "*");

    h = FindFirstFile(filespec, &data);

    while (h != INVALID_HANDLE_VALUE)
    {
        if (data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
        {
            if (FindNextFile(h, &data) == FALSE)
            {
                FindClose(h);
                break;
            }
        }

        if (StrLen(data.cFileName) > 32)
        {
            vector<uint8_t> remote_id;
            ascii_char_to_bin(data.cFileName, remote_id);
            if (remote_id.size() == 32)
                existing_conversations.Append(new STRING(data.cFileName));
        }

        if (FindNextFile(h, &data) == FALSE)
        {
            FindClose(h);
            break;
        }
    }

    SendDlgItemMessage(hDlg, IDC_List, LB_RESETCONTENT, NULL, NULL);

    // sort order
    // 1) Existing conversations with pending messages
    // 2) Existing conversations without pending messages
    // 3) Pending messages not associated with an existing conversation

    PtrSet<STRING> conversations_with_pending_messages, conversations_without_pending_messages;

    for (uint32_t i = 0; i < existing_conversations.GetCount(); i++)
    {
        uint32_t idx_pending_msg;
        const STRING* id_of_conversation_with_pending_messges = pending_messages.GetIfExistsConst(existing_conversations.GetIndexedPtrConst(i), &idx_pending_msg);

        if (id_of_conversation_with_pending_messges == 0)
            conversations_without_pending_messages.Append(new STRING(existing_conversations.GetIndexedPtrConst(i)->GetStringConst()));
        else
        {
            conversations_with_pending_messages.Append(new STRING(existing_conversations.GetIndexedPtrConst(i)->GetStringConst()));
            pending_messages.RemoveByIdx(idx_pending_msg);
        }
    }

    PtrSet<STRING> items; // faciliates the sorting of items into appropriate groups

    // Add the existing conversations with pending messages
    for (uint32_t i = 0; i < conversations_with_pending_messages.GetCount(); i++)
    {
        char item[100];
        ZERO(item);

        vector<uint8_t> remote_id;
        ascii_char_to_bin(conversations_with_pending_messages.GetObjectConst(i)->GetStringConst(), remote_id);

        if (remote_id.size() == 32)
        {
            PrivateMessangerIndexElement token;
            memmove(token.m_remote_id, &remote_id[0], 32);
            const PrivateMessangerIndexElement* obj = s_index_db.GetRecord(token);
            if (obj)
            {
                sprintf_s(item, sizeof(item), "*%s [%s]", obj->m_Nickname, conversations_with_pending_messages.GetObjectConst(i)->GetStringConst());
            }
            else
            {
                sprintf_s(item, sizeof(item), "* [%s]", conversations_with_pending_messages.GetObjectConst(i)->GetStringConst());
            }

            items.Append(new STRING(item));
        }
    }

    for (uint32_t i = 0; i < items.GetCount(); i++)
        SendDlgItemMessage(hDlg, IDC_List, LB_ADDSTRING, NULL, (LPARAM)items.GetIndexedPtrConst(i)->GetStringConst());

    items.RemoveAll();

    // Add the existing conversations without pending messages
    for (uint32_t i = 0; i < conversations_without_pending_messages.GetCount(); i++)
    {
        char item[100];
        ZERO(item);

        vector<uint8_t> remote_id;
        ascii_char_to_bin(conversations_without_pending_messages.GetObjectConst(i)->GetStringConst(), remote_id);

        if (remote_id.size() == 32)
        {
            PrivateMessangerIndexElement token;
            memmove(token.m_remote_id, &remote_id[0], 32);
            const PrivateMessangerIndexElement* obj = s_index_db.GetRecord(token);
            if (obj)
            {
                sprintf_s(item, sizeof(item), "%s [%s]", obj->m_Nickname, conversations_without_pending_messages.GetObjectConst(i)->GetStringConst());
            }
            else
            {
                sprintf_s(item, sizeof(item), "[%s]", conversations_without_pending_messages.GetObjectConst(i)->GetStringConst());
            }

            items.Append(new STRING(item));
        }
    }

    for (uint32_t i = 0; i < items.GetCount(); i++)
        SendDlgItemMessage(hDlg, IDC_List, LB_ADDSTRING, NULL, (LPARAM)items.GetIndexedPtrConst(i)->GetStringConst());

    items.RemoveAll();

    // Add the pending messages not assoicated with conversations
    for (uint32_t i = 0; i < pending_messages.GetCount(); i++)
    {
        char item[100];
        ZERO(item);

        vector<uint8_t> remote_id;
        ascii_char_to_bin(pending_messages.GetObjectConst(i)->GetStringConst(), remote_id);

        if (remote_id.size() == 32)
        {
            sprintf_s(item, sizeof(item), "??? [%s]", pending_messages.GetObjectConst(i)->GetStringConst());
            items.Append(new STRING(item));
        }
    }

    for (uint32_t i = 0; i < items.GetCount(); i++)
        SendDlgItemMessage(hDlg, IDC_List, LB_ADDSTRING, NULL, (LPARAM)items.GetIndexedPtrConst(i)->GetStringConst());
}

// Used by SelectConversation dialog
bool OnRenameConversation(HWND hDlg)
{
    KillTimer(main_hWnd, TIMER_ID_CheckMessages); // don't want to be using the TextMessageAPI elsewhere while doing this

    int isel = SendDlgItemMessage(hDlg, IDC_List, LB_GETCURSEL, NULL, NULL);
    if (isel < 0)
    {
        MessageBox(hDlg, "Select a conversation first.", "Error", MB_ICONERROR);
        return false;
    }

    barray my_id, sender_id;

    string err_msg;
    if (GetHashedInstallID(my_id, s_app_password_hash, err_msg) == false)
    {
        MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
        return false;
    }

    string s_sender_id;
    if (GetConversationID(hDlg, isel, s_sender_id) == false)
        return false;

    ascii_char_to_bin(s_sender_id.c_str(), sender_id);

    PrivateMessangerIndexElement token;
    memmove(token.m_remote_id, &sender_id[0], 32);

    const PrivateMessangerIndexElement* rec = s_index_db.GetRecord(token);

    g_selected_conversation_nickname = rec ? rec->m_Nickname : "";

    uint8_t h = 0;

    bool status = false;

    if (DialogBox(instance_handle, MAKEINTRESOURCE(IDD_Rename), hDlg, RenameConversationNicknameDlg) == IDOK)
    {
        while (1)
        {
            if (rec)
            {
                rec->SetNickname(&g_selected_conversation_nickname[0]);
            }
            else
            {
                PrivateMessangerIndexElement new_rec;
                new_rec.SetNickname(&g_selected_conversation_nickname[0]);
                memmove(&new_rec.m_remote_id[0], &sender_id[0], 32);
                bool changes_made = false;
                if (s_index_db.UpdateRecord(new_rec, changes_made, err_msg) == false)
                {
                    MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                    break;
                }

                if (changes_made == false)
                    break; // should not happen
            }

            if (s_index_db.SaveToFile(s_index_db_file, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                break;
            }

            // Save new nickname to the TextMessage database

            h = TextMessageAPI::Open(s_data_dir, &my_id[0], &sender_id[0], &s_app_password_hash[0], err_msg);
            if (h == 0)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return false;
            }

            if (TextMessageAPI::SetNickname(h, g_selected_conversation_nickname.c_str()) == false)
            {
                MessageBox(hDlg, "TextMessageAPI::SetNickname() fails", "Error", MB_ICONERROR);
                break;
            }

            if (TextMessageAPI::Save(h, &s_app_password_hash[0], err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                break;
            }

            status = true;

            break;
        }

        ///////////////////////////////////////////////////////////////////////////
        // Cleanup
        g_selected_conversation_nickname = "";

        if (h)
        {
            if (TextMessageAPI::Close(h) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                status = false;
            }
        }
    }

    return status;
}

bool OnDeleteConversation(HWND hDlg)
{
    KillTimer(main_hWnd, TIMER_ID_CheckMessages); // don't want to be using the TextMessageAPI elsewhere while doing this

    int isel = SendDlgItemMessage(hDlg, IDC_List, LB_GETCURSEL, NULL, NULL);
    if (isel < 0)
    {
        MessageBox(hDlg, "Select a conversation first.", "Error", MB_ICONERROR);
        return false;
    }

    barray my_id, sender_id;

    string err_msg;
    if (GetHashedInstallID(my_id, s_app_password_hash, err_msg) == false)
    {
        MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
        return false;
    }

    string s_sender_id;
    if (GetConversationID(hDlg, isel, s_sender_id) == false)
        return false;

    ascii_char_to_bin(s_sender_id.c_str(), sender_id);

    PrivateMessangerIndexElement token;
    memmove(token.m_remote_id, &sender_id[0], 32);

    const PrivateMessangerIndexElement* rec = s_index_db.GetRecord(token);

    STRING msg;
    msg = "Are you sure that you want to delete?\n\n";
    msg += rec ? rec->m_Nickname : s_sender_id.c_str();
    if (MessageBox(hDlg, msg, "Confirmation", MB_YESNO) == IDNO)
        return false;

    TextMessageAPI::CloseAll();

    STRING messages_db_file;
    messages_db_file.SetValidFileName(s_data_dir, s_sender_id.c_str());

    if (DoesFileExist(messages_db_file) == false)
    {
        STRING msg;
        msg = "File does not exist: ";
        msg += messages_db_file;
        MessageBox(hDlg, msg, "Error", MB_ICONERROR);
        return false;
    }

    if (_unlink(messages_db_file))
    {
        STRING msg;
        msg = "Problem deleting file: ";
        msg += messages_db_file;
        MessageBox(hDlg, msg, "Error", MB_ICONERROR);
        return false;
    }

    bool exists;
    uint32_t idx = s_index_db.GetRecordIndex(token, exists);
    if (exists)
    {
        if (s_index_db.RemoveRecord(idx, err_msg) == false)
        {
            MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
            return false;
        }

        if (s_index_db.SaveToFile(s_index_db_file, err_msg) == false)
        {
            MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
            return false;
        }
    }

    return true;
}

INT_PTR CALLBACK SelectConversation(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        KillTimer(main_hWnd, TIMER_ID_CheckMessages);
        ReloadConversationList(hDlg);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDOK || (LOWORD(wParam) == IDC_List && HIWORD(wParam) == LBN_DBLCLK))
        {
            int idx = SendDlgItemMessage(hDlg, IDC_List, LB_GETCURSEL, NULL, NULL);
            bool status = false;
            while (idx >= 0)
            {
                string fname;
                if (GetConversationID(hDlg, idx, fname) == false)
                    break;


                // Is there an existing conversation with this id?
                STRING conversation_db_file;
                conversation_db_file.SetValidFileName(s_data_dir, &fname[0]);

                string err_msg;

                if (DoesFileExist(conversation_db_file) == false)
                {
                    barray remote_hashed_id;
                    ascii_char_to_bin(&fname[0], remote_hashed_id);

                    if (InitiateNewConversation(&remote_hashed_id[0], &s_app_password_hash[0], err_msg) == false)
                    {
                        if (err_msg.length())
                            MessageBox(hDlg, err_msg.c_str(), "ERROR", MB_ICONERROR);

                        break;
                    }

                    status = true;
                    break;
                }


                TextMessageAPI::CloseAll();

                vector<uint8_t> hashed_remote_ID;
                ascii_char_to_bin(&fname[0], hashed_remote_ID);

                vector<uint8_t> hashed_install_id;

                if (GetHashedInstallID(hashed_install_id, s_app_password_hash, err_msg) == false)
                {
                    MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                    break;
                }

                // First try with the app password
                uint64_t h = TextMessageAPI::Open(s_data_dir, &hashed_install_id[0], &hashed_remote_ID[0], &s_app_password_hash[0], err_msg);

                if (h == 0)
                {
                    // If this fails, prompt for the password
                    vector<uint8_t> pwd_hash;
                    if (GetPassword(hDlg, pwd_hash) == false)
                        break;

                    h = TextMessageAPI::Open(s_data_dir, &hashed_install_id[0], &hashed_remote_ID[0], &pwd_hash[0], err_msg);
                    if (h == 0)
                    {
                        MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                        break;
                    }
                }

                string tmp;
                const char* nickname = TextMessageAPI::GetNickname(0, tmp);

                PrivateMessangerIndexElement token;
                if (token.SetNickname(nickname) == true) // changes made
                {
                    memmove(&token.m_remote_id, &hashed_remote_ID[0], 32);

                    bool changes_made = false;
                    if (s_index_db.UpdateRecord(token, changes_made, err_msg) == true && changes_made)
                    {
                        if (s_index_db.SaveToFile(s_index_db_file, err_msg) == false)
                        {
                            MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                        }
                    }
                }

                status = true;

                break;
            }

            if (status == true)
            {
                SetTimer(main_hWnd, TIMER_ID_CheckMessages, 500, NULL);
            }

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_Rename)
        {
            if (OnRenameConversation(hDlg) == true)
                ReloadConversationList(hDlg);

            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_Delete)
        {
            if (OnDeleteConversation(hDlg) == true)
                ReloadConversationList(hDlg);

            return (INT_PTR)TRUE;
        }

        break;
    }

    }

    return (INT_PTR)FALSE;
}

inline bool hex_string_to_uint8(const char*& s, uint8_t& v)
{
    if (hex_char_to_bin(s, &v, sizeof(v)) == false)
        return false;

    s += sizeof(v) * 2;
    return true;
}

inline bool hex_string_to_id(const char*& s, void *v)
{
    if (hex_char_to_bin(s, v, 32) == false)
        return false;

    s += 64;
    return true;
}

inline bool hex_string_to_uint64(const char*& s, uint64_t& v)
{
    if (hex_char_to_bin(s, &v, 8) == false)
        return false;

    s += 16;
    return true;
}

//
//[My ID] 32 bytes
//
// Return value:
//[size of message1] - 1 byte
//[Hash of ID of sender1] - 32 bytes
//[Timestamp UTC ms] - 8 bytes
//[Message1] 
//[size of message2] - 1 byte
//[Hash of ID of sender2] - 32 bytes
//[Timestamp UTC ms] - 8 bytes
//[message2] 
// ... 
//[size of messageN] - 1 byte - value is zero, indicates that there are no more messages
bool retrieve_messages(const char* data, int& nmessages_retrieved, string& err_msg)
{
    nmessages_retrieved = 0;

    //vector<uint8_t> remote_id_open_thread; 
    //if (TextMessageAPI::GetHashedRemoteID(0, remote_id_open_thread, err_msg) == false)
     //   return false;

    int idx = 0;

    uint64_t current_time_ms = get_time_ms();
    
    while (data && data[0])
    {
        uint8_t nbytes;
        if (hex_string_to_uint8(data, nbytes) == false)
        {
            ERROR_LOCATION(err_msg);
            err_msg += "Invalid data";
            return false;
        }

        if (nbytes == 0)
            break;

        uint8_t hash_of_id_of_sender[32];
        if (hex_string_to_id(data, hash_of_id_of_sender) == false)
        {
            ERROR_LOCATION(err_msg);
            err_msg += "Invalid data";
            return false;
        }

        uint64_t timestamp_ms;
        if (hex_string_to_uint64(data, timestamp_ms) == false)
        {
            ERROR_LOCATION(err_msg);
            err_msg += "Invalid data";
            return false;
        }

        string fname;
        bin_to_ascii_char(hash_of_id_of_sender, 32, fname);

        STRING pending_file;
        pending_file = s_data_dir;
        pending_file.AppendSubDir("pending");
        if (DoesFileExist(pending_file) == false)
        {
            if (_mkdir(pending_file))
            {
                ERROR_LOCATION(err_msg);
                err_msg += "Unable to create directory: ";
                err_msg += (const char*)pending_file;
                return false;
            }
        }

        pending_file.AppendSubDir(fname.c_str());  

        uint8_t b[sizeof(timestamp_ms) + 256];
        ZERO(b);
        
        if (timestamp_ms > current_time_ms)
            timestamp_ms = current_time_ms;  // Don't allow the time to be in the future wrt this system's clock

        memmove(&b[0], &timestamp_ms, sizeof(timestamp_ms));

        memmove(&b[sizeof(timestamp_ms)], data, nbytes);

        vector<uint8_t> buf;

        FILE* stream = 0;
        if (DoesFileExist(pending_file) == false)
        {
            fopen_s(&stream, pending_file, "wb");

            if (!stream)
            {
                ERROR_LOCATION(err_msg);
                err_msg += "Unable to create file: ";
                err_msg += (const char*)pending_file;
                return false;
            }

            buf.resize(sizeof(timestamp_ms)+nbytes+1); // null terminated
            memmove(&buf[0], b, sizeof(timestamp_ms) + nbytes + 1);
        }
        else
        {
            // Need to read and decrypt the current contents of the file
            int len = filelength(pending_file);
            fopen_s(&stream, pending_file, "r+b");

            if (!stream)
            {
                err_msg += "Unable to open file for reading: ";
                err_msg += (const char*)pending_file;
                return false;
            }

            buf.resize(len);

            if (fread(&buf[0], 1, buf.size(), stream) != buf.size())
            {
                fclose(stream);
                ERROR_LOCATION(err_msg);
                err_msg += "Problem reading file: ";
                err_msg += (const char*)pending_file;
                return false;
            }

            // decrypt the current contents of the file
            symmetric_encryption(&buf[0], buf.size(), &s_app_password_hash[0], s_app_password_hash.size());

            buf.resize(len + sizeof(timestamp_ms) + nbytes + 1);
            memmove(&buf[len], b, sizeof(timestamp_ms) + nbytes + 1); // Append the new data to existing buffer (unencrypted contents of the file)

            fflush(stream);

            // About to switch to writing, need to do this first to reset writing to the beginning of the file
            fseek(stream, 0, SEEK_SET);
        }

        // encrypt the data which is about to be written to file
        symmetric_encryption(&buf[0], buf.size(), &s_app_password_hash[0], s_app_password_hash.size());

        if (fwrite(&buf[0], 1, buf.size(), stream) != buf.size())
        {
            fclose(stream);
            ERROR_LOCATION(err_msg);
            err_msg += "Problem writing to  file: ";
            err_msg += (const char*)pending_file;
            return false;
        }

        fclose(stream);

        data += nbytes;
    }

    return true;
}

bool RetrievePendingMessages(int & nmessages_retrieved, string& err_msg)
{
    // Format of the message sent to the server:
    //[GUID] 16 byte random number generated by the client
    // 
    // The following are encrypted with guid
    //      [op] one byte - value is 2 for RetrievePendingMessages()
    //      [Hash of my ID] 32 bytes - hash of my program id
    //
    //  The following are encrypted with modified_guid
    //      [Instance hash] 16 bytes
    //     
    // Format of the message received from the server:
    //[GUID] 16 byte random number geneated by the server
    //
    // The following are encrypted with the modified_guid
    //      [Instance hash] - 16 bytes, generated by the server. Must be returned to the server for the next operation
    //[size of message1] - 1 byte
    //[Hash of ID of sender1] - 32 bytes
    //[Message1] 
    //[size of message2] - 1 byte
    //[Hash of ID of sender2] - 32 bytes
    //[message2] 
    // ... 
    //[size of messageN] - 1 byte - value is zero, indicates that there are no more messages

    nmessages_retrieved = 0;

    //if (TextMessageAPI::IsOpen() == false)
    //   return false;

    // Get the URL for the server
    STRING url;
    int port;
    string url_full;
    if (GetURLFromConfigFile(url_full, s_app_password_hash, err_msg) == false)
        return false;

    if (extract_port_from_url(url_full.c_str(), url, port, err_msg) == false)
    {
        err_msg = "";
        url = "localhost";
        port = 82;
    }

    GUID guid, modified_guid;
    create_random_buffer((uint8_t*)&guid, sizeof(GUID));

    if (compute_modified_guid(guid, modified_guid, err_msg) == false)
        return false;

    vector<uint8_t> instance_hash;
    if (GetInstanceHash(s_app_password_hash, instance_hash, err_msg) == false)
        return false;

    const char op = 2; // op code for this function

    vector<uint8_t> buf;

    int buf_sz = 0;
    buf_sz += sizeof(guid);
    buf_sz += sizeof(op);
    buf_sz += ID_SIZE_BYTES;    // size of my_hashed_id
    buf_sz += instance_hash.size();
    buf_sz += ID_SIZE_BYTES;    // size of the hash of the remote instance

    buf.resize(buf_sz);

    vector<uint8_t> hashed_id;
    if (GetHashedInstallID(hashed_id, s_app_password_hash, err_msg) == false)
        return false;

    uint32_t i = 0;
    memmove(&buf[i], &guid, sizeof(GUID)); i += sizeof(GUID);
    memmove(&buf[i], &op, sizeof(op)); i += sizeof(op);
    memmove(&buf[i], &hashed_id[0], ID_SIZE_BYTES); i += ID_SIZE_BYTES;
    memmove(&buf[i], &instance_hash[0], instance_hash.size()); i += instance_hash.size();

    // Encrypt the op and my_hashed_id with the unmodified guid
    i = sizeof(GUID);
    int sz = sizeof(op) + ID_SIZE_BYTES;
    symmetric_encryption(&buf[i], sz, guid);

    i += sz;

    // Encrypt everything else (instance_hash) with the modified_guid
    symmetric_encryption(&buf[i], buf_sz - i, modified_guid);

    string send_str, receive_str;
    bin_to_hex_char(&buf[0], buf_sz, send_str);

    STRING emsg;
    char term_char = '}';

    int tmout_ms = 2000;
    bool status = SendBufferToServer(url, port, s_component, send_str, receive_str, emsg, term_char, tmout_ms);

    STRING s = receive_str.c_str();
    STRING* s_array = 0;
    int n = s.GetArrayOfDelimitedItemsEx(s_array, '\n');

#ifdef _DEBUG
    string s_instance_hash;
#endif

    while (status == true)
    {
        status = false;
        if (!n)
            break;

        int len = StrLen(s_array[n - 1]);
        if (len < 32)
            break;

        if (s_array[n - 1][0] != '{')
            break;

        if (s_array[n - 1][len - 1] != '}')
            break;

        s_array[n - 1].Remove(len - 1, 1);
        s_array[n - 1].Remove(0, 1);

        vector<uint8_t> ret_buf;
        string ret_val = (const char*)s_array[n - 1];
        hex_char_to_bin(ret_val, ret_buf);

        uint8_t* b = &ret_buf[0];
        memmove(&guid, b, 16);
        b += 16;

        if (compute_modified_guid(guid, modified_guid, err_msg) == false)
            break;

        // Decrypt the instance hash
        uint8_t* instance_hash_from_server = b;
        b += 16;

        symmetric_encryption(instance_hash_from_server, 16, modified_guid);

        bool instance_hash_from_server_matches_my_instance_hash = memcmp(instance_hash_from_server, &instance_hash[0], 16) == 0 ? true : false;
 
        // Decrypt the remaining data
        int data_sz = ret_buf.size() - 32;
        symmetric_encryption(b, data_sz, modified_guid);

        // Only update my instance hash if it does not match the one from teh server
        if (instance_hash_from_server_matches_my_instance_hash == false)
        { 
            if (UpdateInstanceHash(s_app_password_hash, instance_hash_from_server, 0, err_msg) == false)
                break;
        }

        if (IsEqualTo((const char *)b, "000000"))
        {
            // First message has zero length, i.e. there are no messages
            status = true;
            break;
        }

        status = retrieve_messages((const char *)b, nmessages_retrieved, err_msg);

        break;
    }

#ifdef _DEBUG
    STRING config_file, debug_file;
    debug_file.LoadFileName(GetConfigurationFileName(config_file), TRUE, FALSE, FALSE);
    debug_file.AppendSubDir(__FUNCTION__);
    debug_file += ".txt";

    FILE* stream = 0;
    fopen_s(&stream, debug_file, "w");
    if (stream)
    {
        for (int i = 0; i < n; i++)
            fprintf_s(stream, "[%02ld] %s", i, (const char*)s_array[i]);

        fprintf_s(stream, "\nUpdateInstanceHash() %s\n", s_instance_hash.c_str());

        barray check_instance_hash;
        string emsg;
        GetInstanceHash(s_app_password_hash, check_instance_hash, emsg);

        bin_to_ascii_char(&check_instance_hash[0], 16, s_instance_hash);
        fprintf_s(stream, "GetInstanceHash() %s\n", s_instance_hash.c_str());

        fclose(stream);
    }
#endif

    delete[] s_array;

    // Update the pending_messages_status message which is rendered at the top of the client area
    GetPendingMessagesStatus(s_pending_messages_status);

    // Invalid the top of the window to show the pending messages status
    RECT rect;
    GetClientRect(main_hWnd, &rect);
    rect.bottom = s_pending_message_status_font_ht;
    InvalidateRect(main_hWnd, &rect, TRUE);
     
    return status;
}

// Variables used to maintain state for the NewConversation callback
static vector<uint8_t> g_NC_other_id, g_NC_other_id_initial;
static char s_NC_text_message[260]; // 256 char max, some padding added for validation
static char s_NC_nickname[40];      // 32 char max
static char s_NC_password[40];      // 32 char max

// Update index.bin, which maintains the list of nicknames for each remote ID
bool UpdateIndex(const uint8_t* remote_id, const char* name, string& err_msg)
{
    PrivateMessangerIndexElement obj;
    int len = StrLen(name);
    if (len > 32)
        len = 32;

    memmove(obj.m_Nickname, name, len);
    memmove(obj.m_remote_id, remote_id, 32);

    bool changes_made = false;
    if (s_index_db.UpdateRecord(obj, changes_made, err_msg) == false)
        return false;

    if (changes_made == true)
    {
        if (s_index_db.SaveToFile(s_index_db_file, err_msg) == false)
            return false;
    }

    return true;
}

// remote_hashed_id - hash of id of the remote client for this conversation - 32 bytes
// pwd_hash - hash of password for the new conversation - 32 bytes
// nickname - optional name to assign to this remote client / conversation
// msg_to_send - optional message to send to the remote client
bool InitiateNewConversation(const uint8_t* remote_hashed_id, const uint8_t* pwd_hash, string& err_msg, const char *nickname/*=0*/, const char* msg_to_send/*=0*/)
{
    bool create = true;

    vector<uint8_t> my_hashed_id;
    if (GetHashedInstallID(my_hashed_id, s_app_password_hash, err_msg) == false)
        return false;

    // Initiate the conversation the server at the designated URL
    if (StrLen(msg_to_send))
    {
        if (SendTextMessageToServer((const uint8_t*)&my_hashed_id[0], remote_hashed_id, msg_to_send, err_msg) == false)
        {
            return false;
        }
    }

    string url;
    if (GetURLFromConfigFile(url, s_app_password_hash, err_msg) == false)
        return false;

    uint64_t h = TextMessageAPI::Open(s_data_dir, &my_hashed_id[0], remote_hashed_id, pwd_hash, err_msg, create, nickname, url.c_str());
    if (!h)
    {
        if (err_msg.length())
        {
            return false;;
        }

        return true;
    }

    g_db_handles.push_back(h);

    bool sender = true;

    if (StrLen(msg_to_send))
    {
        if (TextMessageAPI::Append(h, sender, msg_to_send, err_msg) == false)
        {
            return false;
        }
    }

    if (TextMessageAPI::Save(h, pwd_hash, err_msg) == false)
    {
        return false;
    }

    // Update the local database which keeps the association between nickname and remote_id
    if (StrLen(nickname))
    {
        if (UpdateIndex(remote_hashed_id, nickname, err_msg) == false)
        {
            return false;
        }
    }

    BOOL bscroll_to_bottom = GetMenuState(GetMenu(main_hWnd), ID_RevealNewMesages, MF_BYCOMMAND) & MF_CHECKED;

    if (bscroll_to_bottom)
        g_conversation_top_index = 0xFFFFFFFF;

    InvalidateRect(main_hWnd, NULL, TRUE);

    SetTimer(main_hWnd, TIMER_ID_CheckMessages, 500, NULL);

     return true;
}

INT_PTR CALLBACK NewConversation(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        ZERO(s_URL);

        string url, err_msg;
        string s_my_id;
        if (GetHashedInstallID_ascii(s_my_id, s_app_password_hash, err_msg) == false)
        {
            MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
            return (INT_PTR)FALSE;
        }
       
        if (GetURLFromConfigFile(url, s_app_password_hash, err_msg) == false)
        {
            MessageBox(hDlg, "Missing URL for remote server", "Error", MB_ICONERROR);
            return (INT_PTR)FALSE;
        }

        SetWindowText(GetDlgItem(hDlg, IDC_MyID), s_my_id.c_str());
        SetWindowText(GetDlgItem(hDlg, IDC_URL), url.c_str());

        if (g_NC_other_id_initial.size() == 32)
        {
            string remote_id;
            bin_to_ascii_char(&g_NC_other_id_initial[0], g_NC_other_id_initial.size(), remote_id);
            SetWindowText(GetDlgItem(hDlg, IDC_OtherID), remote_id.c_str());
            g_NC_other_id = g_NC_other_id_initial;
            g_NC_other_id_initial.resize(0);

        }
        else
            g_NC_other_id.resize(0);

        ZERO(s_NC_text_message);
        ZERO(s_NC_nickname);
        ZERO(s_NC_password);

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            string err_msg;
            bool create = true;

            if (InitiateNewConversation(&g_NC_other_id[0], &s_app_password_hash[0], err_msg, s_NC_nickname, s_NC_text_message) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            
            /*
            vector<uint8_t> my_hashed_id;
            if (GetHashedInstallID(my_hashed_id, s_app_password_hash, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }

            // Initiate the conversation the server at the designated URL
            if (StrLen(s_NC_text_message))
            {
                if (SendTextMessageToServer((const uint8_t*)&my_hashed_id[0], &g_NC_other_id[0], s_NC_text_message, err_msg) == false)
                {
                    MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
            }
  
            uint64_t h = TextMessageAPI::Open(s_data_dir, &my_hashed_id[0], &g_NC_other_id[0], pwd_hash, err_msg, create, s_NC_nickname, s_URL);
            if (!h)
            {
                if (err_msg.length())
                {
                    MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }

                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }

            g_db_handles.push_back(h);

            bool sender = true;

            if (StrLen(s_NC_text_message))
            {
                if (TextMessageAPI::Append(h, sender, s_NC_text_message, err_msg) == false)
                {
                    MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                    EndDialog(hDlg, LOWORD(wParam));
                    return (INT_PTR)TRUE;
                }
            }

            if (TextMessageAPI::Save(h, pwd_hash, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }

            // Update the local database which keeps the association between nickname and remote_id
            if (UpdateIndex(&g_NC_other_id[0], s_NC_nickname, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }

            BOOL bscroll_to_bottom = GetMenuState(GetMenu(main_hWnd), ID_RevealNewMesages, MF_BYCOMMAND) & MF_CHECKED;

            if (bscroll_to_bottom)
                g_conversation_top_index = 0xFFFFFFFF;

            InvalidateRect(main_hWnd, NULL, TRUE);

            SetTimer(main_hWnd, TIMER_ID_CheckMessages, 500, NULL);
            */

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_OtherID)
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

                char s_other_id[64];
                ZERO(s_other_id);
                GetWindowText(GetDlgItem(hDlg, IDC_OtherID), s_other_id, sizeof(s_other_id)-1);

                string s_my_id, err_msg;
                if (GetHashedInstallID_ascii(s_my_id, s_app_password_hash, err_msg) == false)
                    break;

                // Make sure that the other id is not the same as this ID
                if (IsEqualTo(s_other_id, s_my_id.c_str()))
                    break; 
   
                ascii_char_to_bin(s_other_id, g_NC_other_id);

                // Make sure that other_id has the right size
                if (g_NC_other_id.size() != 32)
                    break;

                // Make sure that we don't already have a  database for this id
                if (TextMessageAPI::Exists(s_data_dir, &g_NC_other_id[0]) == true)
                    break;

                EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);

                break;
            }
        }

        // Limit text messages to 256 bytes
        if (LOWORD(wParam) == IDC_InitialMessage)
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                ZERO(s_NC_text_message);
                GetWindowText(GetDlgItem(hDlg, IDC_InitialMessage), s_NC_text_message, sizeof(s_NC_text_message) - 1);
                if (StrLen(s_NC_text_message) > 256)
                {
                    s_NC_text_message[256] = 0;
                    SetWindowText(GetDlgItem(hDlg, IDC_InitialMessage), s_NC_text_message);
                }
            }

            break;
        }

        // Limit the nickname to 32 characters
        if (LOWORD(wParam) == IDC_OtherNickname)
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                ZERO(s_NC_nickname);
                GetWindowText(GetDlgItem(hDlg, IDC_OtherNickname), s_NC_nickname, sizeof(s_NC_nickname) - 1);
                if (StrLen(s_NC_nickname) > 32)
                {
                    s_NC_nickname[32] = 0;
                    SetWindowText(GetDlgItem(hDlg, IDC_OtherNickname), s_NC_nickname);
                }
            }

            break;
        }

        // Limit the password to 32 characters
        if (LOWORD(wParam) == IDC_Password)
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                ZERO(s_NC_password);
                GetWindowText(GetDlgItem(hDlg, IDC_Password), s_NC_password, sizeof(s_NC_password) - 1);
                if (StrLen(s_NC_password) > 32)
                {
                    memset(&s_NC_password[32], sizeof(s_NC_password) - 32, 0);
                    SetWindowText(GetDlgItem(hDlg, IDC_Password), s_NC_password);
                }
            }

            break;
        }

        break;
    }
    return (INT_PTR)FALSE;
}

bool AddMeToTheServer(const char* s_URL, string& err_msg)
{
    /////////////////////////////////////////////////////////////////////
    // Contents of the message sent to the server
    //[leading_guid] - 16 bytes
    //[op] - one byte, encrypted with leading guid 
    //[hashed id of client] - 32 bytes, encrypted with leading guid

    vector<uint8_t> hashed_id;
    if (GetHashedInstallID(hashed_id, s_app_password_hash, err_msg) == false)
        return false;

    uint8_t buf[16 + 1 + 32]; // leading_guid, op, hashed_id
    ZERO(buf);

    create_random_buffer(&buf[0], 16); // leading guid

    buf[16] = 0; // op is zero

    memmove(&buf[17], &hashed_id[0], hashed_id.size()); // hashed id

    GUID leading_guid;
    memmove(&leading_guid, buf, sizeof(leading_guid));
    symmetric_encryption(&buf[16], sizeof(buf) - sizeof(leading_guid), leading_guid);

    string send_str;
    bin_to_hex_char(&buf[0], sizeof(buf), send_str);

    STRING url;
    int port = 80;

    if (extract_port_from_url(s_URL, url, port, err_msg) == false)
        return false;

    string receive_str;
    STRING emsg;
    char term_char = '}';
    int tmout_ms = 5000;
    bool status = SendBufferToServer(url, port, s_component, send_str, receive_str, emsg, term_char, tmout_ms);

    // Message has been sent to the server, now process the received buffer

    STRING s = receive_str.c_str();
    STRING* s_array = 0;

    // Data retuned from the server is all separate lines of text
    int n = s.GetArrayOfDelimitedItemsEx(s_array, '\n');  

    FILE* stream = 0;

    // Don't bother parsing that data from the server unless SendBufferToServer() returns true
    // Payload is on the last line, inside the curly brackets
    //
    // Payload (data from server) format:
    // 
    // [leading_guid] 16 bytes
    // 
    // The following items are encrypted with the leading_guid
    //      [instance_hash] 16 bytes
    //      [bytecode_size] 2 bytes
    //      [bytecode] remainder, must match bytecode_size

    while (status == true)
    {
        status = false;
        if (!n)
            break;

        // The payload data is on the last line and is enclosed in curly brackets
        int len = StrLen(s_array[n - 1]);
        if (len < 32)
            break;

        // If the last like is not enclosed in curly brackets, don't proceed further
        if (s_array[n - 1][0] != '{') 
            break;

        if (s_array[n - 1][len - 1] != '}')
            break;

        // Remove the curly brackets
        s_array[n - 1].Remove(len - 1, 1);
        s_array[n - 1].Remove(0, 1);

        // Decode the leading guid for the payload buffer
        vector<uint8_t> ret_buf;
        string ret_val = (const char*)s_array[n - 1];
        hex_char_to_bin(ret_val, ret_buf);

        GUID leading_guid;
        memmove(&leading_guid, &ret_buf[0], 16);

        // Don't modify the leading_guid in this case

        uint8_t instance_hash[16];
        ZERO(instance_hash);

        uint8_t* b = &ret_buf[16];

        memmove(instance_hash, b, sizeof(instance_hash)); 
        b += sizeof(instance_hash);
        symmetric_encryption(instance_hash, sizeof(instance_hash), leading_guid);

        int bytecode_size = ret_buf.size() - (sizeof(GUID) + sizeof(instance_hash));
        symmetric_encryption(b, bytecode_size, leading_guid); // decrypt bytecode 

        uint16_t bytecode_size_hdr;
        memmove(&bytecode_size_hdr, b, sizeof(bytecode_size_hdr));
        b += 2;

        bytecode_size -= 2; // The first two bytes indicate the size the of the bytecode, need to take that out of the length

        // Validate that the bytecode_size matches the remaining size of the payload
        if (bytecode_size_hdr != bytecode_size)
        {
            ERROR_LOCATION(err_msg);
            err_msg += "Invalid data received from the server.";
            break;
        }

        // Update the instance hash in the config.bin file. 
        // Initially the instance_hash is set to zero by the client.
        // This is our first instance_hash computed by the server
        if (UpdateInstanceHash(s_app_password_hash, instance_hash, 0, err_msg) == false)
            break;

        ////////////////////////////////////////////////////////////
        // Write the bytecode to file
        STRING bytecode_file;
        GetByteCodeFileName(bytecode_file);

        fopen_s(&stream, bytecode_file, "wb");

        if (stream == 0)
        {
            ERROR_LOCATION(err_msg);
            err_msg += "Unable to create file: ";
            err_msg += (const char*)bytecode_file;
            break;
        }

        if (fwrite(b, 1, bytecode_size, stream) != bytecode_size)
        {
            ERROR_LOCATION(err_msg);
            err_msg += "() Problem writing to file: ";
            err_msg += (const char*)bytecode_file;
            break;
        }

        fclose(stream);

        status = true;

        break;
    }

    if (stream) fclose(stream);
    delete[] s_array;

    return status;
}

bool prepare_protected_code_obj(const char* bytecode_file, RunCompiledCode& obj, vector<uint8_t>& compiled_code, string& err_msg)
{
    STRING emsg;
    uint8_t seed[SEED_SZ_BYTES];

    if (ReadCompiledCodeFile(bytecode_file, compiled_code, emsg) == false)
    {
        err_msg = (const char*)emsg;
        return false;
    }

    string pwd;
    if (GetHashedInstallID_ascii(pwd, s_app_password_hash, err_msg) == false)
        return false;

    //HCURSOR prev_cursor = SetCursor(LoadCursor(instance_handle, MAKEINTRESOURCE(IDC_WAIT)));

    // Create the seed buffer from the hashed InstallID
    ZeroMemory(seed, sizeof(seed)); // seed for generating the random buffer which is use to encrypt the compiled source
    CreateRandomBufferFromSeed((const uint8_t*)pwd.c_str(), StrLen(pwd.c_str()), 0, seed, sizeof(seed), 0x10000);

    uint64_t nbits = compiled_code.size() * 8; // nbits size does not matter as long as it is <= 8 times the size of the compiled code buffer
    obj.Initialize(&compiled_code, nbits, seed);

    AssignExternalFunctionAddresses(obj);

    obj.SetEncryptionBuffer(GetEBufferForSeed(seed));

    //SetCursor(prev_cursor);
 
    return true;
}

// s_URL - url of the server
// my_hashed_id - hash of my 32 byte ID from my config.bin file
// remote_hashed_id - hash of remote client's 32 byte ID from their config.bin file - This function sends a message to the remote client
//                    via the server.
// msg - null terminated text of the message, up to 256 bytes. If the message exceeds 256 bytes, it will be terminated at 256 bytes on send.
bool SendTextMessageToServer(const uint8_t* my_hashed_id, const uint8_t* remote_hashed_id, const char* msg, string& err_msg)
{
    // Format of the message to sent to the server:
    //[GUID] 16 byte random number generated by the client
    // 
    // The following are encrypted with guid
    //      [op] one byte - value is 1 for SendTextMessageToServer()
    //      [Hash of my ID] 32 bytes - hash of my program id
    // 
    //  The following are encrypted with modified_guid
    //      [Instance hash] 16 bytes
    //      [Hash of ID I want to communicate with] 32 bytes - this will be the hash of the program id of the instance I want to communicate with
    //      [Message size] 2 bytes 
    //      [Message] number of bytes as specified by message size, ascii text only

    // Format of the message received from the server:
    //[GUID] 16 byte random number geneated by the server
    //
    // The following are encrypted with the modified_guid
    //      [Instance hash] - 16 bytes, generated by the server. Must be returned to the server for the next operation
    //      [status] "0000" - for success or 4 digit failure code on fail 

    STRING s_msg = msg;

    if (s_msg.Replace("\r", "")) // get rid of CR characters if there are any in the message
        msg = s_msg.GetStringConst();

    int len = StrLen(msg);

    uint16_t msg_len;
    if (len > 256)
        msg_len = 256;
    else
        msg_len = (uint16_t)len;

    GUID guid, modified_guid; // guid is sent, modified_guid is used to encrypt what is sent
    create_random_buffer((uint8_t*)&guid, 16);

    if (compute_modified_guid(guid, modified_guid, err_msg) == false)
        return false;

    vector<uint8_t> instance_hash;
    if (GetInstanceHash(s_app_password_hash, instance_hash, err_msg) == false)
        return false;

    const char op = 1; // op code for this function

    vector<uint8_t> buf;

    int buf_sz = 0;
    buf_sz += sizeof(guid);
    buf_sz += sizeof(op);
    buf_sz += ID_SIZE_BYTES;    // size of my_hashed_id
    buf_sz += instance_hash.size();
    buf_sz += ID_SIZE_BYTES;    // size of the hash of the remote instance
    buf_sz += sizeof(msg_len);  // uint16_t variable, number of characters in the message
    buf_sz += msg_len;          // number of characters in the message

    buf.resize(buf_sz);

    uint32_t i = 0;
    memmove(&buf[i], &guid, sizeof(GUID)); i += sizeof(GUID);
    memmove(&buf[i], &op, sizeof(op)); i += sizeof(op);
    memmove(&buf[i], my_hashed_id, ID_SIZE_BYTES); i += ID_SIZE_BYTES;
    memmove(&buf[i], &instance_hash[0], instance_hash.size()); i += instance_hash.size();
    memmove(&buf[i], remote_hashed_id, ID_SIZE_BYTES); i += ID_SIZE_BYTES;

    memmove(&buf[i], &msg_len, 2); i += 2;
    if (msg_len)
    {
        memmove(&buf[i], msg, msg_len);
        i += msg_len;
    }

    ASSERT(i == buf_sz);

    // Encrypt the op and my_hashed_id with the unmodified guid
    i = sizeof(GUID);
    int sz = sizeof(op) + ID_SIZE_BYTES;
    symmetric_encryption(&buf[i], sz, guid);

    i += sz;

    // Encrypt everything else (instance_hash, remote_hashed_id, message) with the modified_guid
    symmetric_encryption(&buf[i], buf_sz - i, modified_guid);

    string send_str;
    bin_to_hex_char(&buf[0], buf_sz, send_str);

    STRING url;

    string server_url;
    if (GetURLFromConfigFile(server_url, s_app_password_hash, err_msg) == false)
        return false;

    int port = 80;

    if (extract_port_from_url(server_url.c_str(), url, port, err_msg) == false)
        return false;

    string receive_str;
    STRING emsg;
    char term_char = '}';

    int tmout_ms = 2000;
    bool status = SendBufferToServer(url, port, s_component, send_str, receive_str, emsg, term_char, 2000);

    STRING s = receive_str.c_str();
    STRING* s_array = 0;
    int n = s.GetArrayOfDelimitedItemsEx(s_array, '\n');

#ifdef _DEBUG
    string s_instance_hash;
#endif

    while (status == true)
    {
        status = false;
        if (!n)
            break;

        int len = StrLen(s_array[n - 1]);
        if (len < 32)
            break;

        if (s_array[n - 1][0] != '{')
            break;

        if (s_array[n - 1][len - 1] != '}')
            break;

        s_array[n - 1].Remove(len - 1, 1);
        s_array[n - 1].Remove(0, 1);

        vector<uint8_t> ret_buf;
        string ret_val = (const char*)s_array[n - 1];
        hex_char_to_bin(ret_val, ret_buf);

        uint8_t* b = &ret_buf[0];
        memmove(&guid, b, 16); 
        b += 16;

        if (compute_modified_guid(guid, modified_guid, err_msg) == false)
            break;

        // Decrypt the instance hash
        uint8_t* instance_hash = b;
        b += 16;
        symmetric_encryption(instance_hash, 16, modified_guid);

#ifdef _DEBUG
        bin_to_ascii_char(instance_hash, 16, s_instance_hash);
#endif

        // Decrypt the message from the server
        symmetric_encryption(b, ret_buf.size() - 32, modified_guid);

        const char* check = (const char*)b;
        if (IsEqualTo(check, "0000", FALSE, 4) == FALSE) // Success
        {
            ERROR_LOCATION(err_msg);
            err_msg = "Failed with code: ";
            err_msg += check;
            break;
        }

        // Write the instance hash to the config file
        //uint8_t prev_instance_hash[16];
        //ZERO(prev_instance_hash);
        if (UpdateInstanceHash(s_app_password_hash, instance_hash, 0/*prev_instance_hash*/, err_msg) == false)
            break;

        // I'm extracting the prev_instance_hash here because I thought that we might want to roll it back after a failure to verify (new server function)
        // that the server has received confirmation that the client (here) has updated the instance hash.
        // However, I realize now that the verification can simply happen the next time that the client communicates with the server, probably to retrieve messages.
        // The server will at that point accept the old or the new instance hash, however, upon receiving the new instance hash, the server will no longer accept
        // the old instance hash. In other words the next communication with the server LOCKS IN the change to the instance hash on the server.
        //
        // This requires only changes on the server side and does not complicate the communications protocol.

        status = true;

        break;
    }

#ifdef _DEBUG
    STRING config_file, debug_file;
    debug_file.LoadFileName(GetConfigurationFileName(config_file), TRUE, FALSE, FALSE);
    debug_file.AppendSubDir(__FUNCTION__);
    debug_file += ".txt";

    FILE* stream = 0;
    fopen_s(&stream, debug_file, "w");
    if (stream)
    {
        for (int i = 0; i < n; i++)        
            fprintf_s(stream, "[%02ld] %s", i, (const char*)s_array[i]);

        fprintf_s(stream, "\nUpdateInstanceHash() %s\n", s_instance_hash.c_str());

        barray check_instance_hash;
        string emsg;
        GetInstanceHash(s_app_password_hash, check_instance_hash, emsg);

        bin_to_ascii_char(&check_instance_hash[0], 16, s_instance_hash);
        fprintf_s(stream, "GetInstanceHash() %s\n", s_instance_hash.c_str());

        fclose(stream);
    }
#endif

    delete[] s_array;

    return status;
}

INT_PTR CALLBACK AppPasswordDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        STRING file_name;
        GetConfigurationFileName(file_name);

        if (DoesFileExist(file_name) == false)
            SetWindowText(hDlg, "Create App Password - 32 char max");
        else
            SetWindowText(hDlg, "Enter App Password");

        break;
    }


    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char pwd[32];
            ZERO(pwd);
            GetWindowText(GetDlgItem(hDlg, IDC_Password), pwd, sizeof(pwd));

            hash_id((uint8_t*)pwd);
            hash_id((uint8_t*)pwd);

            s_pwd_hash.resize(32);
            memmove(&s_pwd_hash[0], pwd, 32);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)FALSE;
        }

        break;
    }


    return 0;
}

bool GetAppPassword(HWND hWnd)
{
    if (DialogBox(hInst, MAKEINTRESOURCE(IDD_Password), hWnd, AppPasswordDlg) != IDOK)
        return false;

    s_app_password_hash.resize(32);
    memmove(&s_app_password_hash[0], &s_pwd_hash[0], 32);

    s_pwd_hash.resize(0);

    return true;
}

static RunCompiledCode s_run_compiled_code;
static vector<uint8_t> s_compiled_code;

bool compute_modified_guid(const GUID& guid, GUID& modified_guid, string& err_msg)
{
    STRING bytecode_file;
    GetByteCodeFileName(bytecode_file);

    if (s_run_compiled_code.IsInitialized() == false)
    {
        if (prepare_protected_code_obj(bytecode_file, s_run_compiled_code, s_compiled_code, err_msg) == false)
            return false;
    }

    vector<Value*> params;

    params.resize(2);

    Value* v_guid = new Value;
    v_guid->initialize_barray(); // initialize the value as a one dimensional barray

    vector<uint8_t> v_guid_data;
    v_guid_data.resize(sizeof(guid));
    memmove(&v_guid_data[0], &guid, sizeof(guid));
    v_guid->set_barray(0, v_guid_data); // assign data to the parameter value 

    InstallID id;
    if (GetInstallID(id, s_app_password_hash, err_msg) == false)
    {
        delete v_guid;
        return false;
    }

    Value* v_id = new Value;
    v_id->initialize_barray(); // initialize the value as a one dimensional barray

    vector<uint8_t> v_id_data;
    v_id_data.resize(sizeof(id));
    memmove(&v_id_data[0], &id, sizeof(id));
    v_id->set_barray(0, v_id_data); // assign data to the parameter value

    params[0] = v_guid;
    params[1] = v_id;

    bool run_status = s_run_compiled_code.Run(params);
    if (run_status == false)
    {
        err_msg = "Problem running bytecode file: ";
        err_msg += (const char *)bytecode_file;
    }

    if (run_status == true)
        memmove(&modified_guid, &v_guid->get_barray(0)[0], sizeof(GUID));

    // Delete the Value variables 
    delete v_guid;
    delete v_id;

    return run_status;
}

INT_PTR CALLBACK ChangePassword(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char pwd[34];
            ZERO(pwd);
            GetWindowText(GetDlgItem(hDlg, IDC_CurrentPassword), pwd, sizeof(pwd));

            int len = StrLen(pwd);
            if (len > 32 || len < 4)
            {
                MessageBox(hDlg, "Current Password may not be more than 32 characters nor less than 4 characters.", "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            hash_id((uint8_t*)pwd);
            hash_id((uint8_t*)pwd);

            if (memcmp(pwd, &s_app_password_hash[0], 32))
            {
                MessageBox(hDlg, "Incorrect current password entered.", "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            ZERO(pwd);
            GetWindowText(GetDlgItem(hDlg, IDC_NewPassword), pwd, sizeof(pwd));

            len = StrLen(pwd);
            if (len > 32 || len < 4)
            {
                MessageBox(hDlg, "New Password may not be more than 32 characters nor less than 4 characters.", "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            hash_id((uint8_t*)pwd);
            hash_id((uint8_t*)pwd);

            if (memcmp(pwd, &s_app_password_hash[0], 32) == 0)
            {
                EndDialog(hDlg, IDOK); // no change
                return (INT_PTR)TRUE;
            }

            string err_msg;
            InstallID my_id;
            GetInstallID(my_id, s_app_password_hash, err_msg);
   
            barray pwd_new;
            pwd_new.resize(32);
            memmove(&pwd_new[0], pwd, 32);
            if (ChangeConfigFilePassword(s_app_password_hash, pwd_new, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            int ntotal = 0;
            int nupdated = 0;
            TextMessageAPI::UpdatePasswordForAllConversations(s_data_dir, (const uint8_t *)&my_id, &s_app_password_hash[0], &pwd_new[0], ntotal, nupdated, err_msg);

            s_app_password_hash = pwd_new;

            InvalidateRect(main_hWnd, NULL, TRUE);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)FALSE;
        }

        break;
    }


    return 0;
}


INT_PTR CALLBACK ChangeURL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char pwd[34];
            ZERO(pwd);
            GetWindowText(GetDlgItem(hDlg, IDC_Password), pwd, sizeof(pwd));

            hash_id((uint8_t*)pwd);
            hash_id((uint8_t*)pwd);

            if (memcmp(pwd, &s_app_password_hash[0], 32))
            {
                MessageBox(hDlg, "Incorrect current password entered.", "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            char new_url[256];
            GetWindowText(GetDlgItem(hDlg, IDC_NewURL), new_url, sizeof(new_url));

            string err_msg;
            InstallID my_id;
            GetInstallID(my_id, s_app_password_hash, err_msg);

            if (ChangeServerURL(s_app_password_hash, new_url, err_msg) == false)
            {
                MessageBox(hDlg, err_msg.c_str(), "Error", MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            int ntotal = 0;
            int nupdated = 0;
            TextMessageAPI::UpdateURLForAllConversations(s_data_dir, (const uint8_t*)&my_id, &s_app_password_hash[0], new_url, ntotal, nupdated, err_msg);

            InvalidateRect(main_hWnd, NULL, TRUE);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)FALSE;
        }

        break;
    }


    return 0;
}