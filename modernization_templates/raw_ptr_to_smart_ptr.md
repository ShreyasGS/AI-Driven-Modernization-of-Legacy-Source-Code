# Modernization Template: Raw Pointers → Smart Pointers

## Pattern Overview

**Pattern Name**: Raw Pointer to Smart Pointer

**Problem**: Raw pointers require manual memory management, leading to potential memory leaks, use-after-free bugs, and double-delete issues.

**Solution**: Replace raw pointers with appropriate smart pointers that automatically manage object lifetime.

## Before and After Example

### Before (Original Pattern)

```cpp
class Widget {
public:
  Widget() : mName(nullptr), mParent(nullptr) {}
  
  ~Widget() {
    delete mName;
    // Note: mParent is not owned by this class, so we don't delete it
  }
  
  void SetName(const char* aName) {
    delete mName;
    if (aName) {
      mName = new char[strlen(aName) + 1];
      strcpy(mName, aName);
    } else {
      mName = nullptr;
    }
  }
  
  void SetParent(Widget* aParent) {
    mParent = aParent;
  }
  
  const char* GetName() const { return mName; }
  Widget* GetParent() const { return mParent; }
  
private:
  char* mName;     // Owned pointer
  Widget* mParent; // Non-owned pointer
};

// Usage
void ProcessWidget() {
  Widget* widget = new Widget();
  widget->SetName("MyWidget");
  
  // Do something with widget
  
  delete widget; // Must remember to delete
}
```

### After (Modernized Pattern)

```cpp
#include <memory>
#include <string>

class Widget {
public:
  Widget() = default;
  
  // No need for destructor, smart pointers handle cleanup
  
  void SetName(const std::string& aName) {
    mName = aName;
  }
  
  void SetParent(Widget* aParent) {
    mParent = aParent;
  }
  
  const std::string& GetName() const { return mName; }
  Widget* GetParent() const { return mParent; }
  
private:
  std::string mName;       // String manages its own memory
  Widget* mParent = nullptr; // Non-owning pointer, could use std::weak_ptr if parent is also managed by smart pointer
};

// Usage
void ProcessWidget() {
  auto widget = std::make_unique<Widget>();
  widget->SetName("MyWidget");
  
  // Do something with widget
  
  // No need to delete, std::unique_ptr handles cleanup
}
```

## XPCOM-Specific Example

### Before (Original XPCOM Pattern)

```cpp
nsresult CreateDocument(nsIContent** aResult) {
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = nullptr;
  
  nsIContent* content = new nsXMLDocument();
  if (!content) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult rv = content->Init();
  if (NS_FAILED(rv)) {
    delete content;
    return rv;
  }
  
  NS_ADDREF(*aResult = content);
  return NS_OK;
}

// Usage
nsIContent* doc = nullptr;
nsresult rv = CreateDocument(&doc);
if (NS_SUCCEEDED(rv)) {
  // Use doc
  NS_RELEASE(doc);
}
```

### After (Modernized XPCOM Pattern)

```cpp
#include "mozilla/RefPtr.h"

nsresult CreateDocument(nsIContent** aResult) {
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = nullptr;
  
  RefPtr<nsIContent> content = new nsXMLDocument();
  if (!content) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsresult rv = content->Init();
  if (NS_FAILED(rv)) {
    return rv; // RefPtr handles cleanup
  }
  
  content.forget(aResult);
  return NS_OK;
}

// Usage
RefPtr<nsIContent> doc;
nsresult rv = CreateDocument(getter_AddRefs(doc));
if (NS_SUCCEEDED(rv)) {
  // Use doc
  // No need for NS_RELEASE, RefPtr handles cleanup
}
```

## Smart Pointer Selection Guide

Choose the appropriate smart pointer based on ownership semantics:

1. **std::unique_ptr**: For exclusive ownership (only one owner at a time)
   - Use when the object is owned by a single entity
   - Cannot be copied, only moved
   - Automatically deletes the object when the unique_ptr goes out of scope

2. **RefPtr/nsCOMPtr**: For XPCOM objects with reference counting
   - Use for any XPCOM object that implements AddRef/Release
   - Automatically calls AddRef when constructed and Release when destroyed
   - Can be copied (increases reference count) and moved

3. **std::shared_ptr**: For shared ownership (multiple owners)
   - Use when multiple entities need to own the object
   - Reference counted, deletes object when last shared_ptr is destroyed
   - More overhead than unique_ptr, use only when necessary

4. **Raw Pointer/std::weak_ptr**: For non-owning references
   - Use when you need to reference an object but don't own it
   - Does not affect object lifetime
   - Must ensure the referenced object outlives the pointer

## Step-by-Step Implementation Guide

1. **Identify Ownership Semantics**:
   - Determine if the pointer represents ownership (responsible for deletion)
   - Identify if ownership is exclusive or shared
   - For XPCOM objects, check if reference counting is used

2. **Choose the Right Smart Pointer**:
   - For exclusive ownership: `std::unique_ptr`
   - For XPCOM objects: `RefPtr` or `nsCOMPtr`
   - For shared ownership: `std::shared_ptr`
   - For non-owning references: raw pointer or `std::weak_ptr`

3. **Replace Raw Pointer Declarations**:
   - Change `Type* ptr` to `std::unique_ptr<Type> ptr` or appropriate smart pointer
   - Initialize with `nullptr` or use constructor with initial value

4. **Update Resource Acquisition**:
   - Replace `new` with `std::make_unique<Type>()` or similar
   - For XPCOM objects, use `RefPtr<Type> = new Type()` or `do_CreateInstance`

5. **Remove Manual Cleanup**:
   - Remove `delete` calls (smart pointers handle cleanup)
   - Remove destructor if it only contains delete operations

6. **Update Parameter Passing**:
   - For functions taking ownership: pass by value or std::move
   - For functions borrowing: pass by reference or raw pointer
   - For out parameters: use appropriate getter methods (e.g., `getter_AddRefs`)

7. **Consider Container Classes**:
   - Replace raw arrays with `std::vector` or similar containers
   - Replace string buffers with `std::string` or `nsCString`

## Compatibility Considerations

1. **Performance**:
   - Smart pointers have minimal overhead compared to manual memory management
   - RefPtr/nsCOMPtr are optimized for XPCOM's reference counting model

2. **Exception Safety**:
   - Smart pointers provide strong exception safety guarantees
   - Resources are automatically cleaned up if an exception occurs

3. **Interface Compatibility**:
   - Consider providing compatibility functions for code that expects raw pointers
   - Use getter methods to safely extract raw pointers when needed

4. **Thread Safety**:
   - Most smart pointers are not thread-safe by default
   - For thread-safe reference counting, use appropriate atomic operations

## Benefits

1. **Memory Safety**: Eliminates memory leaks and use-after-free bugs
2. **Exception Safety**: Resources are properly cleaned up even if exceptions occur
3. **Code Clarity**: Ownership semantics are explicitly encoded in the type system
4. **Maintainability**: Reduces boilerplate memory management code
5. **Modern C++**: Aligns with modern C++ best practices 