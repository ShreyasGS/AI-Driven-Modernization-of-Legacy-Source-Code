/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-  */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _ORKINFACTORY_
#define _ORKINFACTORY_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#define morkMagic_kFactory 0x46616374 /* ascii 'Fact' */

/*| orkinFactory: 
|*/
class orkinFactory : public morkHandle, public nsIMdbFactory { // nsIMdbObject

// { ===== begin morkNode interface =====
public: // morkNode virtual methods
  // virtual void CloseMorkNode(morkEnv* ev); // morkHandle is fine
  virtual ~orkinFactory(); // morkHandle destructor does everything
  
protected: // construction is protected (use the static Make() method)
  orkinFactory(morkEnv* ev, // note morkUsage is always morkUsage_kPool
    morkHandleFace* ioFace,    // must not be nil, cookie for this handle
    morkFactory* ioObject); // must not be nil, the object for this handle
    
  // void CloseHandle(morkEnv* ev); // don't need to specialize closing

private: // copying is not allowed
  orkinFactory(const morkHandle& other);
  orkinFactory& operator=(const morkHandle& other);

// public: // dynamic type identification
  // mork_bool IsHandle() const //
  // { return IsNode() && mNode_Derived == morkDerived_kHandle; }
// } ===== end morkNode methods =====

protected: // morkHandle memory management operators
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev)
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev)
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace)
  { MORK_USED_1(inSize); return ioFace; }
  
 
public: // construction:

  static orkinFactory* MakeGlobalFactory();
  // instantiate objects using almost no context information.
  
  static orkinFactory* MakeFactory(morkEnv* ev, morkFactory* ioObject);

public: // utilities:

  morkEnv* CanUseFactory(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;
    
  morkEnv* GetInternalFactoryEnv(mdb_err* outErr);
  
  mork_bool CanOpenMorkTextFile(morkEnv* ev,
    // const mdbYarn* inFirst512Bytes,
    nsIMdbFile* ioFile);

public: // type identification
  mork_bool IsOrkinFactory() const
  { return mHandle_Magic == morkMagic_kFactory; }

  mork_bool IsOrkinFactoryHandle() const
  { return this->IsHandle() && this->IsOrkinFactory(); }
  
public:

  NS_DECL_ISUPPORTS

// { ===== begin nsIMdbObject methods =====

  // { ----- begin attribute methods -----
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  // same as nsIMdbPort::GetIsPortReadonly() when this object is inside a port.
  // } ----- end attribute methods -----

  // { ----- begin factory methods -----
  NS_IMETHOD GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  // } ----- end factory methods -----

  // { ----- begin ref counting for well-behaved cyclic graphs -----
  NS_IMETHOD GetWeakRefCount(nsIMdbEnv* ev, // weak refs
    mdb_count* outCount);  
  NS_IMETHOD GetStrongRefCount(nsIMdbEnv* ev, // strong refs
    mdb_count* outCount);

  NS_IMETHOD AddWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev);

  NS_IMETHOD CutWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev);
  
  NS_IMETHOD CloseMdbObject(nsIMdbEnv* ev); // called at strong refs zero
  NS_IMETHOD IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  // } ----- end ref counting -----
  
// } ===== end nsIMdbObject methods =====

