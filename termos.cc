#ifdef __MINGW32__
#include <windows.h>
void hideInput(){
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
}
int totalram(){
	static long mb = 0;
	if (mb) return mb;
	MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    mb = status.ullTotalPhys / (1024 * 1024);
	return mb;
}

#else
#include <termios.h>
#include <unistd.h>
void hideInput(){
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}
int totalram(){
	static long mb = 0;
	if (mb) return mb;
    auto pages = sysconf(_SC_PHYS_PAGES);
    auto page_size = sysconf(_SC_PAGE_SIZE);
	mb = (pages * page_size) / (1024 * 1024);
	return mb;
}
#endif
