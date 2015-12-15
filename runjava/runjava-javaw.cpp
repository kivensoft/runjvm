// runjava.cpp : 定义应用程序的入口点。
//


#include <SDKDDKVer.h>
#include <Windows.h>
#include <string.h>
#include "resource.h"

const LPWSTR JAVAW_EXE = L"jre\\bin\\javaw.exe";
const LPWSTR CONFIG = L"config";
const LPWSTR MAINCLASS = L"mainclass";
const LPWSTR MAINCLASS_DEF = L"Main";
const LPWSTR DBG = L"debug";
const LPWSTR DBG_DEF = L"no";

void GetModuleFilePath(LPWSTR path, DWORD count);
void GetClassPath(LPCWSTR path, LPWSTR classpath);
BOOL FileExists(LPCWSTR file);
void debug(LPCWSTR fmt, ...);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
//int main(void) {
	//得到当前可执行文件名
	//wchar_t exe_name[512];
	//DWORD exelen = GetModuleFileName(NULL, exe_name, 512);

	//读取同名ini文件配置
	//TCHAR ini_name[512], jar_name[128], dbg[32];
	//wcscpy(wcscpy(ini_name, exe_name) + exelen - 3, L"ini");
	//DWORD jarlen = GetPrivateProfileString(CONFIG, JAR, _T(""), jar_name, STR_SIZE(jar_name), ini_name);
	//GetPrivateProfileString(CONFIG, DBG, DBG_DEF, dbg, STR_SIZE(dbg), ini_name);

	//ini不存在，新建一个，并结束程序
	/*
	if(!jarlen) {
		WritePrivateProfileString(CONFIG, JAR, _T("simple.jar"), ini_name);
		WritePrivateProfileString(CONFIG, DBG, DBG_DEF, ini_name);
		TCHAR msg[1024];
		_stprintf(msg, _T("请先配置%s中的jar名字"), ini_name);
		MessageBox(NULL, msg, _T("错误"), MB_OK);
		return -1;
	}
	*/

	//获取当前路径
	//DWORD pathlen = exelen - 1;
	TCHAR exe_path[512];
	GetModuleFilePath(exe_path, 512);

	//优先判断应用程序目录下有没有jre目录，如果有，则优先加载
	wchar_t java_exe[1024];
	wcscpy(java_exe, exe_path);
	wcscat(java_exe, JAVAW_EXE);
	if(!FileExists(java_exe)) {
		//获取java环境变量
		DWORD javalen = GetEnvironmentVariable(L"JAVA_HOME", java_exe, 1024);
		if(!javalen) javalen = GetEnvironmentVariable(L"JRE_HOME", java_exe, 1024);
		if(!javalen) javalen = GetEnvironmentVariable(L"JDK_HOME", java_exe, 1024);
		if(!javalen) {
			MessageBoxW(NULL, L"找不到java环境，请安装java，并设置JAVA_HOME环境变量。", L"加载错误", 0);
			return -1;
		}
		if(java_exe[javalen - 1] != L'\\')
			java_exe[javalen++] = L'\\';
		wcscpy(java_exe + javalen, JAVAW_EXE);
	}
	if(!FileExists(java_exe)) {
		wcscat(java_exe, L"不存在.");
		MessageBoxW(NULL, java_exe, L"加载错误", 0);
	}

	//生成可执行的命令行
	wchar_t cmd_line[8192];
	cmd_line[0] = L'"';
	wcscpy(cmd_line + 1, java_exe);
	wcscat(cmd_line, L"\" ");
	wchar_t classpath[4096];
	GetClassPath(exe_path, classpath);
	wcscat(cmd_line, classpath);
	wcscat(cmd_line, L" groovy.ui.GroovyMain -c utf-8 main.groovy");
	debug(L"%s", cmd_line);
	//return 0;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	::CreateProcess(NULL, cmd_line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	return 0;
}

void GetModuleFilePath(LPWSTR path, DWORD count) {
	path[count - 1] = 0;
	DWORD size = GetModuleFileNameW(NULL, path, count);
	LPWSTR pos = wcsrchr(path, L'\\');
	if(pos) *(pos + 1) = 0;
}

void GetClassPath(LPCWSTR path, LPWSTR classpath) {
	wchar_t *pwcp = classpath;
	wcscpy(pwcp, L"-cp .");
	pwcp += 5;
	wchar_t find_path[1024];
	wcscpy(find_path, path);
	wcscat(find_path, L"lib\\*.jar");
	WIN32_FIND_DATAW find_data;
	HANDLE h_find = FindFirstFileW(find_path, &find_data);
	if(h_find != INVALID_HANDLE_VALUE) {
		//循环查找所有匹配文件并添加到类路径中 -c
		for(BOOL r = TRUE; r; r = FindNextFileW(h_find, &find_data))
		{
			//wprintf(L"%s\n", find_data.cFileName);
			wcscpy(pwcp, L";\"lib\\");
			pwcp += 6;
			wcscpy(pwcp, find_data.cFileName);
			pwcp += wcslen(find_data.cFileName);
			*(pwcp++) = '"';
		}
		FindClose(h_find);
	}
	*pwcp = 0;

	//把Unicode字符串转换成ANSI字符串
	//int wcp_len = WideCharToMultiByte(CP_ACP, 0, wcp, -1, NULL, 0, NULL, NULL);
	//WideCharToMultiByte(CP_ACP, 0, wcp, -1, classpath, wcp_len, NULL, NULL);
}

BOOL FileExists(LPCWSTR file) {
	WIN32_FIND_DATAW find_data;
	HANDLE h_find = FindFirstFileW(file, &find_data);
	if(h_find != INVALID_HANDLE_VALUE) {
		FindClose(h_find);
		return TRUE;
	}
	return FALSE;
}

void debug(LPCWSTR fmt, ...) {
	wchar_t buf[8192];
	va_list argp;
	va_start(argp, fmt);
	wvsprintfW(buf, fmt, argp);
	va_end(argp);
	MessageBoxW(NULL, buf, L"调试信息", 0);
}