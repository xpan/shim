#pragma once
#ifndef __PROXY_H
#define __PROXY_H

#include <Windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlsafe.h>
#include <map>
#include <tuple>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/serialization/split_member.hpp>

namespace falcon {

	template <typename T> struct dealloc;

	template <typename T, typename Dealloc = typename dealloc<T>::type>
	struct safe_handle
	{
		T v;
		safe_handle(T val) :v(val) {}
		~safe_handle()
		{
			Dealloc d;
			d(v);
		}
		operator T()
		{
			return v;
		}
	};

	struct dealloc_handle
	{
		void operator()(HANDLE h)
		{
			CloseHandle(h);
		}
	};

	template <>
	struct dealloc<HANDLE>
	{
		typedef dealloc_handle type;
	};

	struct dealloc_void
	{
		void operator()(LPVOID p)
		{
			UnmapViewOfFile(p);
		}
	};

	template <int N, typename F> struct fsig;

	template <typename F> struct handler;

	struct id
	{
		HWND hwnd;
		void* _this;

		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			ar & (DWORD)hwnd;
			ar & (DWORD)_this;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			DWORD dw;
			ar & dw;
			hwnd = (HWND)dw;
			ar & dw;
			_this = (void*)dw;
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};

	struct channel_data;

	struct in_memory_proxy
	{
		UINT_PTR ptr;
		id id;
		ULONG count;
		IID iid;
		channel_data* channel;
	};

	struct less_guid
	{
		bool operator()(const GUID& l, const GUID& R) const
		{
			return true;
		}
	};

	struct channel_data
	{
		static UINT const buf_size = 1048576;
		HWND hwnd;
		std::map<UINT, LRESULT(*)(channel_data*, UINT, WPARAM, LPARAM, BOOL&)> handlers;
		std::map<GUID, void*(*)(channel_data*, id const&), less_guid> obj_maps;
		std::vector<in_memory_proxy*> proxies;
	};

