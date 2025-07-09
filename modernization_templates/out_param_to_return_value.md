# Modernization Template: Out Parameters → Return Values

## Pattern Overview

**Pattern Name**: Out Parameter to Return Value

**Problem**: Functions use out parameters (pointers or references that are modified by the function) to return values, making the API less intuitive, error-prone, and harder to use with modern C++ features.

**Solution**: Replace out parameters with direct return values or, for multiple returns, with structured return types (tuples, structs, or Result types).

## Before and After Example

### Before (Original Pattern)

```cpp
bool GetUserInfo(const nsAString& aUserID, nsAString* aName, int32_t* aAge, bool* aIsActive) {
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(aAge);
  NS_ENSURE_ARG_POINTER(aIsActive);
  
  if (aUserID.IsEmpty()) {
    return false;
  }
  
  // Fetch user info from database
  *aName = GetUserName(aUserID);
  *aAge = GetUserAge(aUserID);
  *aIsActive = IsUserActive(aUserID);
  
  return true;
}

// Usage
nsAutoString name;
int32_t age;
bool isActive;
if (GetUserInfo(userID, &name, &age, &isActive)) {
  // Use name, age, isActive
} else {
  // Handle error
}
```

### After (Modernized Pattern)

```cpp
struct UserInfo {
  nsString name;
  int32_t age;
  bool isActive;
};

std::optional<UserInfo> GetUserInfo(const nsAString& aUserID) {
  if (aUserID.IsEmpty()) {
    return std::nullopt;
  }
  
  // Fetch user info from database
  UserInfo info;
  info.name = GetUserName(aUserID);
  info.age = GetUserAge(aUserID);
  info.isActive = IsUserActive(aUserID);
  
  return info;
}

// Usage
std::optional<UserInfo> userInfo = GetUserInfo(userID);
if (userInfo) {
  // Use userInfo->name, userInfo->age, userInfo->isActive
} else {
  // Handle error
}
```

### Alternative with std::tuple (for simpler cases)

```cpp
std::optional<std::tuple<nsString, int32_t, bool>> GetUserInfo(const nsAString& aUserID) {
  if (aUserID.IsEmpty()) {
    return std::nullopt;
  }
  
  // Fetch user info from database
  nsString name = GetUserName(aUserID);
  int32_t age = GetUserAge(aUserID);
  bool isActive = IsUserActive(aUserID);
  
  return std::make_tuple(std::move(name), age, isActive);
}

// Usage
auto userInfoResult = GetUserInfo(userID);
if (userInfoResult) {
  auto [name, age, isActive] = *userInfoResult;
  // Use name, age, isActive
} else {
  // Handle error
}
```

## XPCOM-Specific Example

### Before (Original XPCOM Pattern)

```cpp
nsresult GetElement(nsIDocument* aDocument, const nsAString& aID, nsIContent** aResult) {
  NS_ENSURE_ARG_POINTER(aDocument);
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = nullptr;
  
  if (aID.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }
  
  nsIContent* content = aDocument->GetElementById(aID);
  if (!content) {
    return NS_ERROR_FAILURE;
  }
  
  NS_ADDREF(*aResult = content);
  return NS_OK;
}

// Usage
nsIContent* content = nullptr;
nsresult rv = GetElement(doc, id, &content);
if (NS_FAILED(rv)) {
  // Handle error
  return rv;
}
// Use content...
NS_IF_RELEASE(content);
```

### After (Modernized XPCOM Pattern)

```cpp
// Using Result type from error_code_result_type.md template
mozilla::Result<RefPtr<nsIContent>> GetElement(nsIDocument* aDocument, const nsAString& aID) {
  if (!aDocument) {
    return mozilla::Result<RefPtr<nsIContent>>::Err(NS_ERROR_INVALID_ARG);
  }
  
  if (aID.IsEmpty()) {
    return mozilla::Result<RefPtr<nsIContent>>::Err(NS_ERROR_INVALID_ARG);
  }
  
  RefPtr<nsIContent> content = aDocument->GetElementById(aID);
  if (!content) {
    return mozilla::Result<RefPtr<nsIContent>>::Err(NS_ERROR_FAILURE);
  }
  
  return mozilla::Result<RefPtr<nsIContent>>::Ok(std::move(content));
}

// Usage
mozilla::Result<RefPtr<nsIContent>> result = GetElement(doc, id);
if (!result.IsOk()) {
  // Handle error
  return result.ErrorCode();
}
// Use result.Value()...
```

## Return Type Selection Guide

Choose the appropriate return type based on the function's needs:

1. **Single Return Value**
   - Use direct return: `Type Function()`
   - For nullable/optional values: `std::optional<Type> Function()`
   - For XPCOM objects: `RefPtr<Type> Function()`

2. **Return Value with Error Code**
   - Use Result type: `Result<Type> Function()`
   - For XPCOM: `Result<RefPtr<Type>> Function()`

3. **Multiple Return Values**
   - For 2-3 related values: `std::tuple<Type1, Type2, Type3> Function()`
   - For more complex returns: Create a struct/class: `ReturnStruct Function()`
   - For multiple values with error code: `Result<ReturnStruct> Function()`

4. **Collection Return Values**
   - Return appropriate container: `std::vector<Type> Function()`
   - For optional collections: `std::optional<std::vector<Type>> Function()`

## Step-by-Step Implementation Guide

1. **Identify Out Parameters**:
   - Look for pointer or reference parameters that are modified by the function
   - Focus on functions with multiple out parameters
   - Pay special attention to XPCOM-style out parameters (`Type**`)

2. **Design the Return Type**:
   - For single out parameter: Use direct return or std::optional
   - For multiple out parameters: Create a struct or use std::tuple
   - For error codes + return value: Use Result type

3. **Transform the Function**:
   - Change the function signature to return the new type
   - Remove out parameters
   - Replace assignments to out parameters with construction of return value
   - Replace error returns with appropriate return value (std::nullopt, Result::Err, etc.)

4. **Update Calling Code**:
   - Replace out parameter declarations with variables to hold return values
   - Update function calls to assign return value to variables
   - Update error checking logic

5. **Create Compatibility Layer (Optional)**:
   - Keep the original function signature for backward compatibility
   - Implement it by calling the modern function and extracting values to out parameters

## Compatibility Considerations

1. **Performance**:
   - Modern compilers optimize return values effectively (Return Value Optimization)
   - For large objects, consider returning by value with move semantics
   - For very large objects, consider returning by `std::unique_ptr`

2. **Error Handling**:
   - Replace error code returns with Result type or std::optional
   - Ensure error information is preserved
   - Consider adding more specific error types

3. **XPCOM Compatibility**:
   - Use RefPtr for XPCOM objects
   - Consider reference counting implications
   - Provide compatibility functions for legacy code

4. **ABI Compatibility**:
   - Changing function signatures affects ABI
   - Keep compatibility functions until all callers are updated
   - Consider using `MOZ_DEPRECATED` to mark legacy functions

## Benefits

1. **Clarity**: Function signatures clearly indicate what is being returned
2. **Safety**: Eliminates the possibility of null out parameters
3. **Usability**: Makes functions easier to use with modern C++ features (auto, structured bindings)
4. **Composability**: Return values can be directly passed to other functions
5. **Modern C++**: Aligns with modern C++ practices and idioms 