// { ===== begin nsIMdbFactory methods =====

  // { ----- begin file methods -----
  NS_IMETHOD OpenOldFile(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath,
    mdb_bool inFrozen, nsIMdbFile** acqFile);
  // Choose some subclass of nsIMdbFile to instantiate, in order to read
  // (and write if not frozen) the file known by inFilePath.  The file
  // returned should be open and ready for use, and presumably positioned
  // at the first byte position of the file.  The exact manner in which
  // files must be opened is considered a subclass specific detail, and
  // other portions or Mork source code don't want to know how it's done.

  NS_IMETHOD CreateNewFile(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath,
    nsIMdbFile** acqFile);
  // Choose some subclass of nsIMdbFile to instantiate, in order to read
  // (and write if not frozen) the file known by inFilePath.  The file
  // returned should be created and ready for use, and presumably positioned
  // at the first byte position of the file.  The exact manner in which
  // files must be opened is considered a subclass specific detail, and
  // other portions or Mork source code don't want to know how it's done.
  // } ----- end file methods -----

  // { ----- begin env methods -----
  NS_IMETHOD MakeEnv(nsIMdbHeap* ioHeap, nsIMdbEnv** acqEnv); // new env
  // ioHeap can be nil, causing a MakeHeap() style heap instance to be used
  // } ----- end env methods -----

  // { ----- begin heap methods -----
  NS_IMETHOD MakeHeap(nsIMdbEnv* ev, nsIMdbHeap** acqHeap); // new heap
  // } ----- end heap methods -----

  // { ----- begin compare methods -----
  NS_IMETHOD MakeCompare(nsIMdbEnv* ev, nsIMdbCompare** acqCompare); // ASCII
  // } ----- end compare methods -----

  // { ----- begin row methods -----
  NS_IMETHOD MakeRow(nsIMdbEnv* ev, nsIMdbHeap* ioHeap, nsIMdbRow** acqRow); // new row
  // ioHeap can be nil, causing the heap associated with ev to be used
  // } ----- end row methods -----
  
  // { ----- begin port methods -----
  NS_IMETHOD CanOpenFilePort(
    nsIMdbEnv* ev, // context
    // const char* inFilePath, // the file to investigate
    // const mdbYarn* inFirst512Bytes,
    nsIMdbFile* ioFile, // db abstract file interface
    mdb_bool* outCanOpen, // whether OpenFilePort() might succeed
    mdbYarn* outFormatVersion); // informal file format description
    
  NS_IMETHOD OpenFilePort(
    nsIMdbEnv* ev, // context
    nsIMdbHeap* ioHeap, // can be nil to cause ev's heap attribute to be used
    // const char* inFilePath, // the file to open for readonly import
    nsIMdbFile* ioFile, // db abstract file interface
    const mdbOpenPolicy* inOpenPolicy, // runtime policies for using db
    nsIMdbThumb** acqThumb); // acquire thumb for incremental port open
  // Call nsIMdbThumb::DoMore() until done, or until the thumb is broken, and
  // then call nsIMdbFactory::ThumbToOpenPort() to get the port instance.

  NS_IMETHOD ThumbToOpenPort( // redeeming a completed thumb from OpenFilePort()
    nsIMdbEnv* ev, // context
    nsIMdbThumb* ioThumb, // thumb from OpenFilePort() with done status
    nsIMdbPort** acqPort); // acquire new port object
  // } ----- end port methods -----
  
  // { ----- begin store methods -----
  NS_IMETHOD CanOpenFileStore(
    nsIMdbEnv* ev, // context
    // const char* inFilePath, // the file to investigate
    // const mdbYarn* inFirst512Bytes,
    nsIMdbFile* ioFile, // db abstract file interface
    mdb_bool* outCanOpenAsStore, // whether OpenFileStore() might succeed
    mdb_bool* outCanOpenAsPort, // whether OpenFilePort() might succeed
    mdbYarn* outFormatVersion); // informal file format description
    
  NS_IMETHOD OpenFileStore( // open an existing database
    nsIMdbEnv* ev, // context
    nsIMdbHeap* ioHeap, // can be nil to cause ev's heap attribute to be used
    // const char* inFilePath, // the file to open for general db usage
    nsIMdbFile* ioFile, // db abstract file interface
    const mdbOpenPolicy* inOpenPolicy, // runtime policies for using db
    nsIMdbThumb** acqThumb); // acquire thumb for incremental store open
  // Call nsIMdbThumb::DoMore() until done, or until the thumb is broken, and
  // then call nsIMdbFactory::ThumbToOpenStore() to get the store instance.
    
  NS_IMETHOD
  ThumbToOpenStore( // redeem completed thumb from OpenFileStore()
    nsIMdbEnv* ev, // context
    nsIMdbThumb* ioThumb, // thumb from OpenFileStore() with done status
    nsIMdbStore** acqStore); // acquire new db store object
  
  NS_IMETHOD CreateNewFileStore( // create a new db with minimal content
    nsIMdbEnv* ev, // context
    nsIMdbHeap* ioHeap, // can be nil to cause ev's heap attribute to be used
    // const char* inFilePath, // name of file which should not yet exist
    nsIMdbFile* ioFile, // db abstract file interface
    const mdbOpenPolicy* inOpenPolicy, // runtime policies for using db
    nsIMdbStore** acqStore); // acquire new db store object
  // } ----- end store methods -----

// } ===== end nsIMdbFactory methods =====
};

//3456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789

#endif /* _ORKINFACTORY_ */
