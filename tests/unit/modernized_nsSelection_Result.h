#ifndef mozilla_Result_h
#define mozilla_Result_h

#include "nsIDOMNode.h"

namespace mozilla {

/**
 * Result<T> is a type that represents either a value T or an nsresult error code.
 * It's similar to Rust's Result or Haskell's Either types.
 *
 * This implementation is simplified for testing purposes.
 */
template<typename T>
class Result {
public:
    // Construct a successful Result with a value
    explicit Result(const T& aValue) : mValue(aValue), mIsOk(true) {}
    
    // Construct a failed Result with an error code
    explicit Result(nsresult aErrorCode) : mErrorCode(aErrorCode), mIsOk(false) {}

    // Check if the Result represents success
    bool isOk() const { return mIsOk; }

    // Check if the Result represents failure
    bool isErr() const { return !mIsOk; }

    // Get the value (only call if isOk() is true)
    const T& unwrap() const {
        return mValue;
    }

    T& unwrap() {
        return mValue;
    }

    // Get the error code (only call if isOk() is false)
    nsresult unwrapErr() const {
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
 * This implementation is simplified for testing purposes.
 */
template<typename T>
class Maybe {
public:
    // Construct an empty Maybe
    Maybe() : mHasValue(false) {}

    // Construct a Maybe with a value
    explicit Maybe(const T& aValue) : mValue(aValue), mHasValue(true) {}

    // Check if the Maybe has a value
    bool isSome() const { return mHasValue; }

    // Check if the Maybe is empty
    bool isNothing() const { return !mHasValue; }

    // Get the value (only call if isSome() is true)
    const T& value() const {
        return mValue;
    }

    T& value() {
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
Maybe<T> Some(const T& aValue) {
    return Maybe<T>(aValue);
}

template<typename T>
Maybe<T> Nothing() {
    return Maybe<T>();
}

// Helper functions to create Result values
template<typename T>
Result<T> Ok(const T& aValue) {
    return Result<T>(aValue);
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