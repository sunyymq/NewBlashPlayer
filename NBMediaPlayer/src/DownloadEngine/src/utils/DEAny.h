#ifndef DE_ANY_H
#define DE_ANY_H

// can be replaced by other error mechanism
#include <cassert>
#define SHARED_ASSERT(x)    assert(x)

#include <typeinfo>
#include <algorithm>

class DEAny {
public: // structors
    DEAny()
    : content(NULL) {
    }

    template<typename ValueType>
    DEAny(const ValueType & value)
    : content(new holder<ValueType>(value)) {
    }

    DEAny(const DEAny & other)
    : content(other.content ? other.content->clone() : 0) {
    }

    ~DEAny() {
        delete content;
    }

public: // modifiers

    DEAny & swap(DEAny & rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    template<typename ValueType>
    DEAny & operator=(const ValueType & rhs) {
        Any(rhs).swap(*this);
        return *this;
    }

    DEAny & operator=(DEAny rhs) {
        rhs.swap(*this);
        return *this;
    }

public: // queries
    bool empty() const {
        return !content;
    }

    const std::type_info & type() const {
        return content ? content->type() : typeid(void);
    }

public: // types (public so any_cast can be non-friend)

    class placeholder {
    public: // structors
        virtual ~placeholder() {
        }

    public: // queries
        virtual const std::type_info & type() const = 0;

        virtual placeholder * clone() const = 0;
    };

    template<typename ValueType>
    class holder : public placeholder {
    public: // structors

        holder(const ValueType & value)
        : held(value)
        {
        }

    public: // queries
        virtual const std::type_info & type() const {
            return typeid(ValueType);
        }

        virtual placeholder * clone() const {
            return new holder(held);
        }

    public: // representation
        ValueType held;

    private: // intentionally left unimplemented
        holder & operator=(const holder &);
    };

public:
    placeholder * content;
};

template<typename ValueType>
ValueType AnyCast(const DEAny& operand) {
    SHARED_ASSERT( operand.type() == typeid(ValueType) );
    return static_cast<DEAny::holder<ValueType> *>(operand.content)->held;
}

#endif
