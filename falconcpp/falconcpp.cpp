// falconcpp.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

template <typename T> struct alloc;

template <typename T> struct dealloc;

template <typename T> struct source;

template <typename T>
struct safe_ptr
{
	typedef typename source<T>::type source_type;
	typedef typename alloc<T>::type allocator;
	typedef typename dealloc<T>::type deallocator;
	source_type* src;
	T* v;
	template <typename...Args>
	safe_ptr(source_type* source, Args...args)
		:src(source)
	{ 
		v = allocator()(src, args...);		
	}
	~safe_ptr() { deallocator()(src, v); }
	T* operator->() { return v; }
	operator T*() { return v; }
};

struct alloc_typeattr
{
	LPTYPEATTR operator()(ITypeInfo* ti)
	{
		LPTYPEATTR ta;
		ti->GetTypeAttr(&ta);
		return ta;
	}
};

struct dealloc_typeattr
{
	void operator()(ITypeInfo* ti, LPTYPEATTR ta)
	{
		ti->ReleaseTypeAttr(ta);
	}
};

template <>
struct alloc<TYPEATTR>
{
	typedef alloc_typeattr type;
};

template <>
struct dealloc<TYPEATTR>
{
	typedef dealloc_typeattr type;
};

template <>
struct source<TYPEATTR>
{
	typedef ITypeInfo type;
};

struct alloc_funcdesc
{
	FUNCDESC* operator()(ITypeInfo* p, int index)
	{
		FUNCDESC* v;
		HRESULT hr = p->GetFuncDesc(index, &v);
		if (SUCCEEDED(hr))
			return v;
		return NULL;
	}
};

struct dealloc_funcdesc
{
	void operator()(ITypeInfo* p, FUNCDESC* v)
	{
		p->ReleaseFuncDesc(v);
	}
};

template <>
struct alloc<FUNCDESC>
{
	typedef alloc_funcdesc type;
};

template <>
struct dealloc<FUNCDESC>
{
	typedef dealloc_funcdesc type;
};

template <>
struct source<FUNCDESC>
{
	typedef ITypeInfo type;
};


template <typename T>
std::string get_doc_name(T t)
{
	CComBSTR name;
	CComBSTR help;
	t->GetDocumentation(-1, &name, &help, NULL, NULL);
	return std::string{ CW2A{ name } };
}


std::string get_qualified_name(ITypeInfo* p)
{
	std::stringstream s;

	CComPtr<ITypeLib> lib;
	UINT index;
	HRESULT hr = p->GetContainingTypeLib(&lib, &index);
	std::string q = get_doc_name(lib);
	if (q == "stdole") {
		s << get_doc_name(p);
	}
	else {
		s << get_doc_name(lib) << "::" << get_doc_name(p);
	}
	return s.str();
}


std::tuple<int, std::string, VARTYPE, TYPEKIND> type_info(TYPEDESC* t, ITypeInfo* p)
{
	int num = 0;
	TYPEDESC* c = t;
	while (c->vt == VT_PTR) {
		c = c->lptdesc;
		num++;
	}

	TYPEKIND kind;
	std::string s;
	VARTYPE v = c->vt;

	if (v == VT_I4) {
		s = "LONG";
	}
	else if (v == VT_UI2) {
		s = "WORD";
	}
	else if (v == VT_I1) {
		s = "char";
	}
	else if (v == VT_CY) {
		s = "CY";
	}
	else if (v == VT_I2) {
		s = "SHORT";
	}
	else if (v == VT_LPWSTR) {
		s = "LPOLESTR";
	}
	else if (v == VT_HRESULT) {
		s = "HRESULT";
	}
	else if (v == VT_VOID) {
		s = "void";
	}
	else if (v == VT_SAFEARRAY) {
		s = "SAFEARRAY *";
	}
	else if (v == VT_UINT) {
		s = "UINT";
	}
	else if (v == VT_UI4) {
		s = "ULONG";
	}
	else if (v == VT_R8) {
		s = "double";
	}
	else if (v == VT_R4) {
		s = "float";
	}
	else if (v == VT_INT) {
		s = "int";
	}
	else if (v == VT_BSTR) {
		s = "BSTR";
	}
	else if (v == VT_DISPATCH) {
		s = "IDispatch *";
	}
	else if (v == VT_VARIANT) {
		s = "VARIANT";
	}
	else if (v == VT_BOOL) {
		s = "VARIANT_BOOL";
	}
	else if (v == VT_DATE) {
		s = "DATE";
	}
	else if (v == VT_UNKNOWN) {
		s = "IUnknown *";
	}
	else if (v == VT_ERROR) {
		s = "SCODE";
	}
	else if (v == VT_USERDEFINED) {
		HREFTYPE h = c->hreftype;
		CComPtr<ITypeInfo> p2;
		HRESULT hr = p->GetRefTypeInfo(h, &p2);
		
		safe_ptr<TYPEATTR> a{ p2 };
		kind = a->typekind;
		if (a->typekind == TKIND_COCLASS) {
			hr = p2->GetRefTypeOfImplType(0, &h);
			CComPtr<ITypeInfo> p3;
			hr = p2->GetRefTypeInfo(h, &p3);
			p2 = p3;
			safe_ptr<TYPEATTR> b{p2 };
			kind = b->typekind;
		}
		else if (a->typekind == TKIND_ALIAS) {
			return type_info(&a->tdescAlias, p2);
		}
		
		s = get_qualified_name(p2);
	}
	return std::make_tuple(num, s, v, kind);
}

