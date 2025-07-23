#ifndef mozilla_Result_h
#define mozilla_Result_h

#include "nscore.h"
#include <type_traits>
#include <utility>

namespace mozilla {

/**
 * Result<T> is a type that represents either a value T or an nsresult error code.
 * It's similar to Rust's Result or Haskell's Either types.
 *
 * This implementation is simplified for demonstration purposes.
 */
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

  // Check if the Result represents failure
  bool isErr() const { return !mIsOk; }

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

// Specialization for void
template<>
class Result<void> {
public:
  // Construct a successful Result
  Result() : mErrorCode(NS_OK), mIsOk(true) {}

  // Construct a failed Result with an error code
  explicit Result(nsresult aErrorCode) : mErrorCode(aErrorCode), mIsOk(false) {}

  // Check if the Result represents success
  bool isOk() const { return mIsOk; }

  // Check if the Result represents failure
  bool isErr() const { return !mIsOk; }

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
  nsresult mErrorCode;
  bool mIsOk;
};

/**
 * Maybe<T> is a type that represents an optional value of type T.
 * It's similar to C++17's std::optional, Rust's Option, or Haskell's Maybe.
 *
 * This implementation is simplified for demonstration purposes.
 */
template<typename T>
class Maybe {
public:
  // Construct an empty Maybe
  Maybe() : mHasValue(false) {}

  // Construct a Maybe with a value
  explicit Maybe(const T& aValue) : mValue(aValue), mHasValue(true) {}
  explicit Maybe(T&& aValue) : mValue(std::move(aValue)), mHasValue(true) {}

  // Check if the Maybe has a value
  bool isSome() const { return mHasValue; }

  // Check if the Maybe is empty
  bool isNothing() const { return !mHasValue; }

  // Get the value (only call if isSome() is true)
  const T& value() const {
    NS_ASSERTION(mHasValue, "Trying to get the value of an empty Maybe");
    return mValue;
  }

  T& value() {
    NS_ASSERTION(mHasValue, "Trying to get the value of an empty Maybe");
    return mValue;
  }

  // Get the value or a default
  T valueOr(const T& aDefault) const {
    return mHasValue ? mValue : aDefault;
  }

private:
  union {
    T mValue;
    char mDummy;  // To make the union valid when mHasValue is false
  };
  bool mHasValue;
};

// Helper functions to create Maybe values
template<typename T>
Maybe<typename std::decay<T>::type> Some(T&& aValue) {
  return Maybe<typename std::decay<T>::type>(std::forward<T>(aValue));
}

template<typename T>
Maybe<T> Nothing() {
  return Maybe<T>();
}

// Helper functions to create Result values
template<typename T>
Result<T> Ok(T&& aValue) {
  return Result<T>(std::forward<T>(aValue));
}

template<typename T>
Result<T> Err(nsresult aErrorCode) {
  return Result<T>(aErrorCode);
}

// Specialization for void
inline Result<void> Ok() {
  return Result<void>();
}

inline Result<void> Err(nsresult aErrorCode) {
  return Result<void>(aErrorCode);
}

} // namespace mozilla

#endif // mozilla_Result_h 