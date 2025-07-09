# Modernization Template: Manual Reference Counting → Smart Pointers

## Pattern Overview

**Pattern Name**: Manual Reference Counting to Smart Pointers

**Problem**: Manual reference counting (AddRef/Release calls) is error-prone, leading to potential memory leaks or premature object destruction if reference counts are not properly managed.

**Solution**: Replace manual AddRef/Release calls with XPCOM smart pointers (RefPtr, nsCOMPtr) that automatically manage reference counting.

## Before and After Example

### Before (Original Pattern)

```cpp
void ProcessDocument() {
  nsIDocument* doc = GetDocument();
  if (!doc) {
    return;
  }
  
  NS_ADDREF(doc); // Manually increment reference count
  
  nsIContent* rootContent = doc->GetRootContent();
  if (rootContent) {
    NS_ADDREF(rootContent); // Manually increment reference count
    
    // Process the root content
    ProcessContent(rootContent);
    
    NS_RELEASE(rootContent); // Manually decrement reference count
  }
  
  NS_RELEASE(doc); // Manually decrement reference count
}

nsresult GetChildElement(nsIElement* aParent, uint32_t aIndex, nsIElement** aChild) {
  NS_ENSURE_ARG_POINTER(aParent);
  NS_ENSURE_ARG_POINTER(aChild);
  
  *aChild = nullptr;
  
  if (aIndex >= aParent->GetChildCount()) {
    return NS_ERROR_INVALID_ARG;
  }
  
  nsINode* node = aParent->GetChildAt(aIndex);
  if (!node || !node->IsElement()) {
    return NS_ERROR_FAILURE;
  }
  
  nsIElement* element = node->AsElement();
  NS_ADDREF(*aChild = element);
  
  return NS_OK;
}

// Usage
nsIElement* child = nullptr;
nsresult rv = GetChildElement(parent, 0, &child);
if (NS_SUCCEEDED(rv)) {
  // Use child
  NS_RELEASE(child);
}
```

### After (Modernized Pattern)

```cpp
#include "mozilla/RefPtr.h"
#include "nsCOMPtr.h"

void ProcessDocument() {
  nsCOMPtr<nsIDocument> doc = GetDocument();
  if (!doc) {
    return;
  }
  
  // No need for NS_ADDREF
  
  nsCOMPtr<nsIContent> rootContent = doc->GetRootContent();
  if (rootContent) {
    // No need for NS_ADDREF
    
    // Process the root content
    ProcessContent(rootContent);
    
    // No need for NS_RELEASE
  }
  
  // No need for NS_RELEASE
}

nsresult GetChildElement(nsIElement* aParent, uint32_t aIndex, nsIElement** aChild) {
  NS_ENSURE_ARG_POINTER(aParent);
  NS_ENSURE_ARG_POINTER(aChild);
  
  *aChild = nullptr;
  
  if (aIndex >= aParent->GetChildCount()) {
    return NS_ERROR_INVALID_ARG;
  }
  
  nsCOMPtr<nsINode> node = aParent->GetChildAt(aIndex);
  if (!node || !node->IsElement()) {
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIElement> element = node->AsElement();
  element.forget(aChild);
  
  return NS_OK;
}

// Usage
nsCOMPtr<nsIElement> child;
nsresult rv = GetChildElement(parent, 0, getter_AddRefs(child));
if (NS_SUCCEEDED(rv)) {
  // Use child
  // No need for NS_RELEASE
}
```

### Even Better with Result Type

```cpp
mozilla::Result<RefPtr<nsIElement>> GetChildElement(nsIElement* aParent, uint32_t aIndex) {
  if (!aParent) {
    return mozilla::Result<RefPtr<nsIElement>>::Err(NS_ERROR_INVALID_ARG);
  }
  
  if (aIndex >= aParent->GetChildCount()) {
    return mozilla::Result<RefPtr<nsIElement>>::Err(NS_ERROR_INVALID_ARG);
  }
  
  RefPtr<nsINode> node = aParent->GetChildAt(aIndex);
  if (!node || !node->IsElement()) {
    return mozilla::Result<RefPtr<nsIElement>>::Err(NS_ERROR_FAILURE);
  }
  
  RefPtr<nsIElement> element = node->AsElement();
  return mozilla::Result<RefPtr<nsIElement>>::Ok(std::move(element));
}

// Usage
mozilla::Result<RefPtr<nsIElement>> result = GetChildElement(parent, 0);
if (result.IsOk()) {
  RefPtr<nsIElement> child = result.Unwrap();
  // Use child
  // No need for NS_RELEASE
}
```

## Smart Pointer Selection Guide

XPCOM provides two main smart pointer types for reference counting:

