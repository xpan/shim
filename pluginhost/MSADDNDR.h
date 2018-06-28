// Created by Microsoft (R) C/C++ Compiler Version 14.14.26429.4 (a42aed3d).
//
// c:\users\panxi\source\repos\shim\pluginhost\debug\MSADDNDR.tlh
//
// C++ source equivalent of Win32 type library AC0714F2-3D04-11D1-AE7D-00A0C90F26F4
// compiler-generated file created 05/28/18 at 23:16:57 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace AddinDesign {

//
// Forward references and typedefs
//

struct __declspec(uuid("ac0714f2-3d04-11d1-ae7d-00a0c90f26f4"))
/* LIBID */ __AddInDesignerObjects;
struct __declspec(uuid("ac0714f3-3d04-11d1-ae7d-00a0c90f26f4"))
/* dual interface */ IAddinDesigner;
struct __declspec(uuid("ac0714f4-3d04-11d1-ae7d-00a0c90f26f4"))
/* dual interface */ IAddinInstance;
struct __declspec(uuid("b65ad801-abaf-11d0-bb8b-00a0c90f2744"))
/* dual interface */ _IDTExtensibility2;
enum __MIDL___MIDL_itf_msaddndr_0000_0000_0001;
enum __MIDL___MIDL_itf_msaddndr_0000_0000_0002;
struct /* coclass */ AddinDesigner;
struct /* coclass */ AddinDesigner2;
struct /* coclass */ AddinInstance;
struct /* coclass */ AddinInstance2;
typedef enum __MIDL___MIDL_itf_msaddndr_0000_0000_0001 ext_ConnectMode;
typedef enum __MIDL___MIDL_itf_msaddndr_0000_0000_0002 ext_DisconnectMode;
typedef struct _IDTExtensibility2 * IDTExtensibility2;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IAddinDesigner, __uuidof(IAddinDesigner));
_COM_SMARTPTR_TYPEDEF(IAddinInstance, __uuidof(IAddinInstance));
_COM_SMARTPTR_TYPEDEF(_IDTExtensibility2, __uuidof(_IDTExtensibility2));

//
// Type library items
//

struct __declspec(uuid("ac0714f3-3d04-11d1-ae7d-00a0c90f26f4"))
IAddinDesigner : IDispatch
{};

struct __declspec(uuid("ac0714f4-3d04-11d1-ae7d-00a0c90f26f4"))
IAddinInstance : IDispatch
{};

enum __MIDL___MIDL_itf_msaddndr_0000_0000_0001
{
    ext_cm_AfterStartup = 0,
    ext_cm_Startup = 1,
    ext_cm_External = 2,
    ext_cm_CommandLine = 3
};

struct __declspec(uuid("b65ad801-abaf-11d0-bb8b-00a0c90f2744"))
_IDTExtensibility2 : IDispatch
{
    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall OnConnection (
        /*[in]*/ IDispatch * Application,
        /*[in]*/ ext_ConnectMode ConnectMode,
        /*[in]*/ IDispatch * AddInInst,
        /*[in]*/ SAFEARRAY * * custom ) = 0;
      virtual HRESULT __stdcall OnDisconnection (
        /*[in]*/ ext_DisconnectMode RemoveMode,
        /*[in]*/ SAFEARRAY * * custom ) = 0;
      virtual HRESULT __stdcall OnAddInsUpdate (
        /*[in]*/ SAFEARRAY * * custom ) = 0;
      virtual HRESULT __stdcall OnStartupComplete (
        /*[in]*/ SAFEARRAY * * custom ) = 0;
      virtual HRESULT __stdcall OnBeginShutdown (
        /*[in]*/ SAFEARRAY * * custom ) = 0;
};

enum __MIDL___MIDL_itf_msaddndr_0000_0000_0002
{
    ext_dm_HostShutdown = 0,
    ext_dm_UserClosed = 1
};

struct __declspec(uuid("ac0714f6-3d04-11d1-ae7d-00a0c90f26f4"))
AddinDesigner;
    // [ default ] interface IAddinDesigner
    // [ default, source ] interface _IDTExtensibility2

struct __declspec(uuid("e436987e-f427-4ad7-8738-6d0895a3e93f"))
AddinDesigner2;
    // [ default ] interface IAddinDesigner
    // [ default, source ] interface _IDTExtensibility2

struct __declspec(uuid("ac0714f7-3d04-11d1-ae7d-00a0c90f26f4"))
AddinInstance;
    // [ default ] interface IAddinInstance
    // [ default, source ] interface _IDTExtensibility2

struct __declspec(uuid("ab5357a7-3179-47f9-a705-966b8b936d5e"))
AddinInstance2;
    // [ default ] interface IAddinInstance
    // [ default, source ] interface _IDTExtensibility2

} // namespace AddinDesign

#pragma pack(pop)
