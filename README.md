# epic1994clicker

Fast lightweight autoclicker written in C++ for Windows 10

Made in Visual Studio Code and compiled with MinGW

## Features:
 - Set trigger and output to most win32 virtual keycodes
 - Choose to activate by pressing or toggling
 - XInput controller support for activating
 - Esc to abort clicking, and again to minimize the window
 - Uniformly randomize click speed
 - Stop after X amount of clicks
 - Absolutely zero error handling
 - < 0.1% CPU and negligible RAM usage
 - Accurate click speed (100 ms = 10 cps instead of 8.2)

## Behavior:
 - Default trigger is F6, default output is LMB
 - If your mouse leaves a textbox you're editing, it'll stop the edit
 - Fastest cps is 1,000 due to the sleep function resolution (1 ms)
     - Higher speed can be reached with multiple instances of the app open
     - Slowest you can go is 1 click/2,147,483,647 ms (32-bit integer limit)
 - If variation > rate, then the click speed will change
     - EX: rate: 100 + variation: 150 = 50-200 ms (rate of 125)
 - Click and hold the background to drag the window
 - Window size is fixed, sorry 4K monitor users

## Compile settings:
 - g++ resource.res main.cpp -o epic1994clicker -Wall -O3 -flto -s -mwindows -lwinmm -lxinput -static
 - To make ^^^: windres resource.rc -O coff -o resource.res
 - To pack with UPX: upx --best epic1994clicker.exe

## Credits and Inspiration:
 - https://sourceforge.net/projects/orphamielautoclicker   // What I used all the time for years
 - https://github.com/git-eternal/autoclicker-tutorial     // Custom sleep function (ntDelayExecution)
 - https://sourceforge.net/projects/fastclicker            // Helped me understand how to write in C++
 - https://www.youtube.com/watch?v=-TkoO8Z07hI             // ^^^ + installing the necessary components

## Possible features:
 - Click at (x, y)
 - Keybind to change click speed
 - Set multiple trigger and output keybinds
 - Randomly offset mouse position every click
 - Set keybind to multiple keys (EX: Shift + E)