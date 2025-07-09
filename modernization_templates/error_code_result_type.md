# Modernization Template: Error Code Returns → Result Types

## Pattern Overview

**Pattern Name**: Error Code Return to Result Type

**Problem**: Functions return error codes (NS_OK, NS_ERROR_*) and use out parameters for actual return values, making error handling verbose and error-prone.

**Solution**: Replace error code returns and out parameters with a Result type that combines both the return value and error information.

## Before and After Example

### Before (Original Pattern)

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

### After (Modernized Pattern)

```cpp
namespace mozilla {

template<typename T>
class Result {
public:
  // Success constructor
  static Result<T> Ok(T&& value) {
    return Result(std::forward<T>(value), NS_OK);
  }
  
  // Error constructor
  static Result<T> Err(nsresult aErrorCode) {
    return Result(T(), aErrorCode);
  }
  
  // Check if result is successful
  bool IsOk() const { return NS_SUCCEEDED(mErrorCode); }
  
  // Get the value (only call if IsOk() returns true)
  const T& Value() const {
    MOZ_ASSERT(IsOk(), "Accessing value of error result");
    return mValue;
  }
  
  // Get the value (only call if IsOk() returns true)
  T&& Unwrap() && {
    MOZ_ASSERT(IsOk(), "Unwrapping value of error result");
    return std::move(mValue);
  }
  
  // Get the error code
  nsresult ErrorCode() const { return mErrorCode; }
  
private:
  Result(T&& aValue, nsresult aErrorCode)
    : mValue(std::forward<T>(aValue))
    , mErrorCode(aErrorCode)
  {}
  
  T mValue;
  nsresult mErrorCode;
};

// Specialization for void
template<>
class Result<void> {
public:
  static Result<void> Ok() {
    return Result(NS_OK);
  }
  
  static Result<void> Err(nsresult aErrorCode) {
    return Result(aErrorCode);
  }
  
  bool IsOk() const { return NS_SUCCEEDED(mErrorCode); }
  nsresult ErrorCode() const { return mErrorCode; }
  
private:
  explicit Result(nsresult aErrorCode)
    : mErrorCode(aErrorCode)
  {}
  
  nsresult mErrorCode;
};

// Modern implementation
Result<RefPtr<nsIContent>> GetElement(nsIDocument* aDocument, const nsAString& aID) {
  if (!aDocument) {
    return Result<RefPtr<nsIContent>>::Err(NS_ERROR_INVALID_ARG);
  }
  
  if (aID.IsEmpty()) {
    return Result<RefPtr<nsIContent>>::Err(NS_ERROR_INVALID_ARG);
  }
  
  RefPtr<nsIContent> content = aDocument->GetElementById(aID);
  if (!content) {
    return Result<RefPtr<nsIContent>>::Err(NS_ERROR_FAILURE);
  }
  
  return Result<RefPtr<nsIContent>>::Ok(std::move(content));
}

// Usage
Result<RefPtr<nsIContent>> result = GetElement(doc, id);
if (!result.IsOk()) {
  // Handle error
  return result.ErrorCode();
}
// Use result.Value()...
} // namespace mozilla
```

## Compatibility Layer

To maintain compatibility with existing code, provide a legacy function that uses the modern implementation:

```cpp
// Legacy compatibility function
nsresult GetElement(nsIDocument* aDocument, const nsAString& aID, nsIContent** aResult) {
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;
  
  mozilla::Result<RefPtr<nsIContent>> result = mozilla::GetElement(aDocument, aID);
  if (!result.IsOk()) {
    return result.ErrorCode();
  }
  
  RefPtr<nsIContent> content = result.Value();
  NS_ADDREF(*aResult = content);
  return NS_OK;
}
```

## Step-by-Step Implementation Guide

1. **Create the Result Template Class**:
   - Implement the `Result<T>` template class in a common header file
   - Add specialization for `void` return type
   - Include proper move semantics and reference counting support

2. **Identify Functions to Modernize**:
   - Look for functions that return `nsresult`
   - Focus on functions with out parameters (especially `**` parameters)
   - Prioritize functions with many error checks and returns

3. **Create the Modern Implementation**:
   - Change the return type to `Result<T>` where T is the type of the out parameter
   - Replace out parameters with return values
   - Replace error returns with `Result<T>::Err(error_code)`
   - Replace success returns with `Result<T>::Ok(value)`
   - Use smart pointers (RefPtr, nsCOMPtr) for XPCOM objects

4. **Create a Compatibility Layer**:
   - Keep the original function signature for backward compatibility
   - Implement it by calling the modern function and converting the result
   - Ensure proper reference counting in the compatibility layer

5. **Update Calling Code (Optional)**:
   - Gradually update calling code to use the modern API
   - Replace error checking with `if (!result.IsOk())` pattern
   - Use `result.Value()` to access the return value

## Compatibility Considerations

1. **Reference Counting**:
   - Ensure proper reference counting when converting between APIs
   - Use RefPtr/nsCOMPtr in the Result type to manage references automatically

2. **Error Propagation**:
   - The Result type should preserve all error codes from the original function
   - Error handling should be as easy or easier than with the original pattern

3. **Performance**:
   - The Result type should have minimal overhead compared to the original pattern
   - Consider using move semantics to avoid unnecessary copies

4. **Gradual Migration**:
   - Keep the legacy functions until all callers are updated
   - Consider using `MOZ_DEPRECATED` to mark legacy functions once migration is complete

## Benefits

1. **Safety**: Eliminates the possibility of forgetting to check error codes
2. **Clarity**: Makes the relationship between return values and errors explicit
3. **Maintainability**: Reduces boilerplate error handling code
4. **Consistency**: Provides a uniform pattern for error handling
5. **Modernization**: Aligns with modern C++ practices 