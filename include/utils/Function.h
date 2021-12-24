#ifndef FMI_FUNCTION_H
#define FMI_FUNCTION_H

namespace FMI::Utils {
    //! Small wrapper around an arbitrary C++ binary function with signature T(T,T), i.e. accepting two arguments of type T and returning one of type T.
    template<typename T>
    class Function {
    public:
        Function(std::function<T(T,T)> f, bool commutative, bool associative) : f(f), commutative(commutative), associative(associative) {}

        T operator()(T a, T b) const {
            return f(a, b);
        }

        //! User provided information about commutativity
        bool commutative;
        //! User provided information about associativity
        bool associative;
    private:
        std::function<T(T,T)> f;
    };
}

#endif //FMI_FUNCTION_H
