/*
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with the
 * License. You may obtain a copy of the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
 * the specific language governing rights and limitations under the License.
 *
 * The Original Code is the Python XPCOM language bindings.
 *
 * The Initial Developer of the Original Code is ActiveState Tool Corp.
 * Portions created by ActiveState Tool Corp. are Copyright (C) 2000, 2001
 * ActiveState Tool Corp.  All Rights Reserved.
 *
 * Contributor(s): Mark Hammond <mhammond@skippinet.com.au> (original author)
 *
 */

// PyXPCOM.h - the main header file for the Python XPCOM support.
//
// This code is part of the XPCOM extensions for Python.
//
// Written May 2000 by Mark Hammond.
//
// Based heavily on the Python COM support, which is
// (c) Mark Hammond and Greg Stein.
//
// (c) 2000, ActiveState corp.

#ifndef __PYXPCOM_H__
#define __PYXPCOM_H__

#ifdef XP_WIN
#	ifdef BUILD_PYXPCOM
		/* We are building the main dll */
#		define PYXPCOM_EXPORT __declspec(dllexport)
#	else
		/* This module uses the dll */
#		define PYXPCOM_EXPORT __declspec(dllimport)
#	endif // BUILD_PYXPCOM

	// We need these libs!
#	pragma comment(lib, "xpcom.lib")
#	pragma comment(lib, "nspr4.lib")

#else // XP_WIN
#	define PYXPCOM_EXPORT
#endif // XP_WIN


// An IID we treat as NULL when passing as a reference.
extern nsIID Py_nsIID_NULL;

/*************************************************************************
**************************************************************************

 Error and exception related function.

**************************************************************************
*************************************************************************/

// The exception object (loaded from the xpcom .py code)
extern PYXPCOM_EXPORT PyObject *PyXPCOM_Error;

// Client related functions - generally called by interfaces before
// they return NULL back to Python to indicate the error.
// All these functions return NULL so interfaces can generally
// just "return PyXPCOM_BuildPyException(hr, punk, IID_IWhatever)"
PYXPCOM_EXPORT PyObject *PyXPCOM_BuildPyException(nsresult res);

// Used in gateways to handle the current Python exception
// NOTE: this function assumes it is operating within the Python context
PYXPCOM_EXPORT nsresult PyXPCOM_SetCOMErrorFromPyException();

// A couple of logging/error functions.  These probably end up
// being written to the console service.

// Log a warning for the user - something at runtime
// they may care about, but nothing that prevents us actually
// working.
// As it's designed for user error/warning, it exists in non-debug builds.
PYXPCOM_EXPORT void PyXPCOM_LogWarning(const char *fmt, ...);

// Log an error for the user - something that _has_ prevented
// us working.  This is probably accompanied by a traceback.
// As it's designed for user error/warning, it exists in non-debug builds.
PYXPCOM_EXPORT void PyXPCOM_LogError(const char *fmt, ...);

#ifdef DEBUG
// Mainly designed for developers of the XPCOM package.
// Only enabled in debug builds.
PYXPCOM_EXPORT void PyXPCOM_LogDebug(const char *fmt, ...);
#define PYXPCOM_LOG_DEBUG PyXPCOM_LogDebug
#else
#define PYXPCOM_LOG_DEBUG()
#endif // DEBUG

/*************************************************************************
**************************************************************************

 Support for CALLING (ie, using) interfaces.

**************************************************************************
*************************************************************************/

class Py_nsISupports;

typedef Py_nsISupports* (* PyXPCOM_I_CTOR)(nsISupports *, const nsIID &);

//////////////////////////////////////////////////////////////////////////
// class PyXPCOM_TypeObject
// Base class for (most of) the type objects.

class PYXPCOM_EXPORT PyXPCOM_TypeObject : public PyTypeObject {
public:
	PyXPCOM_TypeObject( 
		const char *name, 
		PyXPCOM_TypeObject *pBaseType, 
		int typeSize, 
		struct PyMethodDef* methodList,
		PyXPCOM_I_CTOR ctor);
	~PyXPCOM_TypeObject();

	PyMethodChain chain;
	PyXPCOM_TypeObject *baseType;
	PyXPCOM_I_CTOR ctor;

