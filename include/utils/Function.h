#ifndef SMI_FUNCTION_H
#define SMI_FUNCTION_H

namespace SMI::Utils {
    template<typename T>
    class Function {
    public:
        Function(std::function<T(T,T)> f, bool commutative, bool associative) : f(f), commutative(commutative), associative(associative) {}

        T operator()(T a, T b) const {
            return f(a, b);
        }

        bool commutative;
        bool associative;
    private:
        std::function<T(T,T)> f;
    };
}

#endif //SMI_FUNCTION_H
