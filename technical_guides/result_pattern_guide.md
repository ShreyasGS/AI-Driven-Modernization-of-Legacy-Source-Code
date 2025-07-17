# Technical Guide: Implementing the Result<T> Pattern in Mozilla 1.0

## Introduction

This technical guide provides a detailed explanation of how to implement the Result<T> pattern in the Mozilla 1.0 codebase. The Result<T> pattern is a modern error handling approach that replaces traditional error codes with a type that can represent either a successful result or an error.

## Background

### Traditional Error Handling in Mozilla 1.0

Mozilla 1.0 primarily uses `nsresult` error codes for error handling. This approach has several drawbacks:

1. **Error Propagation**: Error codes can be ignored or lost during propagation.
2. **Type Safety**: Return values and error codes share the same return channel.
3. **Readability**: Error handling code can be verbose and obscure the main logic.
4. **Maintainability**: Error handling is often scattered throughout the code.

### The Result<T> Pattern

The Result<T> pattern addresses these issues by:

1. **Explicit Error Handling**: Errors must be explicitly handled or propagated.
2. **Type Safety**: Success values and errors are properly typed.
3. **Improved Readability**: Clear distinction between success and error paths.
4. **Better Maintainability**: Consistent error handling patterns.

## Implementation Details

### The Result<T> Type

We've implemented a simplified Result<T> type in `modernized_nsSelection_Result.h`:

```cpp
template<typename T>
class Result {
public:
  // Construct a successful Result with a value
  explicit Result(const T& aValue) : mValue(aValue), mIsOk(true) {}
  explicit Result(T&& aValue) : mValue(std::move(aValue)), mIsOk(true) {}

  // Construct a failed Result with an error code
  explicit Result(nsresult aErrorCode) : mErrorCode(aErrorCode), mIsOk(false) {}

  // Check if the Result represents success
  bool isOk() const { return mIsOk; }

  // Get the value (only call if isOk() is true)
  const T& unwrap() const {
    NS_ASSERTION(mIsOk, "Trying to unwrap a failed Result");
    return mValue;
  }

  T& unwrap() {
    NS_ASSERTION(mIsOk, "Trying to unwrap a failed Result");
    return mValue;
  }

  // Get the error code (only call if isOk() is false)
  nsresult unwrapErr() const {
    NS_ASSERTION(!mIsOk, "Trying to unwrapErr a successful Result");
    return mErrorCode;
  }

  // Convert to nsresult for backward compatibility
  operator nsresult() const {
    return mIsOk ? NS_OK : mErrorCode;
  }

private:
  union {
    T mValue;
    nsresult mErrorCode;
  };
  bool mIsOk;
};
```

### Helper Functions

We've also implemented helper functions to create Result objects:

```cpp
// Create a successful Result
template<typename T>
Result<T> Ok(T&& value) {
  return Result<T>(std::forward<T>(value));
}

// Create a failed Result
template<typename E>
Result<E> Err(nsresult error) {
  return Result<E>(error);
}
```

## Step-by-Step Guide to Applying the Result<T> Pattern

### Step 1: Identify Functions to Modernize

Look for functions that:
- Return `nsresult` error codes
- Use out parameters to return values
- Have multiple error paths
- Are called by other functions that could benefit from modern error handling

### Step 2: Design the Modernized Function Signature

Convert the original function signature to use Result<T>:

**Original:**
```cpp
nsresult GetSomething(Type** aOutParam);
```

**Modernized:**
```cpp
Result<nsCOMPtr<Type>, nsresult> GetSomethingModern();
```

### Step 3: Implement the Modernized Function

1. **Handle Input Validation**:
   ```cpp
   Result<nsCOMPtr<Type>, nsresult> GetSomethingModern() {
     if (!mSomeRequiredMember) {
       return Err(NS_ERROR_NOT_INITIALIZED);
     }
   ```

2. **Perform the Operation**:
   ```cpp
   nsCOMPtr<Type> result;
   nsresult rv = SomeOperation(getter_AddRefs(result));
   if (NS_FAILED(rv)) {
     return Err(rv);
   }
   ```

3. **Return Success**:
   ```cpp
   return Ok(result);
   }
   ```

