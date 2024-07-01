#undef UNICODE
#undef _UNICODE

//#include <iostream>    // Printing to terminal
#include "definition.h"  // App icon
#include "Windows.h"     // Win32 API
#include <xinput.h>      // Controller input
#include <chrono>  // Getting current time
#include <random>  // Variation
#include <thread>  // Handling sleep

// Start randomizer with a totally random seed
static std::mt19937 mersenne{489};

// Pull the delay execution function from a windows dll
static auto pNtDelayExecution = reinterpret_cast<NTSTATUS(NTAPI*)(BOOLEAN, PLARGE_INTEGER)>(
    reinterpret_cast<void*>(
        GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtDelayExecution")
    )
);

LARGE_INTEGER delayInterval{};

XINPUT_STATE controllerState;
WORD bitmask;

WNDCLASS windowClass;
HWND windowHandle;

HWND rateInput;
HWND variationInput;
HWND stopInput;

MSG msg;

bool active = true;        // If the program is currently running and processing messages
bool keyState = false;     // The state of the current trigger keybind
bool usingXInput = false;  // If the trigger button is from a controller
bool toggleSwitch = false; // Whether clicking is toggled on

char inputString[8];  // To retrieve input gui text

// IDs for input GUI's, so they can be used in the message callback switch statement
const int TOGGLE = 1;
const int TRIGGER = 2;
const int OUTPUT = 3;
const int INT_INPUT = 4;
const int ALWAYSONTOP = 5;

unsigned int state;   // 0: idle | 1: clicking | 2: setting trigger key | 3: setting output key
unsigned int rate = 100;  // How fast to click in milliseconds
unsigned int stopAfter = 0;  // How many times to click before stopping
unsigned int clicksLeft = 0;       // Tracks how many clicks are left
unsigned int variation = 0;        // How much to randomly deviate from click rate (ms)
unsigned int rateOffset = 0;       // Current deviation from regular speed
unsigned int outputKeybind = 1;    // What input to simulate
unsigned int triggerKeybind = 117; // What win32 keycode starts the autoclicker

const unsigned int keyArray[13]  {0x0A, 0x0E, 0x16, 0x3A, 0x88, 0x97, 0xD8, 0xEB, 0xF8,  0x07, 0x5E, 0xE0, 0xE8};
const unsigned int rangeArray[9] {0x0C, 0x10, 0x1B, 0x41, 0x90, 0xA0, 0xDB, 0xF6, 0xFB};  // ^^^: Singular reserved keycodes

// Check if a controller button is currently being held down
bool holdingTrigger() {
    ZeroMemory( &controllerState, sizeof(XINPUT_STATE));
    if (XInputGetState(0, &controllerState) == ERROR_SUCCESS) {
        return (bool)(controllerState.Gamepad.wButtons & triggerKeybind);
    }
    return false;
}

// Used after setting a keybind, clicking a checkbox, and to abort
void reset() {
    state = 0;
    SetFocus(windowHandle);
    SetWindowText(windowHandle, "epic1994clicker");
}

// Set click related variables and start clicking
void start() {
    state = 1;
    rateOffset = rate;
    clicksLeft = stopAfter;
    SetWindowText(windowHandle, "clicking...");
}

// Stop clicking and wait until key is released
void stop() {
    state = 0;
    toggleSwitch = false;
    SetWindowText(windowHandle, "epic1994clicker");
    if (usingXInput) {
        while (holdingTrigger()) Sleep(1);  // > 15 ms (computer quota length)
    } else {
        while (GetAsyncKeyState(triggerKeybind)) Sleep(1);
    }
}

