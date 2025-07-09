# Layout Component Modernization Example: LeafFrame

This document demonstrates how to modernize a layout component from the Mozilla 1.0 codebase using modern C++ techniques. We've focused on the `nsLeafFrame` component, which is a fundamental building block in the layout engine.

## Original Implementation

The original `nsLeafFrame` implementation has several characteristics typical of legacy C++ code:

1. Manual memory management
2. Out parameters for return values
3. Error codes for error handling
4. C-style casts
5. No clear ownership semantics

## Modernization Patterns Applied

### 1. Namespace Usage

We've placed the modernized implementation in the `mozilla` namespace to clearly separate it from legacy code:

```cpp
namespace mozilla {
  class LeafFrame : public nsFrame {
    // ...
  };
} // namespace mozilla
```

### 2. Factory Methods

We've replaced the global creation functions with static factory methods that return smart pointers:

**Original Pattern:**
```cpp
nsresult NS_NewLeafFrame(nsIFrame** aResult) {
  *aResult = new nsLeafFrame();
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aResult);
  return NS_OK;
}
```

**Modernized Pattern:**
```cpp
static already_AddRefed<LeafFrame> Create(nsIPresContext* aPresContext,
                                         nsIStyleContext* aStyleContext) {
  RefPtr<LeafFrame> frame = new LeafFrame(aStyleContext);
  return frame.forget();
}
```

### 3. Result Types

We've introduced result types that combine return values and error codes, eliminating out parameters:

**Original Pattern:**
```cpp
NS_IMETHODIMP nsLeafFrame::Reflow(nsIPresContext* aPresContext,
                                 nsHTMLReflowMetrics& aMetrics,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus& aStatus) {
  // Implementation that sets aStatus and returns nsresult
}
```

**Modernized Pattern:**
```cpp
struct ReflowResult {
  nsresult rv;
  nsReflowStatus status;
  
  explicit operator bool() const { return NS_SUCCEEDED(rv); }
};

ReflowResult ReflowModern(nsIPresContext* aPresContext,
                         nsHTMLReflowMetrics& aMetrics,
                         const nsHTMLReflowState& aReflowState) {
  // Implementation that returns a ReflowResult
}
```

### 4. RAII for Resource Management

We've implemented RAII patterns for resource acquisition and release:

**Original Pattern:**
```cpp
// Save and restore context
aRenderingContext.PushState();
// ... painting code ...
aRenderingContext.PopState();
```

**Modernized Pattern:**
```cpp
// Use RAII for context state management
class RenderingContextStateRAII {
public:
  explicit RenderingContextStateRAII(nsIRenderingContext& aContext) : mContext(aContext) {
    mContext.PushState();
  }
  ~RenderingContextStateRAII() {
    mContext.PopState();
  }
private:
  nsIRenderingContext& mContext;
};

// Usage
RenderingContextStateRAII state(aRenderingContext);
// ... painting code ...
// State automatically restored when state goes out of scope
```

### 5. Modern C++ Features

We've utilized modern C++ features throughout:

1. **Default constructors/destructors:**
```cpp
LeafFrame::~LeafFrame() = default;
```

2. **Deleted copy operations:**
```cpp
// Prevent copying
LeafFrame(const LeafFrame&) = delete;
LeafFrame& operator=(const LeafFrame&) = delete;
```

3. **Return values instead of out parameters:**
```cpp
// Original
void AddBordersAndPadding(nsIPresContext* aPresContext,
                         const nsHTMLReflowState& aReflowState,
                         nsHTMLReflowMetrics& aDesiredSize,
                         nsMargin& aBorderPadding);

// Modernized
nsMargin AddBordersAndPadding(nsIPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsHTMLReflowMetrics& aDesiredSize);
```

4. **Safe casts:**
```cpp
// Original
const nsStyleVisibility* vis = 
  (const nsStyleVisibility*)mStyleContext->GetStyleData(eStyleStruct_Visibility);

// Modernized
const nsStyleVisibility* vis = 
  static_cast<const nsStyleVisibility*>(mStyleContext->GetStyleData(eStyleStruct_Visibility));
```

### 6. Compatibility Layer

We've maintained compatibility with legacy code by:

1. Implementing the original interfaces:
```cpp
NS_IMETHODIMP LeafFrame::Paint(...) {
  // Use the modern implementation
  PaintResult result = PaintModern(...);
  return result.rv;
}
```

2. Providing legacy creation functions:
```cpp
nsresult NS_NewSpaceFrameModern(nsIPresContext* aPresContext,
                              nsIFrame** aNewFrame,
                              nscoord aWidth,
                              nscoord aHeight) {
  // Use the modern implementation
  RefPtr<mozilla::SpaceFrame> frame = 
    mozilla::SpaceFrame::Create(aPresContext, nullptr, aWidth, aHeight);
  frame.forget(aNewFrame);
  return NS_OK;
}
```

## Concrete Implementation Example

To demonstrate how the modernized `LeafFrame` can be used, we've created a simple `SpaceFrame` implementation:

```cpp
class SpaceFrame : public LeafFrame {
public:
  static already_AddRefed<SpaceFrame> Create(nsIPresContext* aPresContext,
                                           nsIStyleContext* aStyleContext,
                                           nscoord aWidth,
                                           nscoord aHeight);

protected:
  SpaceFrame(nsIStyleContext* aStyleContext, nscoord aWidth, nscoord aHeight);
  
  void GetDesiredSize(nsIPresContext* aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsHTMLReflowMetrics& aDesiredSize) override;

private:
  nscoord mWidth;
  nscoord mHeight;
};
```

This implementation demonstrates:
1. Proper inheritance from the modernized base class
2. Factory method pattern
3. Clean implementation of virtual methods
4. Proper encapsulation of data members

## Benefits of Modernization

1. **Improved Safety**: Smart pointers and RAII prevent resource leaks.
2. **Better Error Handling**: Result types make error conditions explicit.
3. **Cleaner Code**: Modern C++ features result in more readable and maintainable code.
4. **Compatibility**: The dual API approach allows gradual migration without breaking existing code.

## Implementation Strategy

To modernize layout components:

1. **Create Modern Base Classes**: Start with fundamental base classes like frames.
2. **Add Modern APIs**: Implement new APIs alongside existing ones for backward compatibility.
3. **Implement Concrete Components**: Create concrete implementations using the modernized base classes.
4. **Gradually Migrate**: Update code to use the modern APIs over time.
5. **Maintain Tests**: Ensure all functionality works correctly throughout the modernization process. 