	static PRBool IsType(PyTypeObject *t);
	// Static methods for the Python type.
	static void Py_dealloc(PyObject *ob);
	static PyObject *Py_repr(PyObject *ob);
	static PyObject *Py_str(PyObject *ob);
	static PyObject *Py_getattr(PyObject *self, char *name);
	static int Py_setattr(PyObject *op, char *name, PyObject *v);
	static int Py_cmp(PyObject *ob1, PyObject *ob2);
	static long Py_hash(PyObject *self);
};

//////////////////////////////////////////////////////////////////////////
// class Py_nsISupports
// This class serves 2 purposes:
// * It is a base class for other interfaces we support "natively"
// * It is instantiated for _all_ other interfaces.
//
// This is different than win32com, where a PyIUnknown only
// ever holds an IUnknown - but here, we could be holding
// _any_ interface.
class PYXPCOM_EXPORT Py_nsISupports : public PyObject
{
public:
	static PRBool Check( PyObject *ob, const nsIID &checkIID = Py_nsIID_NULL) {
		Py_nsISupports *self = static_cast<Py_nsISupports *>(ob);
		if (ob==NULL || !PyXPCOM_TypeObject::IsType(ob->ob_type ))
			return PR_FALSE;
		if (!checkIID.Equals(Py_nsIID_NULL))
			return self->m_iid.Equals(checkIID) != 0;
		return PR_TRUE;
	}
	// Get the nsISupports interface from the PyObject WITH NO REF COUNT ADDED
	static nsISupports *GetI(PyObject *self, nsIID *ret_iid = NULL);
	nsISupports *m_obj;
	nsIID m_iid;

	// Given an nsISupports and an Interface ID, create and return an object
	// Does not QI the object - the caller must ensure the nsISupports object
	// is really a pointer to an object identified by the IID.
	// PRBool bAddRef indicates if a COM reference count should be added to the interface.
	//  This depends purely on the context in which it is called.  If the interface is obtained
	//  from a function that creates a new ref (eg, ???) then you should use
	//  FALSE.  If you receive the pointer as (eg) a param to a gateway function, then
	//  you normally need to pass TRUE, as this is truly a new reference.
	//  *** ALWAYS take the time to get this right. ***
	// PRBool bMakeNicePyObject indicates if we should call back into
	//  Python to wrap the object.  This allows Python code to
	//  see the correct xpcom.client.Interface object even when calling
	//  xpcom function directly.
	static PyObject *PyObjectFromInterface(nsISupports *ps, 
	                                       const nsIID &iid, 
	                                       PRBool bAddRef,
	                                       PRBool bMakeNicePyObject = PR_TRUE);

	static PyObject *PyObjectFromInterfaceOrVariant(nsISupports *ps,
	                                                const nsIID &iid, 
	                                                PRBool bAddRef,
	                                                PRBool bMakeNicePyObject = PR_TRUE);

	// Given a Python object that is a registered COM type, return a given
	// interface pointer on its underlying object, with a NEW REFERENCE ADDED.
	// bTryAutoWrap indicates if a Python instance object should attempt to
	//  be automatically wrapped in an XPCOM object.  This is really only
	//  provided to stop accidental recursion should the object returned by
	//  the wrap process itself be in instance (where it should already be
	//  a COM object.
	// If |iid|==nsIVariant, then arbitary Python objects will be wrapped
	// in an nsIVariant.
	static PRBool InterfaceFromPyObject(
		PyObject *ob,
		const nsIID &iid,
		nsISupports **ppret,
		PRBool bNoneOK,
		PRBool bTryAutoWrap = PR_TRUE);

	// Given a Py_nsISupports, return an interface.
	// Object *must* be Py_nsISupports - there is no
	// "autowrap", no "None" support, etc
	static PRBool InterfaceFromPyISupports(PyObject *ob, 
	                                       const nsIID &iid, 
	                                       nsISupports **ppv);

	static Py_nsISupports *Constructor(nsISupports *pInitObj, const nsIID &iid);
	// The Python methods
	static PyObject *QueryInterface(PyObject *self, PyObject *args);

