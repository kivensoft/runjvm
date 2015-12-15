// runjava.cpp : ����Ӧ�ó������ڵ㡣
//


#include <windows.h>
#include <shlwapi.h>
#include <string.h>
#include "resource.h"
#include "runjava.h"
#include "jni.h"

#define WSIZE_OF(x) (sizeof(x) << 1)
const wchar_t *JDK_JVM_DLL = L"jre\\bin\\client\\jvm.dll";
const wchar_t *JRE_JVM_DLL = L"bin\\client\\jvm.dll";
const wchar_t *JDK_SERVER_JVM_DLL = L"jre\\bin\\server\\jvm.dll";
const wchar_t *JRE_SERVER_JVM_DLL = L"bin\\server\\jvm.dll";
wchar_t progname[128];

BOOL FileExists(LPCWSTR file);
void MsgBox(LPCWSTR fmt, ...);
void GetModuleFilePath(LPWSTR path, DWORD count);
void InitProgName();
DWORD GetCmdLine(LPWSTR cmdline, DWORD size);
LPWSTR NextArg(LPWSTR cmdline, LPSTR arg, size_t size);
int GetArgCount(LPWSTR cmdline);
void GetClassPath(LPCWSTR path, LPSTR classpath);
HMODULE LoadJvm(JavaVM **pjvm, void **penv);
LPWSTR mwcscpy(LPWSTR dest, ...);
void debug(LPCWSTR fmt, ...);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
//int main(void) {
	InitProgName();
	//����jvm.dllʧ����ֱ���˳�
	JavaVM *jvm; //ָ�� JVM ��ָ��,������Ҫʹ�����ָ������������ʼ�������� JVM��
	JNIEnv *env; //��ʾ JNI ִ�л���
	if(LoadJvm(&jvm, (void**)&env) == NULL) return -1;

	//���������в���
	wchar_t cmdline[4096], *arglist = cmdline;
	GetCmdLine(cmdline, 4096);

	//��һ��������Ҫִ�е�java��
	char mainclass[256];
	arglist = NextArg(arglist, mainclass, 256);
	jclass cls = env->FindClass(mainclass);//"groovy/ui/GroovyMain");
	if(cls == 0) {
		MsgBox(L"�Ҳ�������.���������ļ�.");
		return -1;
	}
	
	jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
	if(mid == 0) {
		MsgBox(L"�Ҳ���groovy.ui.GroovyMain.main������.");
		return -1;
	}

	int argCount = GetArgCount(arglist);
	char arg[512];
	jobjectArray args = env->NewObjectArray(argCount, env->FindClass("java/lang/String"), NULL);
	for(int i = 0; i < argCount; i++) {
		arglist = NextArg(arglist, arg, 512);
		env->SetObjectArrayElement(args, i, env->NewStringUTF(arg));
	}
	env->CallStaticVoidMethod(cls, mid, args);
	jvm->DestroyJavaVM();
	return 0;
}

void InitProgName() {
	wchar_t path[1024];
	DWORD size = GetModuleFileName(NULL, path, WSIZE_OF(path));
	path[WSIZE_OF(path) - 1] = 0;
	LPWSTR pos = wcsrchr(path, L'\\');
	if(pos) wcscpy_s(progname, ++pos);
	else wcscpy_s(progname, WSIZE_OF(progname), path);
	pos = wcsrchr(progname, L'.');
	if(pos) *pos = 0;
}

DWORD GetCmdLine(LPWSTR cmdline, DWORD size) {
	wchar_t ini[1024];
	DWORD iniLen = GetModuleFileName(NULL, ini, 1024);
	wcscpy_s(ini + iniLen - 3, WSIZE_OF(ini) - iniLen + 3, L"ini");
	return GetPrivateProfileString(L"setting", L"cmd", L"Main", cmdline, size, ini);
}

