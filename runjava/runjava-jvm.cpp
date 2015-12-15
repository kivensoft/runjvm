// runjava.cpp : 定义应用程序的入口点。
//


#include <SDKDDKVer.h>
#include <Windows.h>
//#include <tchar.h>
#include <string.h>
//#include <memory.h>
#include "resource.h"
#include "jni.h"

const wchar_t *JVM_DLL = L"jre\\bin\\client\\jvm.dll";

void GetModuleFilePath(LPWSTR path, DWORD count);
void GetClassPath(LPCWSTR path, LPSTR classpath);
HMODULE LoadJvm(LPCWSTR path, JavaVM **pjvm, void **penv);

//int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
int main(void)
{
	wchar_t path[1024];
	GetModuleFilePath(path, 1023);

	//加载jvm.dll失败则直接退出
	JavaVM *jvm; //指向 JVM 的指针,我们主要使用这个指针来创建、初始化和销毁 JVM。
	JNIEnv *env; //表示 JNI 执行环境
	if(LoadJvm(path, &jvm, (void**)&env) == NULL) return -1;
	//jvm->AttachCurrentThread((void**)&env, NULL);
	//////

	jclass cls = env->FindClass("groovy/ui/GroovyMain");
	if(cls == 0) 
	{
		MessageBoxW(NULL, L"找不到groovy.ui.GroovyMain类.", L"加载错误", 0);
		return -1;
	}
	
	jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
	if(mid == 0) 
	{
		MessageBoxW(NULL, L"找不到groovy.ui.GroovyMain.main主函数.", L"加载错误", 0);
		return -1;
	}

	jobjectArray args = env->NewObjectArray(3, env->FindClass("java/lang/String"), NULL);
	env->SetObjectArrayElement(args, 0, env->NewStringUTF("-c"));
	env->SetObjectArrayElement(args, 1, env->NewStringUTF("utf8"));
	env->SetObjectArrayElement(args, 2, env->NewStringUTF("main.groovy"));
	env->CallStaticVoidMethod(cls, mid, args);
	//jvm->DestroyJavaVM();
	Sleep(INFINITE);
	wprintf(L"END\n");
	return 0;
}

void GetModuleFilePath(LPWSTR path, DWORD count)
{
	path[count - 1] = 0;
	DWORD size = GetModuleFileNameW(NULL, path, count - 1);
	LPWSTR pos = wcsrchr(path, L'\\');
	if(pos) *(pos + 1) = 0;
}

HMODULE LoadJvm(LPCWSTR path, JavaVM **pjvm, void **penv)
{
	typedef jint (JNICALL *JNI_CreateJavaVM_Proc) (JavaVM **, void **, void *);
	JNI_CreateJavaVM_Proc My_JNI_CreateJavaVM;

	wchar_t lib_path[1024];
	wcscpy(lib_path, path);
	wcscat(lib_path, JVM_DLL);
	HMODULE hLib = LoadLibraryW(lib_path);
	if(hLib == NULL)
	{
		wchar_t *java_home = _wgetenv(L"JAVA_HOME");
		if(java_home == NULL)
			_wgetenv(L"JRE_HOME");
		if(java_home == NULL)
		{
			MessageBoxW(NULL, L"找不到JAVA_HOME或者JRE_HOME系统变量", L"系统初始化错误", 0);
			return NULL;
		}
		wcscpy(lib_path, java_home);
		wcscat(lib_path, L"\\");
		wcscat(lib_path, JVM_DLL);
		hLib = LoadLibraryW(lib_path);
		if(hLib == NULL)
		{
			MessageBoxW(NULL, L"加载jvm.dll错误.", L"系统初始化错误", 0);
			return NULL;
		}
	}
	My_JNI_CreateJavaVM = (JNI_CreateJavaVM_Proc)GetProcAddress(hLib, "JNI_CreateJavaVM");
	if(My_JNI_CreateJavaVM == NULL)
	{
		MessageBoxW(NULL, L"jvm.dll中找不到JNI_CreateJavaVM导出函数.", L"加载错误", 0);
		return NULL;
	}

	//获取classpath
	char classpath[4096];
	GetClassPath(path, classpath);
	//MessageBoxA(NULL, classpath, "系统", 0);

	JavaVMInitArgs vm_args; //可以用来初始化 JVM 的各种 JVM 参数
	JavaVMOption options[1]; //具有用于 JVM 的各种选项设置
	//options[0].optionString = "-Dfile.encoding=utf8";
	options[0].optionString = classpath;
	memset(&vm_args, 0, sizeof(vm_args));
	vm_args.version = JNI_VERSION_1_6;
	vm_args.nOptions = 1;
	vm_args.options = options;

	jint status = (*My_JNI_CreateJavaVM)(pjvm, penv, &vm_args);
	if(status == JNI_ERR)
	{
		MessageBoxW(NULL, L"创建java虚拟机环境失败.", L"加载错误", 0);
		return NULL;
	}

	return hLib;
}

