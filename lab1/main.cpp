#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cctype>

HWND hwnd, log_edit, pass_edit, new_pass_edit, log_btn, add_btn, change_btn, rest_btn, block_btn;
enum menu_ids { m_about = 1, m_view, m_chpass, m_users, m_add, m_block, m_rest, m_save };

struct user {
    std::string username;
    std::string password;
    bool blocked;
    bool restrict;
};

std::vector<user> users;
user* current = nullptr;

int attempts = 0;
std::string status_text = "";

void set_status(std::string txt) {
    status_text = txt;
    InvalidateRect(hwnd, NULL, TRUE);
}

void build_menu() {
    HMENU hm = CreateMenu();
    HMENU sm = CreatePopupMenu();
    HMENU hlp = CreatePopupMenu();

    AppendMenu(sm, MF_STRING, m_chpass, "change password");
    if (current && current->username == "ADMIN") {
        AppendMenu(sm, MF_STRING, m_view, "view users");
        AppendMenu(sm, MF_STRING, m_add, "add user");
        AppendMenu(sm, MF_STRING, m_block, "block user");
        AppendMenu(sm, MF_STRING, m_rest, "toggle restrictions");
    }
    AppendMenu(sm, MF_SEPARATOR, 0, NULL);
    AppendMenu(sm, MF_STRING, m_save, "save");
    AppendMenu(hm, MF_POPUP, (UINT_PTR)sm, "menu");
    AppendMenu(hlp, MF_STRING, m_about, "about");
    AppendMenu(hm, MF_POPUP, (UINT_PTR)hlp, "help");
    SetMenu(hwnd, hm);
}

void hide_all() {
    ShowWindow(log_edit, SW_HIDE);
    ShowWindow(pass_edit, SW_HIDE);
    ShowWindow(new_pass_edit, SW_HIDE);
    ShowWindow(add_btn, SW_HIDE);
    ShowWindow(log_btn, SW_HIDE);
    ShowWindow(change_btn, SW_HIDE);
    ShowWindow(rest_btn, SW_HIDE);
    ShowWindow(block_btn, SW_HIDE);
}

void load() {
    std::ifstream f("users.txt");
    if (!f) {
        users.push_back({ "ADMIN", "", false, false });
        return;
    }
    user u;
    std::string p;
    while (f >> u.username >> p >> u.blocked >> u.restrict) {
        u.password = (p == "NULL") ? "" : p;
        users.push_back(u);
    }
}

void save() {
    std::ofstream f("users.txt");
    for (auto& u : users) {
        std::string p = u.password.empty() ? "NULL" : u.password;
        f << u.username << " " << p << " " << u.blocked << " " << u.restrict << "\n";
    }
    set_status("changes saved");
}

user* find_user(std::string name) {
    for (auto& u : users)
        if (u.username == name)
            return &u;

    return nullptr;
}

bool check_password(char* password) {
    bool letter = false;
    bool digit = false;
    bool op = false;

    if (password == nullptr || password[0] == '\0') return false;

    for (int i = 0; password[i] != '\0'; i++) {
        char c = password[i];
        if (isalpha((unsigned char)c)) letter = true;
        else if (isdigit((unsigned char)c)) digit = true;
        else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=') op = true;
    }
    return letter && digit && op;
}

void login() {
    char n[128], p[128];
    GetWindowText(log_edit, n, 128);
    GetWindowText(pass_edit, p, 128);
    SetWindowText(log_edit, "");
    SetWindowText(pass_edit, "");

    user* u = find_user(n);

    if (u != nullptr && u->username == n && u->password == p) {
        if (u->blocked) {
            set_status("account is blocked");
            return;
        }

        current = u;
        attempts = 0;
        
        build_menu();
        hide_all();
        set_status("");
    } else {
        attempts++;
        set_status("incorrect login or password");
        
        if (attempts >= 3) {
            set_status("too many attempts");
            MessageBox(hwnd, "access denied", "security", MB_ICONSTOP);
            PostQuitMessage(0);
        }
    }
}

void add_user() {
    char n[128], p[128];
    GetWindowText(log_edit, n, sizeof(n));
    GetWindowText(pass_edit, p, sizeof(p));
    SetWindowText(log_edit, "");
    SetWindowText(pass_edit, "");

    if (find_user(n)) {
        set_status("user already exists");
        return;
    }

    user new_user;
    new_user.username = n;
    new_user.password = p;
    new_user.blocked = 0;
    new_user.restrict = 0;

    users.push_back(new_user);
    hide_all();
    set_status("");
}

void change_password() {
    char n[128], p[128], np[128];
    GetWindowText(log_edit, n, sizeof(n));
    GetWindowText(pass_edit, p, sizeof(p));
    GetWindowText(new_pass_edit, np, sizeof(np));
    SetWindowText(log_edit, "");
    SetWindowText(pass_edit, "");
    SetWindowText(new_pass_edit, "");

    if (current->username != "ADMIN") strncpy(n, current->username.c_str(), sizeof(n) - 1);

    user* u = find_user(n);

    if (!u) {
        set_status("user not found");
        return;
    }

    if (u->password != p) {
        set_status("wrong password");
        return;
    }

    if (u->restrict && !check_password(np)) {
        set_status("restriction active: weak password");
        return;
    }

    u->password = np;
    hide_all();
    set_status("");
}

void restrict() {
    char n[128];
    GetWindowText(log_edit, n, sizeof(n));
    SetWindowText(log_edit, "");

    user* u = find_user(n);

    if (!u) {
        set_status("user not found");
        return;
    }

    u->restrict = !u->restrict;
    hide_all();
    set_status("");
}

