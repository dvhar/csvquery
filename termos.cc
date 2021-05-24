#ifdef _WIN32
#include <windows.h>
void hideInput(){
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
}
long long totalram(){
	static long long mb = 0;
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
long long totalram(){
	static long long mb = 0;
	if (mb) return mb;
    long long pages = sysconf(_SC_PHYS_PAGES);
    long long page_size = sysconf(_SC_PAGE_SIZE);
	mb = (pages * page_size) / (1024 * 1024);
	return mb;
}
#endif