void GetClassPath(LPCWSTR path, LPSTR classpath)
{
	wchar_t wcp[4096], *pwcp = wcp;
	wcscpy(pwcp, L"-Djava.class.path=.");
	pwcp += 19;
	wchar_t find_path[1024];
	wcscpy(find_path, path);
	wcscat(find_path, L"lib\\*.jar");
	WIN32_FIND_DATAW find_data;
	HANDLE h_find = FindFirstFileW(find_path, &find_data);
	if(h_find != INVALID_HANDLE_VALUE)
	{
		//循环查找所有匹配文件并添加到类路径中 -c
		for(BOOL r = TRUE; r; r = FindNextFileW(h_find, &find_data))
		{
			//wprintf(L"%s\n", find_data.cFileName);
			wcscpy(pwcp, L";lib\\");
			pwcp += 5;
			wcscpy(pwcp, find_data.cFileName);
			pwcp += wcslen(find_data.cFileName);
		}
		FindClose(h_find);
	}
	*pwcp = 0;

	//把Unicode字符串转换成ANSI字符串
	int wcp_len = WideCharToMultiByte(CP_ACP, 0, wcp, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, wcp, -1, classpath, wcp_len, NULL, NULL);
}

//设置输出流的方法，替换标准输入输出流
//setStream(env,szStdoutFileName, "setOut")
//setStream(env,szStderrFileName, "setErr")
bool setStream(JNIEnv *env, const char* pszFileName, const char*pszMethod)

{
	int pBufferSize = 1024;
	char* pBuffer = new char[pBufferSize];
	//创建字符串对象。
	jstring pathString =env->NewStringUTF(pszFileName);
	if (env->ExceptionCheck()== JNI_TRUE || pathString == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("创建字符串失败。");
		return false;
	}

	//查找FileOutputStream类。
	jclass fileOutputStreamClass =env->FindClass("java/io/FileOutputStream");
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStreamClass == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("查找FileOutputStream类失败。");
		return false;
	}

	//查找FileOutputStream类构造方法。
	jmethodID fileOutputStreamConstructor = env->GetMethodID(fileOutputStreamClass,"<init>", "(Ljava/lang/String;)V");
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStreamConstructor == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("查找FileOutputStream类构造方法失败。");
		return false;
	}

	//创建FileOutputStream类的对象。
	jobject fileOutputStream = env->NewObject(fileOutputStreamClass,fileOutputStreamConstructor, pathString);
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStream == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("创建FileOutputStream类的对象失败。");
		return false;
	}

	//查找PrintStream类。
	jclass printStreamClass =env->FindClass("java/io/PrintStream");
	if (env->ExceptionCheck()== JNI_TRUE || printStreamClass == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("查找PrintStream类失败。");
		return false;
	}

	//查找PrintStream类构造方法。
	jmethodID printStreamConstructor= env->GetMethodID(printStreamClass, "<init>","(Ljava/io/OutputStream;)V");
	if (env->ExceptionCheck()== JNI_TRUE || printStreamConstructor == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("查找PrintStream类构造方法失败。");
		return false;
	}

	//创建PrintStream类的对象。
	jobject printStream =env->NewObject(printStreamClass, printStreamConstructor, fileOutputStream);
	if (env->ExceptionCheck()== JNI_TRUE || printStream == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("创建PrintStream类的对象失败。");
		return false;
	}



	//查找System类。
	jclass systemClass =env->FindClass("java/lang/System");
	if (env->ExceptionCheck()== JNI_TRUE || systemClass == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf( "查找System类失败。");
		return false;
	}

	//查找System类设置方法。
	jmethodID setStreamMethod =env->GetStaticMethodID(systemClass, pszMethod,"(Ljava/io/PrintStream;)V");
	if (env->ExceptionCheck()== JNI_TRUE || setStreamMethod == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("查找System类设置方法失败。");
		return false;
	}

	//设置System类的流。
	env->CallStaticVoidMethod(systemClass,setStreamMethod, printStream);
	if (env->ExceptionCheck()== JNI_TRUE)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("设置System类的流失败。");
		return false;
	}
}


