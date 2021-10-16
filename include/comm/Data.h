#ifndef SMI_DATA_H
#define SMI_DATA_H

#include <any>
#include <vector>
#include <ostream>
#include <functional>

namespace SMI::Comm {
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

        char* data() {
            return reinterpret_cast<char*>(&val);
        }

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

    template<typename A>
    class Data<std::vector<A>> {
    public:
        Data() = default;
        Data(std::size_t n) : val(n) {}
        Data(std::vector<A> value) : val(value) {}

        std::size_t size_in_bytes() {
            return sizeof(A) * val.size();
        }

        char* data() {
            return reinterpret_cast<char*>(val.data());
        }

        std::vector<A> get() const {
            return val;
        }

    private:
        std::vector<A> val;
    };

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

#endif //SMI_DATA_H