int GetArgCount(LPWSTR cmdline) {
	int count = 0;
	while(*cmdline) {
		while(*cmdline == L' ') cmdline++;
		if(*cmdline) count++;
		wchar_t endchar = L' ';
		if(*cmdline == L'"' || *cmdline == L'\'') endchar = *cmdline++;
		while(*cmdline && *cmdline != endchar) cmdline++;
		if(*cmdline) cmdline++;
	}
	return count;
}

LPWSTR NextArg(LPWSTR cmdline, LPSTR arg, size_t size) {
	while(*cmdline == L' ') cmdline++;
	wchar_t endchar = L' ';
	if(*cmdline == L'"' || *cmdline == L'\'') endchar = *cmdline++;
	wchar_t *p = cmdline;
	while(*p && *p != endchar) p++;
	*p = 0;

	if(*cmdline) {
		//ת��UTF8
		size_t len = WideCharToMultiByte(CP_UTF8, 0, cmdline, -1, NULL, 0, NULL, NULL);
		if(len > size) len = size;
		WideCharToMultiByte(CP_UTF8, 0, cmdline, -1, arg, (int)len, NULL, NULL);
	}

	return (*cmdline) ? p + 1 : NULL;
}

void GetModuleFilePath(LPWSTR path, DWORD count) {
	DWORD size = GetModuleFileName(NULL, path, count);
	path[count - 1] = 0;
	LPWSTR pos = wcsrchr(path, L'\\');
	if(pos) *(pos + 1) = 0;
}

BOOL FileExists(LPCWSTR file) {
	WIN32_FIND_DATAW find_data;
	HANDLE h_find = FindFirstFile(file, &find_data);
	if(h_find != INVALID_HANDLE_VALUE) {
		FindClose(h_find);
		return TRUE;
	}
	return FALSE;
}

void MsgBox(LPCWSTR fmt, ...) {
	wchar_t buf[8192];
	va_list argp;
	va_start(argp, fmt);
	wvsprintfW(buf, fmt, argp);
	va_end(argp);
	MessageBox(NULL, buf, progname, 0);
}