// Attempt to find something that's being pressed and set it as a keybind
void setKeybind() {
    for (unsigned int key = 1; key < VK_ZOOM; key++) {
        // Skip win32 reserved, undefined, and unassigned keycodes
        for(int i = 0; i < 13; i++){
            if(keyArray[i] == key){
                if (i < 9) {  // Skip to end of range
                    key = rangeArray[i];
                } else {  // Add past the value
                    key ++;
                }
                break;
            }
        }

        GetAsyncKeyState(key);  // Has to update this way for some reason

        if (GetAsyncKeyState(key) && outputKeybind != key && triggerKeybind != key) {
            usingXInput = false;
            (state == 2)? triggerKeybind = key : outputKeybind = key;
            SetDlgItemText(windowHandle, state, std::to_string(key).c_str());
            reset();
            while (GetAsyncKeyState(key)) Sleep(1);
            return;
        }
    }

    ZeroMemory( &controllerState, sizeof(XINPUT_STATE));

    if (state == 2 && XInputGetState(0, &controllerState) == ERROR_SUCCESS) {
        bitmask = controllerState.Gamepad.wButtons;
        if (bitmask != 0) {
            usingXInput = true;

            // Iterate through all possible values in the bitmask to find what's being pressed
            for (unsigned int input = 1; input < 32769; input *= 2) {
                if (bitmask & input) {
                    triggerKeybind = input;
                    break;
                }
            }
            
            SetDlgItemText(windowHandle, 2, std::to_string(triggerKeybind).c_str());
            reset();
            while (holdingTrigger()) Sleep(1);
        }
    }
}

