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
        .def("bcast", &SMI::Utils::PythonCommunicator::bcast)
        .def("barrier", &SMI::Utils::PythonCommunicator::barrier)
        .def("gather", &SMI::Utils::PythonCommunicator::gather)
        .def("scatter", &SMI::Utils::PythonCommunicator::scatter)
        .def("reduce", &SMI::Utils::PythonCommunicator::reduce)
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

    enum_<SMI::Utils::PythonOp>("op")
        .value("sum", SMI::Utils::PythonOp::SUM)
        .value("prod", SMI::Utils::PythonOp::PROD)
        .value("max", SMI::Utils::PythonOp::MAX)
        .value("min", SMI::Utils::PythonOp::MIN)
        .value("custom", SMI::Utils::PythonOp::CUSTOM)
    ;

    class_<SMI::Utils::PythonFunc>("func", init<SMI::Utils::PythonOp>())
        .def(init<SMI::Utils::PythonOp, boost::python::object>())
    ;
}