	// Internal (sort-of) objects.
	static PyXPCOM_TypeObject *type;
	static PyMethodDef methods[];
	static PyObject *mapIIDToType;
	static void SafeRelease(Py_nsISupports *ob);
	static void RegisterInterface( const nsIID &iid, PyTypeObject *t);
	static void InitType();

	virtual ~Py_nsISupports();
	virtual PyObject *getattr(const char *name);
	virtual int setattr(const char *name, PyObject *val);
protected:
	// ctor is protected - must create objects via
	// PyObjectFromInterface()
	Py_nsISupports(nsISupports *p, 
		            const nsIID &iid, 
			    PyTypeObject *type);

	static PyObject *MakeInterfaceResult(PyObject *pyis, const nsIID &iid);

};

// Python/XPCOM IID support 
class PYXPCOM_EXPORT Py_nsIID : public PyObject
{
public:
	Py_nsIID(const nsIID &riid);
	nsIID m_iid;

	PRBool 
	IsEqual(const nsIID &riid) {
		return m_iid.Equals(riid);
	}

	PRBool
	IsEqual(PyObject *ob) {
		return ob && 
		       ob->ob_type== &type && 
		       m_iid.Equals(((Py_nsIID *)ob)->m_iid);
	}

	PRBool
	IsEqual(Py_nsIID &iid) {
		return m_iid.Equals(iid.m_iid);
	}

	static PyObject *
	PyObjectFromIID(const nsIID &iid) {
		return new Py_nsIID(iid);
	}

	static PRBool IIDFromPyObject(PyObject *ob, nsIID *pRet);
	/* Python support */
	static PyObject *PyTypeMethod_getattr(PyObject *self, char *name);
	static int PyTypeMethod_compare(PyObject *self, PyObject *ob);
	static PyObject *PyTypeMethod_repr(PyObject *self);
	static long PyTypeMethod_hash(PyObject *self);
	static PyObject *PyTypeMethod_str(PyObject *self);
	static void PyTypeMethod_dealloc(PyObject *self);
	static PyTypeObject type;
	static PyMethodDef methods[];
};

///////////////////////////////////////////////////////
//
// Helper classes for managing arrays of variants.
class PythonTypeDescriptor; // Forward declare.

class PYXPCOM_EXPORT PyXPCOM_InterfaceVariantHelper {
public:
	PyXPCOM_InterfaceVariantHelper();
	~PyXPCOM_InterfaceVariantHelper();
	PRBool Init(PyObject *obParams);
	PRBool FillArray();

	PyObject *MakePythonResult();

	nsXPTCVariant *m_var_array;
	int m_num_array;
protected:
	PyObject *MakeSinglePythonResult(int index);
	PRBool FillInVariant(const PythonTypeDescriptor &, int, int);
	PRBool PrepareOutVariant(const PythonTypeDescriptor &td, int value_index);
	PRBool SetSizeIs( int var_index, PRBool is_arg1, PRUint32 new_size);
	PRUint32 GetSizeIs( int var_index, PRBool is_arg1);

	PyObject *m_pyparams; // sequence of actual params passed (ie, not including hidden)
	PyObject *m_typedescs; // desc of _all_ params, including hidden.
	PythonTypeDescriptor *m_python_type_desc_array;
	void **m_buffer_array;

};

/*************************************************************************
**************************************************************************

 Support for IMPLEMENTING interfaces.

**************************************************************************
*************************************************************************/
#define NS_IINTERNALPYTHON_IID_STR "AC7459FC-E8AB-4f2e-9C4F-ADDC53393A20"
#define NS_IINTERNALPYTHON_IID \
	{ 0xac7459fc, 0xe8ab, 0x4f2e, { 0x9c, 0x4f, 0xad, 0xdc, 0x53, 0x39, 0x3a, 0x20 } }

class PyXPCOM_GatewayWeakReference;

// This interface is needed primarily to give us a known vtable base.
// If we QI a Python object for this interface, we can safely cast the result
// to a PyG_Base.  Any other interface, we do now know which vtable we will get.
// We also allow the underlying PyObject to be extracted
class nsIInternalPython : public nsISupports {
public: 
	NS_DEFINE_STATIC_IID_ACCESSOR(NS_IINTERNALPYTHON_IID)
	// Get the underlying Python object with new reference added
	virtual PyObject *UnwrapPythonObject(void) = 0;
};

