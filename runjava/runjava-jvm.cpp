// runjava.cpp : ����Ӧ�ó������ڵ㡣
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

	//����jvm.dllʧ����ֱ���˳�
	JavaVM *jvm; //ָ�� JVM ��ָ��,������Ҫʹ�����ָ������������ʼ�������� JVM��
	JNIEnv *env; //��ʾ JNI ִ�л���
	if(LoadJvm(path, &jvm, (void**)&env) == NULL) return -1;
	//jvm->AttachCurrentThread((void**)&env, NULL);
	//////

	jclass cls = env->FindClass("groovy/ui/GroovyMain");
	if(cls == 0) 
	{
		MessageBoxW(NULL, L"�Ҳ���groovy.ui.GroovyMain��.", L"���ش���", 0);
		return -1;
	}
	
	jmethodID mid = env->GetStaticMethodID(cls, "main", "([Ljava/lang/String;)V");
	if(mid == 0) 
	{
		MessageBoxW(NULL, L"�Ҳ���groovy.ui.GroovyMain.main������.", L"���ش���", 0);
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
			MessageBoxW(NULL, L"�Ҳ���JAVA_HOME����JRE_HOMEϵͳ����", L"ϵͳ��ʼ������", 0);
			return NULL;
		}
		wcscpy(lib_path, java_home);
		wcscat(lib_path, L"\\");
		wcscat(lib_path, JVM_DLL);
		hLib = LoadLibraryW(lib_path);
		if(hLib == NULL)
		{
			MessageBoxW(NULL, L"����jvm.dll����.", L"ϵͳ��ʼ������", 0);
			return NULL;
		}
	}
	My_JNI_CreateJavaVM = (JNI_CreateJavaVM_Proc)GetProcAddress(hLib, "JNI_CreateJavaVM");
	if(My_JNI_CreateJavaVM == NULL)
	{
		MessageBoxW(NULL, L"jvm.dll���Ҳ���JNI_CreateJavaVM��������.", L"���ش���", 0);
		return NULL;
	}

	//��ȡclasspath
	char classpath[4096];
	GetClassPath(path, classpath);
	//MessageBoxA(NULL, classpath, "ϵͳ", 0);

	JavaVMInitArgs vm_args; //����������ʼ�� JVM �ĸ��� JVM ����
	JavaVMOption options[1]; //�������� JVM �ĸ���ѡ������
	//options[0].optionString = "-Dfile.encoding=utf8";
	options[0].optionString = classpath;
	memset(&vm_args, 0, sizeof(vm_args));
	vm_args.version = JNI_VERSION_1_6;
	vm_args.nOptions = 1;
	vm_args.options = options;

	jint status = (*My_JNI_CreateJavaVM)(pjvm, penv, &vm_args);
	if(status == JNI_ERR)
	{
		MessageBoxW(NULL, L"����java���������ʧ��.", L"���ش���", 0);
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
		//ѭ����������ƥ���ļ�����ӵ���·���� -c
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

	//��Unicode�ַ���ת����ANSI�ַ���
	int wcp_len = WideCharToMultiByte(CP_ACP, 0, wcp, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, wcp, -1, classpath, wcp_len, NULL, NULL);
}

//����������ķ������滻��׼���������
//setStream(env,szStdoutFileName, "setOut")
//setStream(env,szStderrFileName, "setErr")
bool setStream(JNIEnv *env, const char* pszFileName, const char*pszMethod)

{
	int pBufferSize = 1024;
	char* pBuffer = new char[pBufferSize];
	//�����ַ�������
	jstring pathString =env->NewStringUTF(pszFileName);
	if (env->ExceptionCheck()== JNI_TRUE || pathString == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("�����ַ���ʧ�ܡ�");
		return false;
	}

	//����FileOutputStream�ࡣ
	jclass fileOutputStreamClass =env->FindClass("java/io/FileOutputStream");
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStreamClass == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����FileOutputStream��ʧ�ܡ�");
		return false;
	}

	//����FileOutputStream�๹�췽����
	jmethodID fileOutputStreamConstructor = env->GetMethodID(fileOutputStreamClass,"<init>", "(Ljava/lang/String;)V");
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStreamConstructor == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����FileOutputStream�๹�췽��ʧ�ܡ�");
		return false;
	}

	//����FileOutputStream��Ķ���
	jobject fileOutputStream = env->NewObject(fileOutputStreamClass,fileOutputStreamConstructor, pathString);
	if (env->ExceptionCheck()== JNI_TRUE || fileOutputStream == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����FileOutputStream��Ķ���ʧ�ܡ�");
		return false;
	}

	//����PrintStream�ࡣ
	jclass printStreamClass =env->FindClass("java/io/PrintStream");
	if (env->ExceptionCheck()== JNI_TRUE || printStreamClass == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����PrintStream��ʧ�ܡ�");
		return false;
	}

	//����PrintStream�๹�췽����
	jmethodID printStreamConstructor= env->GetMethodID(printStreamClass, "<init>","(Ljava/io/OutputStream;)V");
	if (env->ExceptionCheck()== JNI_TRUE || printStreamConstructor == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����PrintStream�๹�췽��ʧ�ܡ�");
		return false;
	}

	//����PrintStream��Ķ���
	jobject printStream =env->NewObject(printStreamClass, printStreamConstructor, fileOutputStream);
	if (env->ExceptionCheck()== JNI_TRUE || printStream == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����PrintStream��Ķ���ʧ�ܡ�");
		return false;
	}



	//����System�ࡣ
	jclass systemClass =env->FindClass("java/lang/System");
	if (env->ExceptionCheck()== JNI_TRUE || systemClass == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf( "����System��ʧ�ܡ�");
		return false;
	}

	//����System�����÷�����
	jmethodID setStreamMethod =env->GetStaticMethodID(systemClass, pszMethod,"(Ljava/io/PrintStream;)V");
	if (env->ExceptionCheck()== JNI_TRUE || setStreamMethod == NULL)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����System�����÷���ʧ�ܡ�");
		return false;
	}

	//����System�������
	env->CallStaticVoidMethod(systemClass,setStreamMethod, printStream);
	if (env->ExceptionCheck()== JNI_TRUE)
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		printf("����System�����ʧ�ܡ�");
		return false;
	}
}