HMODULE LoadJvm(JavaVM **pjvm, void **penv) {
	typedef jint (JNICALL *JNI_CreateJavaVM_Proc) (JavaVM **, void **, void *);
	JNI_CreateJavaVM_Proc My_JNI_CreateJavaVM;

	wchar_t lib_path[1024];
	GetModuleFilePath(lib_path, WSIZE_OF(lib_path)); //��ȡӦ�ó���·��
	char classpath[4096];
	GetClassPath(lib_path, classpath); //��ȡclasspath��Ӧ�ó���·��lib������jar

	//����ʹ��Ӧ�ó���Ŀ¼�µ�jre������еĻ�
	wcscat_s(lib_path, JDK_JVM_DLL);
	if(!FileExists(lib_path)) {
		bool jdk_home = true;
		//��ȡjava��������
		DWORD javalen = GetEnvironmentVariable(L"JAVA_HOME", lib_path, WSIZE_OF(lib_path));
		if(!javalen) javalen = GetEnvironmentVariable(L"JDK_HOME", lib_path, WSIZE_OF(lib_path));
		if(!javalen) {
			javalen = GetEnvironmentVariable(L"JRE_HOME", lib_path, WSIZE_OF(lib_path));
			jdk_home = false;
		}
		if(!javalen) {
			MsgBox(L"�Ҳ���java�������밲װjava��������JAVA_HOME����������");
			return NULL;
		}
		if(lib_path[javalen - 1] != L'\\')
			lib_path[javalen++] = L'\\';
		wcscpy_s(lib_path + javalen, WSIZE_OF(lib_path) - javalen, jdk_home ? JDK_JVM_DLL : JRE_JVM_DLL);
	}
	if(!FileExists(lib_path)){
		GetModuleFilePath(lib_path, WSIZE_OF(lib_path)); //��ȡӦ�ó���·��
		GetClassPath(lib_path, classpath); //��ȡclasspath��Ӧ�ó���·��lib������jar

		//����ʹ��Ӧ�ó���Ŀ¼�µ�jre������еĻ�
		wcscat_s(lib_path, JDK_SERVER_JVM_DLL);
		if(!FileExists(lib_path)) {
			bool jdk_home = true;
			//��ȡjava��������
			DWORD javalen = GetEnvironmentVariable(L"JAVA_HOME", lib_path, WSIZE_OF(lib_path));
			if(!javalen) javalen = GetEnvironmentVariable(L"JDK_HOME", lib_path, WSIZE_OF(lib_path));
			if(!javalen) {
				javalen = GetEnvironmentVariable(L"JRE_HOME", lib_path, WSIZE_OF(lib_path));
				jdk_home = false;
			}
			if(!javalen) {
				MsgBox(L"�Ҳ���java�������밲װjava��������JAVA_HOME����������");
				return NULL;
			}
			if(lib_path[javalen - 1] != L'\\')
				lib_path[javalen++] = L'\\';
			wcscpy_s(lib_path + javalen, WSIZE_OF(lib_path) - javalen, jdk_home ? JDK_SERVER_JVM_DLL : JRE_SERVER_JVM_DLL);
		}
	}
	if(!FileExists(lib_path)) {
		wcscat_s(lib_path, L"������.");
		MsgBox(lib_path);
	}

	//�����ҵ���jvm.dll����װ��JNI_CreateJavaVM������ַ
	HMODULE hLib = LoadLibrary(lib_path);
	if(hLib == NULL) {
		MsgBox(L"����jvm.dll����.");
		return NULL;
	}
	My_JNI_CreateJavaVM = (JNI_CreateJavaVM_Proc)GetProcAddress(hLib, "JNI_CreateJavaVM");
	if(My_JNI_CreateJavaVM == NULL) {
		MsgBox(L"jvm.dll���Ҳ���JNI_CreateJavaVM��������.");
		return NULL;
	}

	//��ʼ��JavaVM����
	JavaVMInitArgs vm_args; //����������ʼ�� JVM �ĸ��� JVM ����
	JavaVMOption options[1]; //�������� JVM �ĸ���ѡ������
	//options[0].optionString = "-Dfile.encoding=utf8";
	options[0].optionString = classpath;
	memset(&vm_args, 0, sizeof(vm_args));
	vm_args.version = JNI_VERSION_1_6;
	vm_args.nOptions = 1;
	vm_args.options = options;

	jint status = (*My_JNI_CreateJavaVM)(pjvm, penv, &vm_args);
	if(status == JNI_ERR) {
		MsgBox(L"����java���������ʧ��.");
		return NULL;
	}

	return hLib;
}

void GetClassPath(LPCWSTR path, LPSTR classpath) {
	wchar_t wcp[4096], *pwcp = wcp;
	wcscpy_s(pwcp, WSIZE_OF(wcp), L"-Djava.class.path=.");
	pwcp += 19;
	wchar_t find_path[1024];
	wcscpy_s(find_path, WSIZE_OF(find_path), path);
	wcscat_s(find_path, WSIZE_OF(find_path) - wcslen(find_path), L"lib\\*.jar");
	WIN32_FIND_DATAW find_data;
	HANDLE h_find = FindFirstFileW(find_path, &find_data);
	if(h_find != INVALID_HANDLE_VALUE) {
		//ѭ����������ƥ���ļ�����ӵ���·���� -c
		for(BOOL r = TRUE; r; r = FindNextFileW(h_find, &find_data)) {
			//wprintf(L"%s\n", find_data.cFileName);
			wcscpy_s(pwcp, WSIZE_OF(wcp) - wcslen(wcp), L";lib\\");
			pwcp += 5;
			wcscpy_s(pwcp, WSIZE_OF(wcp) - wcslen(wcp), find_data.cFileName);
			pwcp += wcslen(find_data.cFileName);
		}
		FindClose(h_find);
	}
	*pwcp = 0;

	//��Unicode�ַ���ת����ANSI�ַ���
	int wcp_len = WideCharToMultiByte(CP_ACP, 0, wcp, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, wcp, -1, classpath, wcp_len, NULL, NULL);
}

