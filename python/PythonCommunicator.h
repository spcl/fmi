#ifndef SMI_PYTHONCOMMUNICATOR_H
#define SMI_PYTHONCOMMUNICATOR_H

#include <Communicator.h>
#include <memory>
#include <boost/python/object.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/iterator.hpp>

namespace SMI::Utils {

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
        PythonFunc(PythonOp op, const boost::python::object& func) : op(op), func(func) {}

        PythonOp op;
        boost::python::object func;
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
        PythonCommunicator(SMI::Utils::peer_num peer_id, SMI::Utils::peer_num num_peers, std::string config_path, std::string comm_name,
                     unsigned int faas_memory = 128);

        void send(const boost::python::object& py_obj, SMI::Utils::peer_num dst, SMI::Utils::PythonData type);

        boost::python::object recv(SMI::Utils::peer_num src, SMI::Utils::PythonData type);

        boost::python::object bcast(const boost::python::object& src_data, SMI::Utils::peer_num root, SMI::Utils::PythonData type);

        void barrier();

        boost::python::object gather(const boost::python::object& src_data, SMI::Utils::peer_num root, SMI::Utils::PythonData snd_type);

        boost::python::object scatter(const boost::python::object& src_data, SMI::Utils::peer_num root, SMI::Utils::PythonData snd_type);

        boost::python::object reduce(const boost::python::object& src_data, SMI::Utils::peer_num root, SMI::Utils::PythonFunc f,
                                     SMI::Utils::PythonData type);

    private:
        std::shared_ptr<SMI::Communicator> comm;
        SMI::Utils::peer_num peer_id;
        SMI::Utils::peer_num num_peers;

        template<typename T> T extract_object(const boost::python::object &py_obj) {
            T val;
            boost::python::extract<T> o(py_obj);
            if (o.check()) {
                val = o();
            } else {
                throw "Conversion of value not possible";
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
                throw "Could not convert to list";
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

    };
}



#endif //SMI_PYTHONCOMMUNICATOR_H