std::string get_type(ELEMDESC e, ITypeInfo* p)
{
	auto [num, s, vt, kind] = type_info(&e.tdesc, p);
	std::stringstream stream;
	stream << s;
	if (num > 0
		&& vt == VT_USERDEFINED 
		&& kind == TKIND_RECORD
		&& e.paramdesc.wParamFlags == PARAMFLAG_FIN) {
		stream << " const";
	}
	for (int i = 0; i < num; i++) {
		stream << " *";
	}
	return stream.str();
}

std::set<std::string> methods;

std::string get_sig(FUNCDESC* f, ITypeInfo* p)
{
	std::stringstream stream;
	stream << get_type(f->elemdescFunc, p) << "(__stdcall*)(void *";
	for (WORD index = 0; index < f->cParams; index++) {
		stream << ", " << get_type(f->lprgelemdescParam[index], p);
	}
	stream << ")";
	return stream.str();
}

std::vector<std::string> get_function_sigs(ITypeInfo* p)
{
	std::vector<std::string> r;
	safe_ptr<TYPEATTR> s{ p };

	if (IsEqualGUID(s->guid, IID_IUnknown)) {
		r.push_back("HRESULT(__stdcall*)(void *, const GUID *, void * *)");
		r.push_back("ULONG(__stdcall*)(void *)");
		r.push_back("ULONG(__stdcall*)(void *)");
		return r;
	}
	if (IsEqualGUID(s->guid, IID_IDispatch)) {
		r.push_back("HRESULT(__stdcall*)(void *, const GUID *, void * *)");
		r.push_back("ULONG(__stdcall*)(void *)");
		r.push_back("ULONG(__stdcall*)(void *)");
		r.push_back("HRESULT(__stdcall*)(void *, UINT *)");
		r.push_back("HRESULT(__stdcall*)(void *, UINT, LCID, ITypeInfo * *)");
		r.push_back("HRESULT(__stdcall*)(void *, const GUID *, LPOLESTR *, UINT, LCID, DISPID *)");
		r.push_back("HRESULT(__stdcall*)(void *, DISPID, const GUID *, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, UINT *)");
		return r;
	}

	for (WORD index = 0; index < s->cImplTypes; index++) {
		HREFTYPE h;
		HRESULT hr = p->GetRefTypeOfImplType(index, &h);
		CComPtr<ITypeInfo> i;
		hr = p->GetRefTypeInfo(h, &i);
		std::vector<std::string> a = get_function_sigs(i);
		r.insert(r.end(), a.begin(), a.end());
	}

	for (WORD index = 0; index < s->cFuncs; index++) {
		safe_ptr<FUNCDESC> f{ p, index };
		r.push_back(get_sig(f, p));
	}
	return r;
}

TYPEKIND get_type_kind(ITypeInfo* p)
{
	safe_ptr<TYPEATTR> attr{ p };
	return attr->typekind;
}

void dump(std::ofstream& ostream, ITypeLib* lib)
{
	UINT n = lib->GetTypeInfoCount();
	for (UINT index = 0; index < n; index++) {
		CComPtr<ITypeInfo> p;
		HRESULT hr = lib->GetTypeInfo(index, &p);
		if (FAILED(hr)) {
			continue;
		}

		auto type_kind = get_type_kind(p);

		if (type_kind != TKIND_INTERFACE 
			&& type_kind != TKIND_DISPATCH) {
			continue;
		}

		if (get_type_kind(p) == TKIND_DISPATCH) {
			HREFTYPE h;
			HRESULT hr = p->GetRefTypeOfImplType(-1, &h);
			if (FAILED(hr)) continue;
			CComPtr<ITypeInfo> p2;
			hr = p->GetRefTypeInfo(h, &p2);
			if (FAILED(hr)) continue;
			p = p2;
		}

		ostream << "template <>" << std::endl
			<< "struct falcon::define_interface<" << get_qualified_name(p) << ">" << std::endl
			<< "{" << std::endl 
			<< "typedef vtable<" << std::endl;
		auto sigs = get_function_sigs(p);
		for (auto& s : sigs) {
			methods.insert(s);
		}
		auto c = sigs.size();
		for (size_t index = 0; index < c; index++) {
			ostream << sigs[index];
			if (index < c - 1) {
				ostream << ", " << std::endl;
			}
		}

		ostream << "> type;" << std::endl 
			<< "};" << std::endl;		
	}
}

int main()
{
	std::ofstream file{ "msshim.h" };
	{
		CComPtr<ITypeLib> tl;
		HRESULT hr = LoadTypeLibEx(L"C:\\Program Files (x86)\\Microsoft Office\\root\\vfs\\ProgramFilesCommonX86\\Microsoft Shared\\OFFICE16\\MSO.DLL", REGKIND_NONE, &tl);
		dump(file, tl);
	}
	{
		CComPtr<ITypeLib> tl;
		HRESULT hr = LoadTypeLibEx(L"C:\\Program Files (x86)\\Microsoft Office\\root\\Office16\\MSOUTL.OLB", REGKIND_NONE, &tl);
		dump(file, tl);

	}
	for (auto& s : methods) {

	}
	return 0;
}