### Step 4: Create a Compatibility Wrapper

Implement a wrapper function that maintains the original API:

```cpp
nsresult GetSomething(Type** aOutParam) {
  if (!aOutParam) {
    return NS_ERROR_NULL_POINTER;
  }
  *aOutParam = nullptr;
  
  auto result = GetSomethingModern();
  if (result.isErr()) {
    return result.unwrapErr();
  }
  
  nsCOMPtr<Type> value = result.unwrap();
  value.forget(aOutParam);
  return NS_OK;
}
```

### Step 5: Update Callers (Optional)

If appropriate, update callers to use the modernized function:

**Original:**
```cpp
Type* something;
nsresult rv = GetSomething(&something);
if (NS_FAILED(rv)) {
  return rv;
}
// Use something
```

**Modernized:**
```cpp
auto result = GetSomethingModern();
if (result.isErr()) {
  return result.unwrapErr();
}
nsCOMPtr<Type> something = result.unwrap();
// Use something
```

## Example: Modernizing GetRangeAt

### Original Implementation

```cpp
NS_IMETHODIMP
nsSelection::GetRangeAt(PRInt32 aIndex, nsIDOMRange** aReturn)
{
  if (!aReturn)
    return NS_ERROR_NULL_POINTER;
  *aReturn = nsnull;
  
  nsresult res = NS_OK;
  if (aIndex < 0 || aIndex >= (PRInt32)mRanges.Length())
    return NS_ERROR_INVALID_ARG;
  
  if (mRanges[aIndex]) {
    res = CallQueryInterface(mRanges[aIndex], aReturn);
  }
  
  return res;
}
```

### Modernized Implementation

```cpp
Result<nsCOMPtr<nsIDOMRange>, nsresult>
nsSelection::GetRangeAtModern(int32_t aIndex)
{
  if (aIndex < 0 || aIndex >= static_cast<int32_t>(mRanges.Length())) {
    return Err(NS_ERROR_INVALID_ARG);
  }
  
  if (!mRanges[aIndex]) {
    return Ok(nullptr);
  }
  
  nsCOMPtr<nsIDOMRange> range = do_QueryInterface(mRanges[aIndex]);
  return Ok(range);
}

// Compatibility wrapper
NS_IMETHODIMP
nsSelection::GetRangeAt(int32_t aIndex, nsIDOMRange** aReturn)
{
  if (!aReturn) {
    return NS_ERROR_NULL_POINTER;
  }
  *aReturn = nullptr;
  
  auto result = GetRangeAtModern(aIndex);
  if (result.isErr()) {
    return result.unwrapErr();
  }
  
  nsCOMPtr<nsIDOMRange> range = result.unwrap();
  range.forget(aReturn);
  return NS_OK;
}
```

## Best Practices

1. **Use Early Returns**: Handle errors as early as possible.
2. **Be Explicit**: Make error cases explicit and provide meaningful error codes.
3. **Use Smart Pointers**: Combine with smart pointers for memory safety.
4. **Maintain Compatibility**: Always provide compatibility wrappers.
5. **Document Assumptions**: Document any assumptions about input validation.

## Common Pitfalls

1. **Forgetting to Check Result**: Always check if a Result is Ok before unwrapping.
2. **Incorrect Error Propagation**: Ensure errors are properly propagated.
3. **Missing Compatibility**: Don't forget to provide compatibility wrappers.
4. **Inconsistent Naming**: Use consistent naming conventions for modernized functions.

## Performance Considerations

The Result<T> pattern introduces minimal overhead compared to traditional error codes:
- Memory usage is optimized using a union to store either the value or the error code.
- The pattern eliminates the need for out parameters, which can improve performance.
- Error handling is more predictable, which can lead to better compiler optimizations.

## Conclusion

The Result<T> pattern is a powerful tool for modernizing error handling in the Mozilla 1.0 codebase. By following this guide, you can improve the safety, readability, and maintainability of your code while maintaining compatibility with the existing codebase.

## References

1. Rust's Result type: https://doc.rust-lang.org/std/result/
2. C++ Expected proposal: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0323r7.html
3. Mozilla's nsresult documentation: https://developer.mozilla.org/en-US/docs/Mozilla/Errors 