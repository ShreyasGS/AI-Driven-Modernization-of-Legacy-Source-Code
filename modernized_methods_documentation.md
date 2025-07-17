# Mozilla 1.0 Modernized Methods Documentation

This document provides detailed information about the methods in `nsSelection.cpp` that have been modernized as part of our Mozilla 1.0 codebase modernization project. Each entry includes the original implementation, the modernized implementation, and an explanation of the modernization patterns applied.

## Table of Contents

1. [GetRangeAt](#getrangeat)
2. [GetPresContext](#getprescontext)
3. [GetAnchorNode](#getanchornode)
4. [AddItem](#additem)
5. [RemoveItem](#removeitem)
6. [Clear](#clear)
7. [CurrentItem](#currentitem)
8. [FetchDesiredX](#fetchdesiredx)
9. [FetchFocusNode](#fetchfocusnode)
10. [FetchFocusOffset](#fetchfocusoffset)
11. [FetchStartParent](#fetchstartparent)
12. [FetchStartOffset](#fetchstartoffset)

## GetRangeAt

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

### Modernization Patterns Applied

1. **Error Code Returns → Result Types**: Replaced error code returns with a `Result<T>` type that can contain either a success value or an error code.
2. **Raw Pointers → Smart Pointers**: Used `nsCOMPtr` for automatic reference counting.
3. **C-style Casts → Safe Casts**: Replaced C-style cast with `static_cast`.
4. **Explicit Error Handling**: Used early returns and explicit error handling.
5. **Compatibility Wrapper**: Created a compatibility wrapper that maintains the original API.

## GetPresContext

### Original Implementation

```cpp
nsresult
nsTypedSelection::GetPresContext(nsIPresContext **aPresContext)
{
  if (!aPresContext)
    return NS_ERROR_NULL_POINTER;
  
  *aPresContext = nsnull;
  
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;
  
  nsIPresShell *presShell = mFrameSelection->GetShell();
  
  if (!presShell)
    return NS_ERROR_FAILURE;
  
  NS_ADDREF(*aPresContext = presShell->GetPresContext());
  
  return NS_OK;
}
```

### Modernized Implementation

```cpp
Result<nsCOMPtr<nsIPresContext>, nsresult>
nsTypedSelection::GetPresContextModern()
{
  if (!mFrameSelection) {
    return Err(NS_ERROR_FAILURE);
  }
  
  nsIPresShell* presShell = mFrameSelection->GetShell();
  
  if (!presShell) {
    return Err(NS_ERROR_FAILURE);
  }
  
  nsCOMPtr<nsIPresContext> presContext = presShell->GetPresContext();
  if (!presContext) {
    return Err(NS_ERROR_FAILURE);
  }
  
  return Ok(presContext);
}

// Compatibility wrapper
nsresult
nsTypedSelection::GetPresContext(nsIPresContext** aPresContext)
{
  if (!aPresContext) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aPresContext = nullptr;
  
  auto result = GetPresContextModern();
  if (result.isErr()) {
    return result.unwrapErr();
  }
  
  nsCOMPtr<nsIPresContext> presContext = result.unwrap();
  presContext.forget(aPresContext);
  
  return NS_OK;
}
```

### Modernization Patterns Applied

1. **Error Code Returns → Result Types**: Replaced error code returns with a `Result<T>` type.
2. **Out Parameters → Return Values**: Returned the value directly instead of using an out parameter.
3. **Raw Pointers → Smart Pointers**: Used `nsCOMPtr` for automatic reference counting.
4. **Explicit Error Handling**: Used early returns and explicit error handling.
5. **Compatibility Wrapper**: Created a compatibility wrapper that maintains the original API.

## FetchFocusNode

### Original Implementation

```cpp
nsIDOMNode*
nsTypedSelection::FetchFocusNode()
{   //where is the carret
  nsCOMPtr<nsIDOMNode>returnval;
  GetFocusNode(getter_AddRefs(returnval));//this queries
  return returnval;
}//at end it will release, no addreff was called
```

### Modernized Implementation

```cpp
Result<nsCOMPtr<nsIDOMNode>, nsresult>
nsTypedSelection::FetchFocusNodeModern()
{   
  // Get the focus node
  nsCOMPtr<nsIDOMNode> returnval;
  nsresult rv = GetFocusNode(getter_AddRefs(returnval));
  
  // Handle error case
  if (NS_FAILED(rv)) {
    return Err(rv);
  }
  
  // Return success with the node
  return Ok(returnval);
}

// Compatibility wrapper
nsIDOMNode*
nsTypedSelection::FetchFocusNode()
{
  return FetchFocusNodeModern().unwrapOr(nullptr);
}
```

### Modernization Patterns Applied

1. **Error Code Returns → Result Types**: Replaced raw pointer return with a `Result<T>` type.
2. **Raw Pointers → Smart Pointers**: Used `nsCOMPtr` for automatic reference counting.
3. **Explicit Error Handling**: Added explicit error handling with early returns.
4. **Compatibility Wrapper**: Created a compatibility wrapper that maintains the original API.

## FetchFocusOffset

### Original Implementation

```cpp
PRInt32
nsTypedSelection::FetchFocusOffset()
{
  PRInt32 returnval;
  if (NS_SUCCEEDED(GetFocusOffset(&returnval)))//this queries
    return returnval;
  return NS_OK;
}
```

### Modernized Implementation

```cpp
Result<PRInt32, nsresult>
nsTypedSelection::FetchFocusOffsetModern()
{
  // Get the focus offset
  PRInt32 returnval;
  nsresult rv = GetFocusOffset(&returnval);
  
  // Handle error case
  if (NS_FAILED(rv)) {
    return Err(rv);
  }
  
  // Return success with the offset value
  return Ok(returnval);
}

// Compatibility wrapper
PRInt32
nsTypedSelection::FetchFocusOffset()
{
  return FetchFocusOffsetModern().unwrapOr(0);
}
```

### Modernization Patterns Applied

1. **Error Code Returns → Result Types**: Replaced raw integer return with a `Result<T>` type.
2. **Explicit Error Handling**: Added explicit error handling with early returns.
3. **Compatibility Wrapper**: Created a compatibility wrapper that maintains the original API.

## FetchStartParent

### Original Implementation

```cpp
nsIDOMNode*
nsTypedSelection::FetchStartParent(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  nsCOMPtr<nsIDOMNode> returnval;
  aRange->GetStartContainer(getter_AddRefs(returnval));
  return returnval;
}
```

### Modernized Implementation

```cpp
Result<nsCOMPtr<nsIDOMNode>, nsresult>
nsTypedSelection::FetchStartParentModern(nsIDOMRange *aRange)
{
  // Validate input
  if (!aRange) {
    return Err(NS_ERROR_INVALID_ARG);
  }
  
  // Get the start container
  nsCOMPtr<nsIDOMNode> returnval;
  nsresult rv = aRange->GetStartContainer(getter_AddRefs(returnval));
  
  // Handle error case
  if (NS_FAILED(rv)) {
    return Err(rv);
  }
  
  // Return success with the node
  return Ok(returnval);
}

// Compatibility wrapper
nsIDOMNode*
nsTypedSelection::FetchStartParent(nsIDOMRange *aRange)
{
  return FetchStartParentModern(aRange).unwrapOr(nullptr);
}
```

### Modernization Patterns Applied

1. **Error Code Returns → Result Types**: Replaced raw pointer return with a `Result<T>` type.
2. **Raw Pointers → Smart Pointers**: Used `nsCOMPtr` for automatic reference counting.
3. **Explicit Error Handling**: Added explicit error handling with early returns.
4. **Proper Null Checking**: Added proper null checking with meaningful error codes.
5. **Compatibility Wrapper**: Created a compatibility wrapper that maintains the original API.

## FetchStartOffset

### Original Implementation

```cpp
PRInt32
nsTypedSelection::FetchStartOffset(nsIDOMRange *aRange)
{
  if (!aRange)
    return nsnull;
  PRInt32 returnval;
  if (NS_SUCCEEDED(aRange->GetStartOffset(&returnval)))
    return returnval;
  return 0;
}
```

### Modernized Implementation

```cpp
Result<PRInt32, nsresult>
nsTypedSelection::FetchStartOffsetModern(nsIDOMRange *aRange)
{
  // Validate input
  if (!aRange) {
    return Err(NS_ERROR_INVALID_ARG);
  }
  
  // Get the start offset
  PRInt32 returnval;
  nsresult rv = aRange->GetStartOffset(&returnval);
  
  // Handle error case
  if (NS_FAILED(rv)) {
    return Err(rv);
  }
  
  // Return success with the offset value
  return Ok(returnval);
}

// Compatibility wrapper
PRInt32
nsTypedSelection::FetchStartOffset(nsIDOMRange *aRange)
{
  return FetchStartOffsetModern(aRange).unwrapOr(0);
}
```

### Modernization Patterns Applied

1. **Error Code Returns → Result Types**: Replaced raw integer return with a `Result<T>` type.
2. **Explicit Error Handling**: Added explicit error handling with early returns.
3. **Proper Null Checking**: Added proper null checking with meaningful error codes.
4. **Compatibility Wrapper**: Created a compatibility wrapper that maintains the original API.

## Modernization Impact

The modernization of these methods has resulted in several improvements:

1. **Better Error Handling**: Explicit error handling with Result<T> types makes error paths clear and prevents error code loss.
2. **Memory Safety**: Smart pointers ensure proper reference counting and prevent memory leaks.
3. **Type Safety**: Safe casts and explicit types improve type safety and prevent type-related bugs.
4. **Code Clarity**: Modern patterns make the code's intent clearer and easier to understand.
5. **Maintainability**: Modernized code follows consistent patterns that are easier to maintain and extend.

## Next Steps

We will continue modernizing the remaining methods in nsSelection.cpp, focusing on:

1. **FetchEndParent** and **FetchEndOffset**: Similar to FetchStartParent and FetchStartOffset
2. **GetAnchorOffset**: Similar to GetFocusOffset
3. **Collapse**, **Extend**, and other selection manipulation methods
4. **ContainsNode** and other query methods
5. **DeleteFromDocument** and other document interaction methods 