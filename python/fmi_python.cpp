#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/stl_iterator.hpp>
#include <Communicator.h>
#include <comm/Data.h>
#include "PythonCommunicator.h"

using namespace boost::python;



BOOST_PYTHON_MODULE(fmi)
{
    class_<FMI::Utils::PythonCommunicator>("Communicator", init<FMI::Utils::peer_num, FMI::Utils::peer_num, std::string, std::string, optional<unsigned int> >())
        .def("send", &FMI::Utils::PythonCommunicator::send)
        .def("recv", &FMI::Utils::PythonCommunicator::recv)
        .def("bcast", &FMI::Utils::PythonCommunicator::bcast)
        .def("barrier", &FMI::Utils::PythonCommunicator::barrier)
        .def("gather", &FMI::Utils::PythonCommunicator::gather)
        .def("scatter", &FMI::Utils::PythonCommunicator::scatter)
        .def("reduce", &FMI::Utils::PythonCommunicator::reduce)
        .def("allreduce", &FMI::Utils::PythonCommunicator::allreduce)
        .def("scan", &FMI::Utils::PythonCommunicator::scan)
        .def("hint", &FMI::Utils::PythonCommunicator::hint)
    ;

    enum_<FMI::Utils::PythonType>("datatypes")
        .value("int", FMI::Utils::PythonType::INT)
        .value("double", FMI::Utils::PythonType::DOUBLE)
        .value("int_list", FMI::Utils::PythonType::INT_LIST)
        .value("double_list", FMI::Utils::PythonType::DOUBLE_LIST)
    ;

    class_<FMI::Utils::PythonData>("types", init<FMI::Utils::PythonType>())
        .def(init<FMI::Utils::PythonType, unsigned int>())
    ;

    enum_<FMI::Utils::PythonOp>("op")
        .value("sum", FMI::Utils::PythonOp::SUM)
        .value("prod", FMI::Utils::PythonOp::PROD)
        .value("max", FMI::Utils::PythonOp::MAX)
        .value("min", FMI::Utils::PythonOp::MIN)
        .value("custom", FMI::Utils::PythonOp::CUSTOM)
    ;

    class_<FMI::Utils::PythonFunc>("func", init<FMI::Utils::PythonOp>())
        .def(init<FMI::Utils::PythonOp, boost::python::object, bool, bool>())
    ;

    enum_<FMI::Utils::Hint>("hints")
        .value("cheap", FMI::Utils::Hint::cheap)
        .value("fast", FMI::Utils::Hint::fast)
    ;
}