// This is roughly equivilent to PyGatewayBase in win32com
//
class PYXPCOM_EXPORT PyG_Base : public nsIInternalPython, public nsISupportsWeakReference
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSISUPPORTSWEAKREFERENCE
	PyObject *UnwrapPythonObject(void);

	// A static "constructor" - the real ctor is protected.
	static nsresult CreateNew(PyObject *pPyInstance, 
		                  const nsIID &iid, 
				  void **ppResult);

	// A utility to auto-wrap an arbitary Python instance 
	// in a COM gateway.
	static PRBool AutoWrapPythonInstance(PyObject *ob, 
		                           const nsIID &iid, 
					   nsISupports **ppret);


	// A helper that creates objects to be passed for nsISupports
	// objects.  See extensive comments in PyG_Base.cpp.
	PyObject *MakeInterfaceParam(nsISupports *pis, 
	                                     const nsIID *piid, 
					     int methodIndex = -1,
					     const XPTParamDescriptor *d = NULL, 
					     int paramIndex = -1);

	// A helper that ensures all casting and vtable offsetting etc
	// done against this object happens in the one spot!
	virtual void *ThisAsIID( const nsIID &iid ) = 0;

	// Helpers for "native" interfaces.
	// Not used by the generic stub interface.
	nsresult HandleNativeGatewayError(const char *szMethodName);

	// These data members used by the converter helper functions - hence public
	nsIID m_iid;
	PyObject * m_pPyObject;
	// We keep a reference count on this object, and the object
	// itself uses normal refcount rules - thus, it will only
	// die when we die, and all external references are removed.
	// This means that once we have created it (and while we
	// are alive) it will never die.
	nsCOMPtr<nsIWeakReference> m_pWeakRef;
#ifdef NS_BUILD_REFCNT_LOGGING
	char refcntLogRepr[64]; // sigh - I wish I knew how to use the Moz string classes :(  OK for debug only tho.
#endif
protected:
	PyG_Base(PyObject *instance, const nsIID &iid);
	virtual ~PyG_Base();
	PyG_Base *m_pBaseObject; // A chain to implement identity rules.
	nsresult InvokeNativeViaPolicy(	const char *szMethodName,
			PyObject **ppResult = NULL,
			const char *szFormat = NULL,
			...
			);
	nsresult InvokeNativeViaPolicyInternal(	const char *szMethodName,
			PyObject **ppResult,
			const char *szFormat,
			va_list va);
	nsresult InvokeNativeGetViaPolicy(const char *szPropertyName,
			PyObject **ppResult = NULL
			);
	nsresult InvokeNativeSetViaPolicy(const char *szPropertyName,
			...);
};

class PYXPCOM_EXPORT PyXPCOM_XPTStub : public PyG_Base, public nsXPTCStubBase
{
friend class PyG_Base;
public:
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
		{return PyG_Base::QueryInterface(aIID, aInstancePtr);}     \
	NS_IMETHOD_(nsrefcnt) AddRef(void) {return PyG_Base::AddRef();}    \
	NS_IMETHOD_(nsrefcnt) Release(void) {return PyG_Base::Release();}  \

	NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** info);
	// call this method and return result
	NS_IMETHOD CallMethod(PRUint16 methodIndex,
                          const nsXPTMethodInfo* info,
                          nsXPTCMiniVariant* params);

	virtual void *ThisAsIID(const nsIID &iid);
protected:
	PyXPCOM_XPTStub(PyObject *instance, const nsIID &iid) : PyG_Base(instance, iid) {;}
private:
};

// For the Gateways we manually implement.
#define PYGATEWAY_BASE_SUPPORT(INTERFACE, GATEWAY_BASE)                    \
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
		{return PyG_Base::QueryInterface(aIID, aInstancePtr);}     \
	NS_IMETHOD_(nsrefcnt) AddRef(void) {return PyG_Base::AddRef();}    \
	NS_IMETHOD_(nsrefcnt) Release(void) {return PyG_Base::Release();}  \
	virtual void *ThisAsIID(const nsIID &iid) {                        \
		if (iid.Equals(NS_GET_IID(INTERFACE))) return (INTERFACE *)this; \
		return GATEWAY_BASE::ThisAsIID(iid);                       \
	}                                                                  \


