// pluginhost.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "proxy.h"

struct hook
{
	BYTE* proc;
	BYTE* original_proc;

	hook(const TCHAR* module_name, const char* proc_name, void* new_proc)
	{
		HINSTANCE hModule = GetModuleHandle(module_name);
		proc = (BYTE*)GetProcAddress(hModule, proc_name);
		if (proc != NULL) {
			std::cout << "hook \"" << proc_name << "\" succeeded" << std::endl;
			original_proc = (BYTE*)VirtualAlloc(NULL, 10, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			memcpy(original_proc, proc, 5);
			original_proc[5] = 0xe9;
			*(unsigned*)(original_proc + 6) = (unsigned)(proc + 5) - (unsigned)(original_proc + 10);

			DWORD dwOldProtect;
			VirtualProtect(proc, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
			proc[0] = 0xe9;
			*(unsigned*)(proc + 1) = (unsigned)new_proc - (unsigned)(proc + 5);
		}
	}
	~hook()
	{
		memcpy(proc, original_proc, 5);
		VirtualFree(original_proc, 0, MEM_RELEASE);
	}
};

namespace falcon {
	bool post_message_then_wait(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		DWORD tid = GetWindowThreadProcessId(hwnd, NULL);
		safe_handle<HANDLE> thread(OpenThread(SYNCHRONIZE, FALSE, tid));
		if (!thread) {
			return false;
		}
		HANDLE rghs[] = { thread };
		MSG msg;
		while (true) {
			while (PeekMessage(&msg, (HWND)wParam, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_RESPONSE) {
					return true;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (WaitForSingleObject(thread, 0) == WAIT_OBJECT_0) {
				return false;
			}

			if (MsgWaitForMultipleObjectsEx(1, rghs, INFINITE, QS_ALLPOSTMESSAGE, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0) {
				return false;
			}
		}
		return true;
	}
}

int main()
{
	falcon::channel_data channel;
	falcon::id id;
	auto p = falcon::meta<IUnknown>::make_proxy(&channel, id);
	BOOL b;
	falcon::handler<HRESULT(__stdcall*)(void*, IID const*, void * *)>::handle(NULL, 0, NULL, NULL, b);
	HRESULT hr;
	CComPtr<ICLRMetaHost> pMetaHost;
	
	hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&pMetaHost));
	if (FAILED(hr))	{
		
	}

	CComPtr<ICLRRuntimeInfo> pRuntimeInfo;

	// Get the ICLRRuntimeInfo corresponding to a particular CLR version. It  
	// supersedes CorBindToRuntimeEx with STARTUP_LOADER_SAFEMODE. 
	hr = pMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&pRuntimeInfo));
	if (FAILED(hr)){
	}


	// Check if the specified runtime can be loaded into the process. This  
	// method will take into account other runtimes that may already be  
	// loaded into the process and set pbLoadable to TRUE if this runtime can  
	// be loaded in an in-process side-by-side fashion.  
	BOOL fLoadable;
	hr = pRuntimeInfo->IsLoadable(&fLoadable);
	if (FAILED(hr)){
	}
	// Load the CLR into the current process and return a runtime interface  
	// pointer. ICorRuntimeHost and ICLRRuntimeHost are the two CLR hosting   
	// interfaces supported by CLR 4.0. Here we demo the ICorRuntimeHost  
	// interface that was provided in .NET v1.x, and is compatible with all  
	// .NET Frameworks.  
	CComPtr<ICorRuntimeHost> pRuntimeHost;
	hr = pRuntimeInfo->GetInterface(CLSID_CorRuntimeHost,
		IID_PPV_ARGS(&pRuntimeHost));
	if (FAILED(hr)){
	}

	// Start the CLR. 
	hr = pRuntimeHost->Start();
	if (FAILED(hr)) {
	}

	
	CComPtr<IUnknown> pDomain;
	hr = pRuntimeHost->CreateDomainSetup(&pDomain);
	if (FAILED(hr)) {
	}
	CComQIPtr<mscorlib::IAppDomainSetup> pDomainSetup{ pDomain };
	hr = pDomainSetup->put_ApplicationBase(CComBSTR(L"C:\\Users\\panxi\\source\\repos\\shim\\gosh\\bin\\Debug"));
	if (FAILED(hr)) {
	}

	CComPtr<IUnknown> pAppDomain;
	hr = pRuntimeHost->CreateDomainEx(L"gosh", pDomain, NULL, &pAppDomain);
	if (FAILED(hr)) {
	}

	CComQIPtr<mscorlib::_AppDomain> pCustomDomain{ pAppDomain };
	CComPtr<mscorlib::_ObjectHandle> pObjHandle;
	hr = pCustomDomain->CreateInstance(CComBSTR(L"gosh"), CComBSTR{ L"Falcon.Desktop.Gosh" }, &pObjHandle);
	
	CComVariant v;
	hr = pObjHandle->Unwrap(&v);
	if (FAILED(hr)) {
	}
	

	CComQIPtr<AddinDesign::_IDTExtensibility2> sp{ v.pdispVal };
	CComSafeArray<VARIANT> sa;
	hr = sp->OnConnection(NULL, AddinDesign::ext_cm_Startup, NULL, &sa.m_psa);

	hr = pRuntimeHost->Stop();
	if (FAILED(hr)) {
	}
}