//����������ķ������滻��׼���������
//setStream(env,szStdoutFileName, "setOut")
//setStream(env,szStderrFileName, "setErr")
bool setStream(JNIEnv *env, const char* pszFileName, const char*pszMethod) {
	int pBufferSize = 1024;
	char* pBuffer = new char[pBufferSize];
	//�����ַ�������
	jstring pathString =env->NewStringUTF(pszFileName);
	if (env->ExceptionCheck()== JNI_TRUE || pathString == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("�����ַ���ʧ�ܡ�");
		return false;
	}

	//����FileOutputStream�ࡣ
	jclass fileOutputStreamClass =env->FindClass("java/io/FileOutputStream");
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStreamClass == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����FileOutputStream��ʧ�ܡ�");
		return false;
	}

	//����FileOutputStream�๹�췽����
	jmethodID fileOutputStreamConstructor = env->GetMethodID(fileOutputStreamClass,"<init>", "(Ljava/lang/String;)V");
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStreamConstructor == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����FileOutputStream�๹�췽��ʧ�ܡ�");
		return false;
	}

	//����FileOutputStream��Ķ���
	jobject fileOutputStream = env->NewObject(fileOutputStreamClass,fileOutputStreamConstructor, pathString);
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStream == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����FileOutputStream��Ķ���ʧ�ܡ�");
		return false;
	}

	//����PrintStream�ࡣ
	jclass printStreamClass =env->FindClass("java/io/PrintStream");
	if (env->ExceptionCheck()== JNI_TRUE || printStreamClass == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����PrintStream��ʧ�ܡ�");
		return false;
	}

	//����PrintStream�๹�췽����
	jmethodID printStreamConstructor= env->GetMethodID(printStreamClass, "<init>","(Ljava/io/OutputStream;)V");
	if (env->ExceptionCheck()== JNI_TRUE || printStreamConstructor == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����PrintStream�๹�췽��ʧ�ܡ�");
		return false;
	}

	//����PrintStream��Ķ���
	jobject printStream =env->NewObject(printStreamClass, printStreamConstructor, fileOutputStream);
	if (env->ExceptionCheck()== JNI_TRUE || printStream == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����PrintStream��Ķ���ʧ�ܡ�");
		return false;
	}

	//����System�ࡣ
	jclass systemClass =env->FindClass("java/lang/System");
	if (env->ExceptionCheck()== JNI_TRUE || systemClass == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf( "����System��ʧ�ܡ�");
		return false;
	}

	//����System�����÷�����
	jmethodID setStreamMethod =env->GetStaticMethodID(systemClass, pszMethod,"(Ljava/io/PrintStream;)V");
	if (env->ExceptionCheck()== JNI_TRUE || setStreamMethod == NULL) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����System�����÷���ʧ�ܡ�");
		return false;
	}

	//����System�������
	env->CallStaticVoidMethod(systemClass,setStreamMethod, printStream);
	if (env->ExceptionCheck()== JNI_TRUE) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����System�����ʧ�ܡ�");
		return false;
	}
}

LPWSTR mwcscpy(LPWSTR dest, ...) {
	LPWSTR p = dest;
	va_list argp;
	LPWSTR arg;
	va_start(argp, dest);
	arg = va_arg(argp, LPWSTR);
	while(arg)
		while(*p++ = *arg++);
	return dest;
}

void debug(LPCWSTR fmt, ...) {
	wchar_t buf[8192];
	va_list argp;
	va_start(argp, fmt);
	wvsprintfW(buf, fmt, argp);
	va_end(argp);
	MessageBox(NULL, buf, L"������Ϣ", 0);
}