// Weak Reference class.  This is a true COM object, representing
// a weak reference to a Python object.  For each Python XPCOM object,
// there is exactly zero or one corresponding weak reference instance.
// When both are alive, each holds a pointer to the other.  When the main
// object dies due to XPCOM reference counting, it zaps the pointer
// in its corresponding weak reference object.  Thus, the weak-reference
// can live beyond the object (possibly with a NULL pointer back to the 
// "real" object, but as implemented, the weak reference will never be 
// destroyed  before the object
class PYXPCOM_EXPORT PyXPCOM_GatewayWeakReference : public nsIWeakReference {
public:
	PyXPCOM_GatewayWeakReference(PyG_Base *base);
	virtual ~PyXPCOM_GatewayWeakReference();
	NS_DECL_ISUPPORTS
    NS_DECL_NSIWEAKREFERENCE;
	PyG_Base *m_pBase; // NO REF COUNT!!!
#ifdef NS_BUILD_REFCNT_LOGGING
	char refcntLogRepr[41];
#endif
};


// Helpers classes for our gateways.
class PYXPCOM_EXPORT PyXPCOM_GatewayVariantHelper
{
public:
	PyXPCOM_GatewayVariantHelper( PyG_Base *gateway,
	                              int methodIndex,
	                              const nsXPTMethodInfo *info, 
	                              nsXPTCMiniVariant* params );
	~PyXPCOM_GatewayVariantHelper();
	PyObject *MakePyArgs();
	nsresult ProcessPythonResult(PyObject *ob);
	PyG_Base *m_gateway;
private:
	nsresult BackFillVariant( PyObject *ob, int index);
	PyObject *MakeSingleParam(int index, PythonTypeDescriptor &td);
	PRBool GetIIDForINTERFACE_ID(int index, const nsIID **ppret);
	nsresult GetArrayType(PRUint8 index, PRUint8 *ret);
	PRUint32 GetSizeIs( int var_index, PRBool is_arg1);
	PRBool SetSizeIs( int var_index, PRBool is_arg1, PRUint32 new_size);
	PRBool CanSetSizeIs( int var_index, PRBool is_arg1 );
	nsIInterfaceInfo *GetInterfaceInfo(); // NOTE: no ref count on result.


	nsXPTCMiniVariant* m_params;
	const nsXPTMethodInfo *m_info;
	int m_method_index;
	PythonTypeDescriptor *m_python_type_desc_array;
	int m_num_type_descs;
	nsCOMPtr<nsIInterfaceInfo> m_interface_info;
};

// Misc converters.
PyObject *PyObject_FromXPTType( const nsXPTType *d);
// XPTTypeDescriptor derived from XPTType - latter is automatically processed via PyObject_FromXPTTypeDescriptor XPTTypeDescriptor 
PyObject *PyObject_FromXPTTypeDescriptor( const XPTTypeDescriptor *d);

PyObject *PyObject_FromXPTParamDescriptor( const XPTParamDescriptor *d);
PyObject *PyObject_FromXPTMethodDescriptor( const XPTMethodDescriptor *d);
PyObject *PyObject_FromXPTConstant( const XPTConstDescriptor *d);

nsIVariant *PyObject_AsVariant( PyObject *ob);
PyObject *PyObject_FromVariant( nsIVariant *v);

// DLL reference counting functions.
// Although we maintain the count, we never actually
// finalize Python when it hits zero!
void PyXPCOM_DLLAddRef();
void PyXPCOM_DLLRelease();

/*************************************************************************
**************************************************************************

 LOCKING AND THREADING

**************************************************************************
*************************************************************************/

//
// We have 2 discrete locks in use (when no free-threaded is used, anyway).
// The first type of lock is the global Python lock.  This is the standard lock
// in use by Python, and must be used as documented by Python.  Specifically, no
// 2 threads may _ever_ call _any_ Python code (including INCREF/DECREF) without
// first having this thread lock.
//
// The second type of lock is a "global framework lock", and used whenever 2 threads 
// of C code need access to global data.  This is different than the Python 
// lock - this lock is used when no Python code can ever be called by the 
// threads, but the C code still needs thread-safety.

