#ifndef FMI_DATA_H
#define FMI_DATA_H

#include <any>
#include <vector>
#include <ostream>
#include <functional>

namespace FMI::Comm {
    //! Small data wrapper around a generic type T with some helper utilities
    template<typename T>
    class Data {
    public:
        Data() = default;
        Data(T value) : val(value) {}

        std::size_t size_in_bytes() {
            if (std::is_fundamental<T>::value) {
                return sizeof(T);
            } else {
                throw std::runtime_error("Cannot get size in bytes of non-fundamental type");
            }
        }

        //! Gets pointer to the start of the buffer.
        char* data() {
            return reinterpret_cast<char*>(&val);
        }

        //! Returns the value
        T get() const {
            return val;
        }

        friend std::ostream& operator<<( std::ostream& o, const Data& t ) {
            o << t.get();
            return o;
        }

        friend bool operator==(const Data& lhs, const Data& rhs) {
            return lhs.get() == rhs.get();
        }

    private:
        T val;

    };

    //! Wrapper around an arbitrary STL vector
    template<typename A>
    class Data<std::vector<A>> {
    public:
        Data() = default;
        Data(std::size_t n) : val(n) {}
        Data(std::vector<A> value) : val(value) {}

        std::size_t size_in_bytes() {
            return sizeof(A) * val.size();
        }

        //! Pointer to the start of the vector
        char* data() {
            return reinterpret_cast<char*>(val.data());
        }

        //! Returns the vector
        std::vector<A> get() const {
            return val;
        }

    private:
        std::vector<A> val;
    };

    //! Instantiate data with a pointer to memory and an arbitrary size. Should only be used in exceptional cases, the native types should be used otherwise.
    template<>
    class Data<void*> {
    public:
        Data() = default;
        Data(void* buf, std::size_t len) : buf(buf), len(len) {}

        std::size_t size_in_bytes() {
            return len;
        }

        char* data() {
            return reinterpret_cast<char*>(buf);
        }

        void* get() {
            return buf;
        }

    private:
        void* buf;
        std::size_t len;
    };


}

#endif //FMI_DATA_H