// Process input messages
LRESULT CALLBACK messageCallback(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case TOGGLE:
                    reset();  // This runs when it gets checked on startup
                    break;
                case TRIGGER:
                    //if (state < 2) {
                    state = 2;
                    SetWindowText(windowHandle, "send an input..");
                    //}
                    break;
                case OUTPUT:
                    state = 3;
                    SetWindowText(windowHandle, "send an input..");
                    break;
                case INT_INPUT:  // Perfectly readable and will definitely not break
                    GetWindowText((HWND)lParam, inputString, 9);
                    if ((HWND)lParam == rateInput) {
                        rate = atoi(inputString);
                    } else if ((HWND)lParam == stopInput)
                        stopAfter = atoi(inputString);
                    else if ((HWND)lParam == variationInput)
                        variation = atoi(inputString);
                    break;
                case ALWAYSONTOP:
                    SetWindowPos(windowHandle,
                                 (IsDlgButtonChecked(windowHandle, ALWAYSONTOP))
                                     ? HWND_TOPMOST
                                     : HWND_NOTOPMOST,
                                 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    reset();
                    break;
            }
            break;
        case WM_NCHITTEST: {  // When the mouse is on the background
            if (GetDlgCtrlID(GetFocus()) == INT_INPUT) SetFocus(windowHandle);
            LRESULT hit = DefWindowProc(hWindow, msg, wParam, lParam);
            if (hit == HTCLIENT) hit = HTCAPTION;
            return hit;
        }
        case WM_DESTROY:  // End message loop and quit application
            active = false;
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWindow, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE previous, LPSTR commandLine, int show) {

    // Set the window's properties
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = messageCallback;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    //windowClass.hbrBackground = CreateSolidBrush(RGB(225, 225, 225));  // Doesn't work with a terminal
    windowClass.lpszClassName = "_";

    RegisterClass(&windowClass);

    windowHandle = CreateWindow("_", "epic1994clicker", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,  // | WS_MINIMIZEBOX, // Disabled so you can read the name
        GetSystemMetrics(SM_CXSCREEN) / 2.17f, GetSystemMetrics(SM_CYSCREEN) / 2.6f, 150, 180, NULL, NULL, hInstance, NULL);

    // SetWindowLong(windowHandle, GWL_EXSTYLE, GetWindowLong(windowHandle,
    // GWL_EXSTYLE) | WS_EX_LAYERED); SetLayeredWindowAttributes(windowHandle,
    // RGB(225, 225, 225), 0, LWA_COLORKEY); // Code for a transparent window

    // Create the UI
    CreateWindow("Static", "",                              WS_VISIBLE | WS_CHILD, 5, 5, 135, 141, windowHandle, 0, 0, 0);  // Border
    CreateWindow("Static", " rate (ms)\n trigger\n output", WS_VISIBLE | WS_CHILD, 6, 10, 75, 50, windowHandle, 0, 0, 0);
    CreateWindow("Static", " variation\n stop after",       WS_VISIBLE | WS_CHILD, 6, 110, 75, 33, windowHandle, 0, 0, 0);

    CreateWindow("Button", "117",           WS_VISIBLE | WS_CHILD,                   79, 25, 61, 20, windowHandle, HMENU(TRIGGER), 0, 0);
    CreateWindow("Button", "1",             WS_VISIBLE | WS_CHILD,                   79, 45, 61, 20, windowHandle, HMENU(OUTPUT),  0, 0);
    CreateWindow("Button", "always on top", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 6, 86, 134, 18, windowHandle, HMENU(ALWAYSONTOP), 0, 0);

    rateInput =             CreateWindow("Edit", "100", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 79, 005, 61, 20, windowHandle, HMENU(INT_INPUT), 0, 0);
    variationInput =          CreateWindow("Edit", "0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 79, 107, 61, 20, windowHandle, HMENU(INT_INPUT), 0, 0);
    stopInput =               CreateWindow("Edit", "0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 79, 126, 61, 20, windowHandle, HMENU(INT_INPUT), 0, 0);

    HWND toggleCheckbox = CreateWindow("Button", "toggle", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 6, 66, 134, 20, windowHandle, HMENU(TOGGLE),  0, 0);
    SendMessage(toggleCheckbox, BM_CLICK, 0, 0);  // Defaults to toggle

    delayInterval.QuadPart = -5000LL; // 100 ns increment, 1M ns in 1.0 ms, 1M/100 = 10'000
    std::chrono::time_point<std::chrono::steady_clock> after, current, before;
    after = std::chrono::steady_clock::now();

    // For sending input
    INPUT mouse{};
    INPUT keyboard{};
    mouse.type = INPUT_MOUSE;
    keyboard.type = INPUT_KEYBOARD;

    ShowWindow(windowHandle, show);

    // Main message loop
    while (active) {
        
        if (GetAsyncKeyState(VK_ESCAPE)) {
            if (state == 0) {  // Minimize
                if (GetForegroundWindow() == windowHandle) ShowWindow(windowHandle, 6);
            } else {  // Abort
                reset();
                toggleSwitch = false;
                while (GetAsyncKeyState(VK_ESCAPE)) Sleep(1);
            }
        } else if (state == 0) {
            if (usingXInput) {
                if (holdingTrigger()) start();
            } else if (GetAsyncKeyState(triggerKeybind) != 0) {
                start();
            }
        } else if (state == 1) {
            before = std::chrono::steady_clock::now();

            timeBeginPeriod(1);
            pNtDelayExecution(false, &delayInterval);
            timeEndPeriod(1);

            keyState = (usingXInput)? holdingTrigger() : GetAsyncKeyState(triggerKeybind);

            if (!toggleSwitch && !keyState && IsDlgButtonChecked(windowHandle, TOGGLE)) {  // Flip the switch so the next time you press down it'll stop
                toggleSwitch = true;
            } else if (keyState + toggleSwitch == 1) {  // If it's only one, then try to click                
                current = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>((current - after) + (current - before)).count() >= rateOffset) {

                    if (stopAfter) {
                        clicksLeft --;
                        if (!clicksLeft) stop();
                    }

                    if (variation) {
                        std::uniform_int_distribution die{-variation, variation};
                        rateOffset = rate + die(mersenne);
                    }

                    switch (outputKeybind) {
                        case 1:
                            mouse.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
                            SendInput(1, &mouse, sizeof(INPUT));
                            break;
                        case 2:
                            mouse.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
                            SendInput(1, &mouse, sizeof(INPUT));
                            break;
                        case 4:
                            mouse.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP;
                            SendInput(1, &mouse, sizeof(INPUT));
                            break;
                        case 5: case 6:
                            mouse.mi.mouseData = outputKeybind - 4; // XBUTTON1 or XBUTTON2
                            mouse.mi.dwFlags = MOUSEEVENTF_XDOWN;
                            SendInput(1, &mouse, sizeof(INPUT));

                            mouse.mi.dwFlags = MOUSEEVENTF_XUP;
                            SendInput(1, &mouse, sizeof(INPUT));
                            mouse.mi.mouseData = 0;
                            break;
                        default:
                            keyboard.ki.wVk = outputKeybind;
                            SendInput(1, &keyboard, sizeof(INPUT));

                            keyboard.ki.dwFlags = KEYEVENTF_KEYUP;
                            SendInput(1, &keyboard, sizeof(INPUT));
                            keyboard.ki.dwFlags = 0;
                            break;
                    }
                    after = std::chrono::steady_clock::now();
                }
            } else {  // If no conditions to click or switch are met, then stop
                stop();
            }
        } else setKeybind();

        if (state != 1) Sleep(1);

        // Process pending input messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return 0;
}