// We also supply helper classes which make the usage of these locks a one-liner.

// The "framework" lock, implemented as a PRLock
PYXPCOM_EXPORT void PyXPCOM_AcquireGlobalLock(void);
PYXPCOM_EXPORT void PyXPCOM_ReleaseGlobalLock(void);

// Helper class for the DLL global lock.
//
// This class magically waits for PyXPCOM framework global lock, and releases it
// when finished.  
// NEVER new one of these objects - only use on the stack!
class CEnterLeaveXPCOMFramework {
public:
	CEnterLeaveXPCOMFramework() {PyXPCOM_AcquireGlobalLock();}
	~CEnterLeaveXPCOMFramework() {PyXPCOM_ReleaseGlobalLock();}
};

// Python thread-lock stuff.  Free-threading patches use different semantics, but
// these are abstracted away here...
//#include <threadstate.h>

// Helper class for Enter/Leave Python
//
// This class magically waits for the Python global lock, and releases it
// when finished.  

// Nested invocations will deadlock, so be careful.

// NEVER new one of these objects - only use on the stack!

extern PYXPCOM_EXPORT PyInterpreterState *PyXPCOM_InterpreterState;
extern PYXPCOM_EXPORT PRBool PyXPCOM_ThreadState_Ensure();
extern PYXPCOM_EXPORT void PyXPCOM_ThreadState_Free();
extern PYXPCOM_EXPORT void PyXPCOM_ThreadState_Clear();
extern PYXPCOM_EXPORT void PyXPCOM_InterpreterLock_Acquire();
extern PYXPCOM_EXPORT void PyXPCOM_InterpreterLock_Release();
extern PYXPCOM_EXPORT void PyXPCOM_MakePendingCalls();

extern PYXPCOM_EXPORT PRBool PyXPCOM_Globals_Ensure();

class CEnterLeavePython {
public:
	CEnterLeavePython() {
		created = PyXPCOM_ThreadState_Ensure();
		PyXPCOM_InterpreterLock_Acquire();
		if (created) {
			// If pending python calls are waiting as we enter Python,
			// it will generally mean an asynch signal handler, etc.
			// We can either call it here, or wait for Python to call it
			// as part of its "even 'n' opcodes" check.  If we wait for
			// Python to check it and the pending call raises an exception,
			// then it is _our_ code that will fail - this is unfair,
			// as the signal was raised before we were entered - indeed,
			// we may be directly responding to the signal!
			// Thus, we flush all the pending calls here, and report any
			// exceptions via our normal exception reporting mechanism.
			// We can then execute our code in the knowledge that only
			// signals raised _while_ we are executing will cause exceptions.
			PyXPCOM_MakePendingCalls();
		}
	}
	~CEnterLeavePython() {
	// The interpreter state must be cleared
	// _before_ we release the lock, as some of
	// the sys. attributes cleared (eg, the current exception)
	// may need the lock to invoke their destructors - 
	// specifically, when exc_value is a class instance, and
	// the exception holds the last reference!
		if ( created )
			PyXPCOM_ThreadState_Clear();
		PyXPCOM_InterpreterLock_Release();
		if ( created )
			PyXPCOM_ThreadState_Free();
	}
private:
	PRBool created;
};

// Our classes.
// Hrm - So we can't have templates, eh??
// preprocessor to the rescue, I guess.
#define PyXPCOM_INTERFACE_DECLARE(ClassName, InterfaceName, Methods )     \
                                                                          \
extern struct PyMethodDef Methods[];                                      \
                                                                          \
