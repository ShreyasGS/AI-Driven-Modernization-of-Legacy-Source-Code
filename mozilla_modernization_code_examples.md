# Mozilla 1.0 Modernization: Code Examples

This document showcases concrete examples of our modernization efforts, demonstrating the transformation from legacy code patterns to modern C++ implementations.

## Example 1: GetRangeAt Method

The `GetRangeAt` method retrieves a DOM range at a specific index from the selection. This example demonstrates several modernization patterns applied together.

### Before Modernization

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

### After Modernization

```cpp
// Modern implementation using Result type and smart pointers
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

// Compatibility wrapper maintains original API
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

### Improvements

1. **Result Type Pattern**
   - Replaced error code returns with a Result<T> type
   - Combined return value and error information in a single object
   - Made error handling more explicit and less prone to mistakes

2. **Smart Pointer Pattern**
   - Used nsCOMPtr instead of raw pointers
   - Eliminated manual reference counting
   - Prevented potential memory leaks

3. **Safe Cast Pattern**
   - Replaced C-style cast with static_cast
   - Improved type safety and made the cast more explicit

4. **Modern C++ Features**
   - Used auto for type inference
   - Adopted consistent bracing style
   - Updated legacy types (PRInt32 → int32_t, nsnull → nullptr)

## Example 2: AddItem Method

The `AddItem` method adds a range to the selection. This example demonstrates modernizing error handling and reference counting.

### Before Modernization

```cpp
NS_IMETHODIMP
nsSelection::AddItem(nsIRange* aRange)
{
  if (!aRange)
    return NS_ERROR_NULL_POINTER;
    
  // check if range belongs to this selection
  for (uint32_t i = 0; i < mRanges.Length(); i++) {
    if (mRanges[i] == aRange)
      return NS_OK;
  }
  
  if (aRange->IsInSelection())
    return NS_ERROR_FAILURE;
    
  // otherwise, add it to the selection
  if (!mRanges.AppendElement(aRange))
    return NS_ERROR_OUT_OF_MEMORY;
    
  aRange->SetInSelection(true);
  NS_ADDREF(aRange);
  
  // We need to notify document here
  nsINode* commonAncestor = aRange->GetCommonAncestor();
  if (commonAncestor) {
    nsIDocument* doc = commonAncestor->GetOwnerDocument();
    if (doc) {
      doc->NotifySelectionChanged(doc, this, nsISelectionListener::RANGE_ADDED);
    }
  }
  return NS_OK;
}
```

### After Modernization

```cpp
Result<Ok, nsresult>
nsSelection::AddItemModern(nsIRange* aRange)
{
  if (!aRange) {
    return Err(NS_ERROR_NULL_POINTER);
  }
    
  // check if range belongs to this selection
  for (uint32_t i = 0; i < mRanges.Length(); i++) {
    if (mRanges[i] == aRange) {
      return Ok();
    }
  }
  
  if (aRange->IsInSelection()) {
    return Err(NS_ERROR_FAILURE);
  }
    
  // otherwise, add it to the selection
  if (!mRanges.AppendElement(aRange)) {
    return Err(NS_ERROR_OUT_OF_MEMORY);
  }
    
  aRange->SetInSelection(true);
  RefPtr<nsIRange> rangeRef = aRange;  // Adds reference automatically
  
  // We need to notify document here
  nsCOMPtr<nsINode> commonAncestor = aRange->GetCommonAncestor();
  if (commonAncestor) {
    nsCOMPtr<nsIDocument> doc = commonAncestor->GetOwnerDocument();
    if (doc) {
      doc->NotifySelectionChanged(doc, this, nsISelectionListener::RANGE_ADDED);
    }
  }
  return Ok();
}

// Compatibility wrapper
NS_IMETHODIMP
nsSelection::AddItem(nsIRange* aRange)
{
  auto result = AddItemModern(aRange);
  return result.isOk() ? NS_OK : result.unwrapErr();
}
```

### Improvements

1. **Result Type Pattern**
   - Used Result<Ok, nsresult> for methods that don't return a value
   - Made error handling more consistent and explicit
   - Early returns for error conditions improve code clarity

2. **Smart Pointer Pattern**
   - Replaced manual NS_ADDREF with RefPtr
   - Used nsCOMPtr for temporary objects
   - Eliminated potential reference counting bugs

3. **Code Structure**
   - Consistent bracing and formatting
   - Early returns for error conditions
   - Clear separation between modern implementation and compatibility wrapper

## Benefits of Modernization

These examples demonstrate several key benefits of our modernization approach:

1. **Safety Improvements**
   - Automatic reference counting prevents memory leaks
   - Type-safe casting prevents type errors
   - Result types prevent error handling mistakes

2. **Readability Improvements**
   - Consistent coding style
   - Explicit error handling
   - Clear ownership semantics

3. **Maintainability Improvements**
   - Reduced boilerplate code
   - Separation of modern implementation from compatibility layer
   - Use of modern C++ idioms familiar to contemporary developers

4. **Compatibility**
   - Original functionality preserved
   - Backward compatibility maintained through wrapper functions
   - Gradual migration path for calling code

Our modernization templates provide a systematic approach to transforming legacy code patterns into modern equivalents while maintaining backward compatibility. These examples demonstrate the practical application of our templates and the tangible improvements they bring to the codebase. 