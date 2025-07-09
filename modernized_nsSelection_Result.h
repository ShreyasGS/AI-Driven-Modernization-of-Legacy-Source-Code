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

} // namespace mozilla

#endif // mozilla_Result_h 