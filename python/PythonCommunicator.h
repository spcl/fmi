#ifndef FMI_PYTHONCOMMUNICATOR_H
#define FMI_PYTHONCOMMUNICATOR_H

#include <Communicator.h>
#include <memory>
#include <boost/python/object.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/iterator.hpp>

namespace FMI::Utils {

    enum PythonOp {
        SUM, PROD, MAX, MIN, CUSTOM
    };

    enum PythonType {
        INT,
        DOUBLE,
        INT_LIST,
        DOUBLE_LIST
    };

    class PythonFunc {
    public:
        PythonFunc(PythonOp op) : op(op) {}
        PythonFunc(PythonOp op, const boost::python::object& func, bool comm, bool assoc) : op(op), func(func), comm(comm), assoc(assoc) {}

        PythonOp op;
        boost::python::object func;
        bool comm;
        bool assoc;
    };

    class PythonData {
    public:
        PythonData(PythonType type) : type(type) {}
        PythonData(PythonType type, unsigned int num_objects) : type(type), num_objects(num_objects) {}

        PythonType type;
        unsigned int num_objects;
    };

    class PythonCommunicator {
    public:
        PythonCommunicator(FMI::Utils::peer_num peer_id, FMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name,
                           unsigned int faas_memory = 128);

        void send(const boost::python::object& py_obj, FMI::Utils::peer_num dst, FMI::Utils::PythonData type);

        boost::python::object recv(FMI::Utils::peer_num src, FMI::Utils::PythonData type);

        boost::python::object bcast(const boost::python::object& src_data, FMI::Utils::peer_num root, FMI::Utils::PythonData type);

        void barrier();

        boost::python::object gather(const boost::python::object& src_data, FMI::Utils::peer_num root, FMI::Utils::PythonData snd_type);

        boost::python::object scatter(const boost::python::object& src_data, FMI::Utils::peer_num root, FMI::Utils::PythonData snd_type);

        boost::python::object reduce(const boost::python::object& src_data, FMI::Utils::peer_num root, FMI::Utils::PythonFunc f,
                                     FMI::Utils::PythonData type);

        boost::python::object allreduce(const boost::python::object& src_data, FMI::Utils::PythonFunc f, FMI::Utils::PythonData type);

        boost::python::object scan(const boost::python::object& src_data, FMI::Utils::PythonFunc f, FMI::Utils::PythonData type);

        void hint(FMI::Utils::Hint hint);

    private:
        std::shared_ptr<FMI::Communicator> comm;
        FMI::Utils::peer_num peer_id;
        FMI::Utils::peer_num num_peers;

        template<typename T> T extract_object(const boost::python::object &py_obj) {
            T val;
            boost::python::extract<T> o(py_obj);
            if (o.check()) {
                val = o();
            } else {
                throw std::runtime_error("Conversion of value not possible");
            }
            return val;
        }

        template<typename T> std::vector<T> extract_list(const boost::python::object &py_obj) {
            std::vector<T> res;
            boost::python::extract<boost::python::list> o(py_obj);
            if (o.check()) {
                auto list = o();
                return to_vec<T>(list);
            } else {
                throw std::runtime_error("Could not convert to list");
            }
        }

        template<typename T>
        std::vector<T> to_vec( const boost::python::list& iterable )
        {
            return std::vector<T>(std::vector< T >( boost::python::stl_input_iterator< T >( iterable ),
                                                    boost::python::stl_input_iterator< T >( ) ));
        }

        template<typename T>
        boost::python::list to_list(const std::vector<T>& v)
        {
            typename std::vector<T>::const_iterator iter;
            boost::python::list list;
            for (iter = v.begin(); iter != v.end(); ++iter) {
                list.append(*iter);
            }
            return list;
        }

        template<typename T>
        FMI::Utils::Function<T> get_function(FMI::Utils::PythonFunc f)
        {
            FMI::Utils::Function<T> func([] (T a, T b) {return a + b;}, true, true);
            if (f.op == PROD) {
                func = FMI::Utils::Function<T>([] (T a, T b) {return a * b;}, true, true);
            } else if (f.op == MAX) {
                func = FMI::Utils::Function<T>([] (T a, T b) {return std::max(a, b);}, true, true);
            } else if (f.op == MIN) {
                func = FMI::Utils::Function<T>([] (T a, T b) {return std::min(a, b);}, true, true);
            } else if (f.op == CUSTOM) {
                func = FMI::Utils::Function<T>([this, f] (T a, T b) {return extract_object<T>(f.func(a, b));}, f.comm, f.assoc);
            }
            return func;
        }

        template<typename A>
        FMI::Utils::Function<std::vector<A>> get_vec_function(FMI::Utils::PythonFunc f)
        {
            using T = std::vector<A>;
            FMI::Utils::Function<T> func([] (T a, T b) { std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<A>()); return a; }, true, true);
            if (f.op == PROD) {
                func = FMI::Utils::Function<T>([] (T a, T b) { std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::multiplies<A>()); return a; }, true, true);
            } else if (f.op == MAX) {
                func = FMI::Utils::Function<T>([] (T a, T b) { std::transform(a.begin(), a.end(), b.begin(), a.begin(),
                                                                              [] (A a, A b) {return std::max(a,b);}); return a; }, true, true);
            } else if (f.op == MIN) {
                func = FMI::Utils::Function<T>([] (T a, T b) { std::transform(a.begin(), a.end(), b.begin(), a.begin(),
                                                                              [] (A a, A b) {return std::min(a,b);}); return a; }, true, true);
            } else if (f.op == CUSTOM) {
                auto iter = [this, f] (A a, A b) {return extract_object<A>(f.func(a, b));};
                func = FMI::Utils::Function<T>([iter] (T a, T b) { std::transform(a.begin(), a.end(), b.begin(), a.begin(), iter); return a; }, true, true);
            }
            return func;
        }

    };
}



#endif //FMI_PYTHONCOMMUNICATOR_H