void block() {
    char n[128];
    GetWindowText(log_edit, n, sizeof(n));
    SetWindowText(log_edit, "");
    user* u = find_user(n);

    if (!u) {
        set_status("user not found");
        return;
    }

    u->blocked = !u->blocked;
    hide_all();
    set_status("");
}

void wPaint(HWND hw) {
    PAINTSTRUCT ps;
    HDC Canvas = BeginPaint(hw, &ps);
    SetTextColor(Canvas, RGB(255, 0, 0));
    SetBkMode(Canvas, TRANSPARENT);

    SIZE textSize;
    GetTextExtentPoint32(Canvas, status_text.c_str(), status_text.length(), &textSize);
    TextOut(Canvas, (440 - textSize.cx - 15) / 2, 140, status_text.c_str(), status_text.length());
    EndPaint(hw, &ps);
}

long WINAPI MainEventManager(HWND hw, UINT Code, WPARAM wParam, LPARAM lParam) {
    switch(Code) {
        case WM_PAINT:
            wPaint(hw);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hw);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_COMMAND:
            if ((HWND)lParam == log_btn) login();
            else if ((HWND)lParam == add_btn) add_user();
            else if ((HWND)lParam == change_btn) change_password();
            else if ((HWND)lParam == rest_btn) restrict();
            else if ((HWND)lParam == block_btn) block();

            switch (LOWORD(wParam)) {
                case m_about:
                    MessageBox(hw, "author: FB-35 Dmytro Kubiuk\nindividual_task: The presence of letters, numbers, and arithmetic operation symbols.", "about", MB_OK | MB_ICONINFORMATION);
                    break;

                case m_view: {
                    std::string result = "user list:\n";
                    for (auto& u : users)
                        result += u.username + " (b:" + std::to_string(u.blocked) + " r:" + std::to_string(u.restrict) + ")\n";
                    MessageBox(hw, result.c_str(), "info", MB_OK);
                    break;
                }

                case m_add:
                    set_status("enter login and password for the new user");
                    hide_all();
                    ShowWindow(log_edit, SW_NORMAL);
                    ShowWindow(pass_edit, SW_NORMAL);
                    ShowWindow(add_btn, SW_NORMAL);
                    SetWindowText(log_edit, "login");
                    SetWindowText(pass_edit, "password");
                    break;

                case m_chpass:
                    set_status("enter login, current and new password");
                    hide_all();
                    ShowWindow(log_edit, SW_NORMAL);
                    ShowWindow(pass_edit, SW_NORMAL);
                    ShowWindow(new_pass_edit, SW_NORMAL);
                    ShowWindow(change_btn, SW_NORMAL);
                    break;

                case m_rest:
                    set_status("enter login to toggle user restrictions");
                    hide_all();
                    ShowWindow(log_edit, SW_NORMAL);
                    ShowWindow(rest_btn, SW_NORMAL);
                    break;

                case m_block:
                    set_status("enter login to block or unblock the user");
                    hide_all();
                    ShowWindow(log_edit, SW_NORMAL);
                    ShowWindow(block_btn, SW_NORMAL);
                    break;

                case m_save:
                    save();
                    break;
            }
            return 0;
    }
    return DefWindowProc(hw,Code,wParam,lParam);
}

HWND MainRegister(HINSTANCE Instance, char* Name, int x, int y, int w, int h) {
    static WNDCLASS rc;
    rc.lpfnWndProc = MainEventManager;
    rc.hInstance = Instance;
    rc.hIcon = LoadIcon(NULL, IDI_EXCLAMATION);
    rc.hCursor = LoadCursor(NULL, IDC_ARROW);
    rc.hbrBackground = CreateSolidBrush(RGB(255,255,255));
    rc.lpszClassName = Name;
    RegisterClass(&rc);
    return CreateWindow(Name, Name, WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y, w, h, NULL, NULL, Instance, NULL);
}

int WINAPI WinMain (HINSTANCE osInstance, HINSTANCE osPreviousInstance, LPSTR osParameters, int CommandShow) {
    load();
    set_status("enter your login and password");
    hwnd = MainRegister(osInstance, "lab1", 50, 80, 440, 220);
    log_edit = CreateWindow("EDIT", "login", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, 20, 300, 25, hwnd, NULL, osInstance, NULL);
    pass_edit = CreateWindow("EDIT", "password", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 5, 60, 300, 25, hwnd, NULL, osInstance, NULL);
    new_pass_edit = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 5, 100, 300, 25, hwnd, NULL, osInstance, NULL);
    log_btn = CreateWindow("BUTTON", "enter", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 330, 20, 65, 65, hwnd, NULL, osInstance, NULL);
    add_btn = CreateWindow("BUTTON", "add", WS_CHILD | BS_PUSHBUTTON, 330, 20, 65, 65, hwnd, NULL, osInstance, NULL);
    change_btn = CreateWindow("BUTTON", "change", WS_CHILD | BS_PUSHBUTTON, 330, 20, 65, 65, hwnd, NULL, osInstance, NULL);
    rest_btn = CreateWindow("BUTTON", "apply", WS_CHILD | BS_PUSHBUTTON, 330, 20, 65, 65, hwnd, NULL, osInstance, NULL);
    block_btn = CreateWindow("BUTTON", "apply", WS_CHILD | BS_PUSHBUTTON, 330, 20, 65, 65, hwnd, NULL, osInstance, NULL);

    MSG msg;
    while(GetMessage(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}