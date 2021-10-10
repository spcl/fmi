#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/stl_iterator.hpp>
#include <Communicator.h>
#include <comm/Data.h>
#include "PythonCommunicator.h"

using namespace boost::python;



BOOST_PYTHON_MODULE(smi)
{
    class_<SMI::Utils::PythonCommunicator>("Communicator", init<SMI::Utils::peer_num, SMI::Utils::peer_num, std::string, std::string, optional<unsigned int> >())
        .def("send", &SMI::Utils::PythonCommunicator::send)
        .def("recv", &SMI::Utils::PythonCommunicator::recv)
    ;

    enum_<SMI::Utils::PythonType>("datatypes")
        .value("int", SMI::Utils::PythonType::INT)
        .value("double", SMI::Utils::PythonType::DOUBLE)
        .value("int_list", SMI::Utils::PythonType::INT_LIST)
        .value("double_list", SMI::Utils::PythonType::DOUBLE_LIST)
    ;

    class_<SMI::Utils::PythonData>("types", init<SMI::Utils::PythonType>())
        .def(init<SMI::Utils::PythonType, unsigned int>())
    ;
}