	template <typename T>
	struct in
	{
		T v;
		channel_data* channel;
		in(channel_data* data, T val) :channel(data), v(val) {}

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			//ar & v;
		}
	};

	template <typename T>
	struct out
	{
		T v;
		channel_data* channel;
		out(channel_data* data) :channel(data) {}

		operator T()
		{
			return v;
		}

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			//ar & v;
		}
	};


	

	class dispatcher : public CWindowImpl<dispatcher>
	{
	public:
		BEGIN_MSG_MAP(dispatcher)
			MESSAGE_HANDLER(WM_CREATE, OnCreate)
			MESSAGE_RANGE_HANDLER(0, 150, OnRequest)
		END_MSG_MAP()

		LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			bHandled = TRUE;
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			channel = (channel_data*)lpcs->lpCreateParams;
			channel->hwnd = m_hWnd;
			return S_OK;
		}

		LRESULT OnRequest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			auto it = channel->handlers.find(uMsg);
			if (it != channel->handlers.end()) {
				return it->second(channel, uMsg, wParam, lParam, bHandled);
			}
			return S_OK;
		}
		channel_data* channel;
	};

	template <int N, typename...Args> struct vtable_helper;

	template <int N, typename F, typename...Remaining>
	struct vtable_helper<N, F, Remaining...>
	{
		F _ = fsig<N, F>::apply;
		vtable_helper<N + 1, Remaining...> remaining;
	};

	template <int N>
	struct vtable_helper<N>
	{
	};

	template <typename... Args>
	struct vtable
		: vtable_helper<0, Args...>
	{
	};

	template <typename T> struct define_interface;

	template <typename T> struct define_msg;

	bool post_message_then_wait(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	enum falcon_msg
	{
		WM_RESPONSE = WM_USER + 0x0e10
	};

	template<unsigned N>
	struct serialize_helper
	{
		template<class Archive, typename... Args>
		static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
		{
			ar & std::get<N - 1>(t);
			serialize_helper<N - 1>::serialize(ar, t, version);
		}
	};

	template<>
	struct serialize_helper<0>
	{
		template<class Archive, typename... Args>
		static void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
		{
			(void)ar;
			(void)t;
			(void)version;
		}
	};

	template <int N, typename R, typename...Args>
	struct fsig<N, R(__stdcall*)(void*, Args...)>
	{
		static R __stdcall apply(void* _this, Args... args)
		{
			auto proxy = (in_memory_proxy*)(_this);
			auto channel = proxy->channel;
			safe_handle<HANDLE> file(CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, channel_data::buf_size, L"gosh_channel"));
			safe_handle<LPVOID, dealloc_void> view(MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, 0));

			boost::iostreams::array_sink sr((char*)(LPVOID)view, channel_data::buf_size);
			boost::iostreams::stream<boost::iostreams::array_sink> source(sr);
			boost::archive::binary_oarchive oa(source);

			oa & N;
			auto tuple = std::make_tuple(in<void*>{channel, _this}, in<Args>{channel, args}...);
			oa & tuple;

			auto constexpr message = define_msg<R(__stdcall*)(void*, Args...)>::value;

			bool result = post_message_then_wait(proxy->id.hwnd, message, (WPARAM)channel->hwnd, NULL);

			if (!result) return (R)-1;

			boost::iostreams::array_source device((char*)(LPVOID)view, channel_data::buf_size);
			boost::iostreams::stream<boost::iostreams::array_source> s(device);
			boost::archive::binary_iarchive ia(s);

			R hr{};
			ia & hr;
			ia & tuple;
			return hr;
		}
	};

	template<typename F, typename Tuple, size_t ...S >
	decltype(auto) apply_tuple_impl(F&& fn, Tuple&& t, std::index_sequence<S...>)
	{
		return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
	}

	template<typename F, typename Tuple>
	decltype(auto) apply_from_tuple(F&& fn, Tuple&& t)
	{
		std::size_t constexpr tSize
			= std::tuple_size<typename std::remove_reference<Tuple>::type>::value;

		return apply_tuple_impl(std::forward<F>(fn),
			std::forward<Tuple>(t),
			std::make_index_sequence<tSize>());
	}

	struct in_memory_obj
	{
		UINT_PTR* vtable;
		// Whatever layout we don't care
	};

	template <typename R, typename...Args>
	struct handler<R(__stdcall*)(void*, Args...)>
	{
		static LRESULT handle(channel_data* channel, UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
		{
			bHandled = TRUE;

			safe_handle<HANDLE> file(CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, channel_data::buf_size, L"gosh_channel"));
			safe_handle<LPVOID, dealloc_void> view(MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, 0));

			boost::iostreams::array_source device((char*)(LPVOID)view, channel_data::buf_size);
			boost::iostreams::stream<boost::iostreams::array_source> s(device);
			boost::archive::binary_iarchive ia(s);

			int index;
			ia & index;

			auto tuple = std::make_tuple(out<void*>(channel), out<Args>(channel)...);
			ia & tuple;

			auto obj = (in_memory_obj*)(void*)(std::get<0>(tuple));
			auto vtable = obj->vtable;
			typedef R(__stdcall* func_type)(void*, Args...);
			func_type func = (func_type)vtable[index];
			R hr = apply_from_tuple(func, tuple);

			boost::iostreams::array_sink sr((char*)(LPVOID)view, channel_data::buf_size);
			boost::iostreams::stream<boost::iostreams::array_sink> source(sr);
			boost::archive::binary_oarchive oa(source);

			oa & hr;
			oa & tuple;

			PostMessage((HWND)wParam, WM_RESPONSE, (WPARAM)channel->hwnd, NULL);

			return S_OK;
		}
	};

	template <typename T>
	struct meta
	{
		typedef typename define_interface<T>::type vtable;
		vtable _vtable;

		static T* make_proxy(channel_data* channel, id const& id)
		{
			static meta<T>* metadata = new meta<T>;

			in_memory_proxy* proxy = new in_memory_proxy;
			proxy->ptr = (UINT_PTR)&metadata->_vtable;
			proxy->id = id;
			proxy->iid = __uuidof(T);
			proxy->count = 1;
			proxy->channel = channel;
			return (T*)proxy;
		}
	};

	template <>
	struct define_interface<IUnknown>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*)> type;
	};

	template <>
	struct define_interface<IDispatch>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, UINT*),
			HRESULT(__stdcall*)(void*, UINT, LCID, ITypeInfo**),
			HRESULT(__stdcall*)(void*, IID const*, LPOLESTR*, UINT, LCID, DISPID*),
			HRESULT(__stdcall*)(void*, DISPID, IID const*, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)> type;
	};

	template <>
	struct define_interface<IConnectionPointContainer>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, IEnumConnectionPoints**),
			HRESULT(__stdcall*)(void*, IID const*, IConnectionPoint**)> type;
	};

	template <>
	struct define_interface<IEnumConnectionPoints>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, ULONG, LPCONNECTIONPOINT*, ULONG*),
			HRESULT(__stdcall*)(void*, ULONG),
			HRESULT(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, IEnumConnectionPoints**)> type;
	};

	template <>
	struct define_interface<IConnectionPoint>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, IID*),
			HRESULT(__stdcall*)(void*, IConnectionPointContainer**),
			HRESULT(__stdcall*)(void*, IUnknown*, DWORD*),
			HRESULT(__stdcall*)(void*, DWORD),
			HRESULT(__stdcall*)(void*, IEnumConnections**)> type;
	};

	template <>
	struct define_interface<IEnumConnections>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, ULONG, LPCONNECTDATA, ULONG*),
			HRESULT(__stdcall*)(void*, ULONG),
			HRESULT(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, IEnumConnections**)> type;
	};

	template <>
	struct define_interface<IEnumVARIANT>
	{
		typedef vtable<
			HRESULT(__stdcall*)(void*, IID const*, void**),
			ULONG(__stdcall*)(void*),
			ULONG(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, ULONG, VARIANT *, ULONG*),
			HRESULT(__stdcall*)(void*, ULONG),
			HRESULT(__stdcall*)(void*),
			HRESULT(__stdcall*)(void*, IEnumVARIANT**)> type;
	};

	template <>
	struct define_msg<HRESULT(__stdcall*)(void *, IID const*, void * *)>
	{
		static unsigned const value = 5;
	};

	template <>
	struct define_msg<ULONG(__stdcall*)(void *)>
	{
		static unsigned const value = 6;
	};

	

	template <>
	struct out<VARIANT*>
	{
		CComVariant v;
		~out() 
		{
			if (v.vt == VT_UNKNOWN || v.vt == VT_DISPATCH) {
				VARIANT v2;
				VariantInit(&v2);
				v.Detach(&v2);
			}
		}
		operator VARIANT*()
		{
			return &v;
		}

		template <typename Archive>
		void save(Archive &ar, const unsigned int version)
		{
			if (v.vt == VT_UNKNOWN) {
				v.punkVal;
			}
		}
	};

	template <typename T> 
	struct is_unk_ptr
	{
	};

	template <typename T>
	struct in<T*>
	{
		T* v;
		channel_data* channel;
		in(channel_data* _channel, T* val) : channel(_channel), v(val) {}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			bool null = v == NULL;
			ar & null;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			if (v != NULL) {
				ar & *v;
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};

	template <typename T>
	struct out<T*>
	{
		T v;
		channel_data* channel;
		bool null;
		out(channel_data* _channel) : channel(_channel) {}

		operator T*()
		{
			return null? NULL: &v;
		}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			if (!null) {
				ar & v;
			}
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			ar & null;
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};

	template <>
	struct in<void*>
	{
		void* v;
		channel_data* channel;
		in(channel_data* _channel, void* val) : channel(_channel), v(val) {}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			id id;
			id.hwnd = channel->hwnd;
			id._this = v;
			ar & id;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{

		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};

	template <>
	struct out<void*>
	{
		void* v;
		channel_data* channel;
		out(channel_data* _channel) : channel(_channel) {}

		operator void*()
		{
			return v;
		}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{

		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			id id;
			ar & id;
			v = id._this;
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};


	template <typename T>
	struct in<T const*>
	{
		T const* v;
		channel_data* channel;
		in(channel_data* _channel, T const* val) : channel(_channel), v(val) {}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			ar & *v;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};

	template <typename T>
	struct out<T const*>
	{
		T v;
		channel_data* channel;
		out(channel_data* _channel) : channel(_channel) {}

		operator T const*()
		{
			return &v;
		}

		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			ar & v;
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};


	template <typename T>
	struct in<is_unk_ptr<T>>
	{
		T* v;
		channel_data* channel;
		in(channel_data* _channel, T* val) : channel(_channel), v(val) {}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			auto proxy = (in_memory_proxy*)v;
			auto it = std::find(channel->proxies.cbegin(), channel->proxies.cend());
			if (it != channel->proxies.cend()) {
				ar & proxy->id;
			}
			else {
				id id;
				id.hwnd = channel->hwnd;
				id._this = v;
				ar & id;
			}
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};

	template <typename T>
	struct out<is_unk_ptr<T>>
	{
		T* v;
		channel_data* channel;
		out(channel_data* _channel) :channel(_channel) {}

		operator T*()
		{
			return v;
		}
		
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{		
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			id id;
			ar & id;
			if (id.hwnd == channel->hwnd) {
				v = (T*)id._this;
			}
			else {
				if (id._this == NULL) {
					v = NULL;
				}
				else {
					v = meta<T>::make_proxy(channel, id);
				}
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};

	template <typename T>
	struct in<T**>
	{
		T** v;
		channel_data* channel;
		in(channel_data* _channel, T** val) : channel(_channel), v(val) {}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			bool null = v == NULL;
			ar & null;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			if (v != NULL) {
				id id;
				ar & id;
				*v = meta<T>::make_proxy(channel, id);
			}
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};

	template <typename T>
	struct out<T**>
	{
		T* v;
		channel_data* channel;
		bool null;
		out(channel_data* _channel) : channel(_channel) {}

		operator T**()
		{
			return null? NULL : &v;
		}

		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			if (!null) {
				id id;
				id.hwnd = channel->hwnd;
				id._this = v;
				ar & id;
			}
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			ar & null;
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};

	template <>
	struct in<void**>
	{
		void** v;
		channel_data* channel;
		IID const* iid;
		in(channel_data* _channel, void** val, IID const* _iid) : channel(_channel), v(val), iid(_iid) {}
		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			bool null = v == NULL;
			ar & null;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			if (v != NULL) {
				id id;
				ar & id;
				*v = NULL;
				auto it = channel->obj_maps.find(*iid);
				if (it != channel->obj_maps.end()) {
					*v = it->second(channel, id);
				}
			}
			
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};

	template <>
	struct out<void**>
	{
		void* v = NULL;
		channel_data* channel;
		bool null;
		out(channel_data* _channel) :channel(_channel) {}

		operator void**()
		{
			return null ? NULL : &v;
		}

		template<class Archive>
		void save(Archive & ar, const unsigned int version) const
		{
			id id;
			id.hwnd = channel->hwnd;
			id._this = v;
			ar & id;
		}
		template<class Archive>
		void load(Archive & ar, const unsigned int version)
		{
			ar & null;
		}
		BOOST_SERIALIZATION_SPLIT_MEMBER()

	};

	template <>
	struct fsig<0, HRESULT(__stdcall*)(void*, IID const*, void**)>
	{
		static HRESULT __stdcall apply(void* _this, IID const* iid, void** ppv)
		{
			auto proxy = (in_memory_proxy*)(_this);
			auto channel = proxy->channel;
			safe_handle<HANDLE> file(CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, channel_data::buf_size, L"gosh_channel"));
			safe_handle<LPVOID, dealloc_void> view(MapViewOfFile(file, FILE_MAP_ALL_ACCESS, 0, 0, 0));

			boost::iostreams::array_sink sr((char*)(LPVOID)view, channel_data::buf_size);
			boost::iostreams::stream<boost::iostreams::array_sink> source(sr);
			boost::archive::binary_oarchive oa(source);

			oa & 0;
			auto tuple = std::make_tuple(in<void*>{channel, _this}, in<IID const*>{channel, iid}, in<void**>(channel, ppv, iid));
			oa & tuple;

			auto constexpr message = define_msg<HRESULT(__stdcall*)(void*, IID const*, void**)>::value;

			bool result = post_message_then_wait(proxy->id.hwnd, message, (WPARAM)channel->hwnd, NULL);

			if (!result) return -1;

			boost::iostreams::array_source device((char*)(LPVOID)view, channel_data::buf_size);
			boost::iostreams::stream<boost::iostreams::array_source> s(device);
			boost::archive::binary_iarchive ia(s);

			HRESULT hr;
			ia & hr;
			ia & tuple;
			return hr;
		}
	};

	template <>
	struct fsig<2, ULONG(__stdcall*)(void*)>
	{
		static ULONG __stdcall apply(void* _this)
		{
			in_memory_proxy* proxy = (in_memory_proxy*)_this;
			proxy->count--;
			ULONG count = proxy->count;
			if (count == 0) {
				delete proxy;
			}
			return count;
		}
	};
}

namespace boost {
	namespace serialization {

		template<class Archive, typename... Args>
		void serialize(Archive & ar, std::tuple<Args...> & t, const unsigned int version)
		{
			falcon::serialize_helper<sizeof...(Args)>::serialize(ar, t, version);
		}

		template<class Archive>
		void serialize(Archive & ar, IID & t, const unsigned int version)
		{
			ar & t.Data1;
			ar & t.Data2;
			ar & t.Data3;
			ar & t.Data4[0];
			ar & t.Data4[1];
			ar & t.Data4[2];
			ar & t.Data4[3];
			ar & t.Data4[4];
			ar & t.Data4[5];
			ar & t.Data4[6];
			ar & t.Data4[7];
		}
	}

}
#endif