class ClassName : public Py_nsISupports                                   \
{                                                                         \
public:                                                                   \
	static PyXPCOM_TypeObject *type;                                  \
	static Py_nsISupports *Constructor(nsISupports *pInitObj, const nsIID &iid) { \
		return new ClassName(pInitObj, iid);                      \
	}                                                                 \
	static void InitType(PyObject *iidNameDict) {                     \
		type = new PyXPCOM_TypeObject(                            \
				#InterfaceName,                           \
				Py_nsISupports::type,                     \
				sizeof(ClassName),                        \
				Methods,                                  \
				Constructor);                             \
		const nsIID &iid = NS_GET_IID(InterfaceName);             \
		RegisterInterface(iid, type);                             \
		PyObject *iid_ob = Py_nsIID::PyObjectFromIID(iid);        \
		PyDict_SetItemString(iidNameDict, "IID_"#InterfaceName, iid_ob); \
		Py_DECREF(iid_ob);                                        \
	}                                                                 \
protected:                                                                \
	ClassName(nsISupports *p, const nsIID &iid) :                     \
		Py_nsISupports(p, iid, type) {                            \
		/* The IID _must_ be the IID of the interface we are wrapping! */    \
		NS_ABORT_IF_FALSE(iid.Equals(NS_GET_IID(InterfaceName)), "Bad IID"); \
	}                                                                 \
};                                                                        \
                                                                          \
// End of PyXPCOM_INTERFACE_DECLARE macro

#define PyXPCOM_ATTR_INTERFACE_DECLARE(ClassName, InterfaceName, Methods )\
                                                                          \
extern struct PyMethodDef Methods[];                                      \
                                                                          \
class ClassName : public Py_nsISupports                                   \
{                                                                         \
public:                                                                   \
	static PyXPCOM_TypeObject *type;                                  \
	static Py_nsISupports *Constructor(nsISupports *pInitObj, const nsIID &iid) { \
		return new ClassName(pInitObj, iid);                      \
	}                                                                 \
	static void InitType(PyObject *iidNameDict) {                     \
		type = new PyXPCOM_TypeObject(                            \
				#InterfaceName,                           \
				Py_nsISupports::type,                     \
				sizeof(ClassName),                        \
				Methods,                                  \
				Constructor);                             \
		const nsIID &iid = NS_GET_IID(InterfaceName);             \
		RegisterInterface(iid, type);                             \
		PyObject *iid_ob = Py_nsIID::PyObjectFromIID(iid);        \
		PyDict_SetItemString(iidNameDict, "IID_"#InterfaceName, iid_ob); \
		Py_DECREF(iid_ob);                                        \
}                                                                         \
	virtual PyObject *getattr(const char *name);                      \
	virtual int setattr(const char *name, PyObject *val);             \
protected:                                                                \
	ClassName(nsISupports *p, const nsIID &iid) :                     \
		Py_nsISupports(p, iid, type) {                            \
		/* The IID _must_ be the IID of the interface we are wrapping! */    \
		NS_ABORT_IF_FALSE(iid.Equals(NS_GET_IID(InterfaceName)), "Bad IID"); \
	}                                                                 \
};                                                                        \
                                                                          \
// End of PyXPCOM_ATTR_INTERFACE_DECLARE macro

#define PyXPCOM_INTERFACE_DEFINE(ClassName, InterfaceName, Methods )      \
PyXPCOM_TypeObject *ClassName::type = NULL;


// And the classes
PyXPCOM_INTERFACE_DECLARE(Py_nsIComponentManager, nsIComponentManagerObsolete, PyMethods_IComponentManager)
PyXPCOM_INTERFACE_DECLARE(Py_nsIInterfaceInfoManager, nsIInterfaceInfoManager, PyMethods_IInterfaceInfoManager)
PyXPCOM_INTERFACE_DECLARE(Py_nsIEnumerator, nsIEnumerator, PyMethods_IEnumerator)
PyXPCOM_INTERFACE_DECLARE(Py_nsISimpleEnumerator, nsISimpleEnumerator, PyMethods_ISimpleEnumerator)
PyXPCOM_INTERFACE_DECLARE(Py_nsIInterfaceInfo, nsIInterfaceInfo, PyMethods_IInterfaceInfo)
PyXPCOM_INTERFACE_DECLARE(Py_nsIServiceManager, nsIServiceManager, PyMethods_IServiceManager)
PyXPCOM_INTERFACE_DECLARE(Py_nsIInputStream, nsIInputStream, PyMethods_IInputStream)
PyXPCOM_ATTR_INTERFACE_DECLARE(Py_nsIClassInfo, nsIClassInfo, PyMethods_IClassInfo)
PyXPCOM_ATTR_INTERFACE_DECLARE(Py_nsIVariant, nsIVariant, PyMethods_IVariant)

#endif // __PYXPCOM_H__
