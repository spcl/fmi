#ifndef SMI_FUNCTION_H
#define SMI_FUNCTION_H

namespace SMI::Utils {
    template<typename T>
    class Function {
    public:
        Function(std::function<T(T,T)> f, bool commutative) : f(f), commutative(commutative) {}
    private:
        std::function<T(T,T)> f;
        bool commutative;
    };
}

#endif //SMI_FUNCTION_H
