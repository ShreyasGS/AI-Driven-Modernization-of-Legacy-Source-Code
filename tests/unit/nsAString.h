#ifndef nsAString_h
#define nsAString_h

// Simple implementation of nsAString for testing
class nsAString {
public:
    nsAString() {}
    virtual ~nsAString() {}
    
    // Basic operations
    virtual void Assign(const char* aData) = 0;
    virtual void Append(const char* aData) = 0;
    virtual const char* get() const = 0;
};

// Implementation of nsAString
class nsString : public nsAString {
public:
    nsString() { mData[0] = '\0'; }
    explicit nsString(const char* aData) { 
        int i = 0;
        while (aData[i] && i < 255) {
            mData[i] = aData[i];
            i++;
        }
        mData[i] = '\0';
    }
    virtual ~nsString() {}
    
    // Basic operations
    virtual void Assign(const char* aData) override {
        int i = 0;
        while (aData[i] && i < 255) {
            mData[i] = aData[i];
            i++;
        }
        mData[i] = '\0';
    }
    
    virtual void Append(const char* aData) override {
        int i = 0;
        while (mData[i]) i++;
        
        int j = 0;
        while (aData[j] && i < 255) {
            mData[i++] = aData[j++];
        }
        mData[i] = '\0';
    }
    
    virtual const char* get() const override {
        return mData;
    }
    
private:
    char mData[256]; // Fixed-size buffer for simplicity
};

#endif // nsAString_h 