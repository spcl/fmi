#ifndef SMI_DATA_H
#define SMI_DATA_H

#include <any>
#include <vector>

namespace SMI::comm {
    template<typename T>
    class Data {
    public:
        Data(T value) : val(value) {}

        std::size_t size_in_bytes() {
            if (std::is_fundamental<T>::value) {
                return sizeof(T);
            } else {
                throw "Cannot get size in bytes of non-fundamental type";
            }
        }

    private:
        T val;

    };

    template<typename A>
    class Data<std::vector<A>> {
    public:
        Data(std::vector<A> value) : val(value) {}

        std::size_t size_in_bytes() {
            return sizeof(A) * val.size();
        }

    private:
        std::vector<A> val;
    };

    template<>
    class Data<void*> {
    public:
        Data(void* buf, std::size_t len) : buf(buf), len(len) {}

        std::size_t size_in_bytes() {
            return len;
        }
    private:
        void* buf;
        std::size_t len;
    };


}

#endif //SMI_DATA_H