1. **RefPtr<T>**
   - Template class for managing XPCOM object references
   - Automatically calls AddRef in constructor and Release in destructor
   - Supports move semantics
   - Preferred for function returns and class member variables
   - Similar to std::shared_ptr but optimized for XPCOM

2. **nsCOMPtr<T>**
   - Similar to RefPtr but with additional XPCOM-specific features
   - Provides helper methods like `do_QueryInterface`
   - Has specialized assignment operators for XPCOM patterns
   - Supports `getter_AddRefs` for out parameters
   - Historically more common in Mozilla codebase

## Step-by-Step Implementation Guide

1. **Identify Manual Reference Counting**:
   - Look for explicit NS_ADDREF/NS_RELEASE calls
   - Look for AddRef()/Release() method calls
   - Pay attention to out parameters with AddRef patterns

2. **Choose the Appropriate Smart Pointer**:
   - For local variables: Use `nsCOMPtr<T>` or `RefPtr<T>`
   - For member variables: Use `RefPtr<T>` (generally preferred)
   - For function returns: Use `already_AddRefed<T>` or `RefPtr<T>`

3. **Replace Variable Declarations**:
   - Change `T* ptr` to `nsCOMPtr<T> ptr` or `RefPtr<T> ptr`
   - Remove explicit AddRef/Release calls

4. **Update Function Parameters**:
   - For out parameters: Use `getter_AddRefs` pattern
   - Example: `GetObject(getter_AddRefs(ptr))` instead of `GetObject(&ptr)`

5. **Update Return Values**:
   - For returning AddRef'd objects: Use `already_AddRefed<T>` or `RefPtr<T>`
   - Use `forget()` to transfer ownership without changing reference count

6. **Handle Temporary Objects**:
   - Replace temporary AddRef/Release patterns with smart pointer assignment
   - Use move semantics for efficiency when appropriate

7. **Consider Result Type Integration**:
   - Combine with Result type pattern for functions that return objects and error codes
   - Return `Result<RefPtr<T>>` instead of using out parameters

## Compatibility Considerations

1. **Performance**:
   - Smart pointers have minimal overhead compared to manual reference counting
   - In performance-critical code, verify that the overhead is acceptable

2. **Existing Code Integration**:
   - Provide overloads or wrapper functions for code that expects raw pointers
   - Use `get()` method to obtain raw pointer when needed (without changing reference count)
   - Use `forget()` to extract raw pointer and transfer ownership

3. **Debugging**:
   - Smart pointers make reference counting issues easier to debug
   - Consider adding assertions or logging in debug builds

4. **Cycle Collection**:
   - Be aware that RefPtr and nsCOMPtr don't handle reference cycles
   - For cyclic references, consider using nsCycleCollectingAutoRefCnt

## Common Patterns

### Pattern 1: Local Variable with AddRef/Release

Before:
```cpp
nsIDocument* doc = GetDocument();
if (doc) {
  NS_ADDREF(doc);
  // Use doc
  NS_RELEASE(doc);
}
```

After:
```cpp
nsCOMPtr<nsIDocument> doc = GetDocument();
if (doc) {
  // Use doc
  // No need for AddRef/Release
}
```

### Pattern 2: Out Parameter with AddRef

Before:
```cpp
nsresult rv = GetDocument(&doc);
if (NS_SUCCEEDED(rv)) {
  // doc already AddRef'd by GetDocument
  // Use doc
  NS_RELEASE(doc);
}
```

After:
```cpp
nsCOMPtr<nsIDocument> doc;
nsresult rv = GetDocument(getter_AddRefs(doc));
if (NS_SUCCEEDED(rv)) {
  // Use doc
  // No need for Release
}
```

### Pattern 3: Returning AddRef'd Object

Before:
```cpp
nsIDocument* CreateDocument() {
  nsIDocument* doc = new nsXMLDocument();
  NS_ADDREF(doc);
  return doc;
}
```

After:
```cpp
already_AddRefed<nsIDocument> CreateDocument() {
  RefPtr<nsIDocument> doc = new nsXMLDocument();
  return doc.forget();
}

// Or even better
RefPtr<nsIDocument> CreateDocument() {
  return new nsXMLDocument();
}
```

### Pattern 4: Setting Out Parameter

Before:
```cpp
NS_ADDREF(*aResult = mDocument);
```

After:
```cpp
nsCOMPtr<nsIDocument> document = mDocument;
document.forget(aResult);
```

## Benefits

1. **Memory Safety**: Eliminates reference counting errors (leaks and premature frees)
2. **Exception Safety**: Resources are properly released even if exceptions occur
3. **Code Clarity**: Ownership semantics are more explicit
4. **Maintainability**: Reduces boilerplate reference counting code
5. **Modern C++**: Aligns with modern C++ resource management practices 