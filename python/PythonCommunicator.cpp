#include "PythonCommunicator.h"
#include <utils/Common.h>
#include <string>
#include <boost/python/extract.hpp>

SMI::Utils::PythonCommunicator::PythonCommunicator(SMI::Utils::peer_num peer_id, SMI::Utils::peer_num num_peers, std::string config_path,
                                                   std::string comm_name, unsigned int faas_memory) {
    comm = std::make_shared<SMI::Communicator>(peer_id, num_peers, config_path, comm_name, faas_memory);
    this->peer_id = peer_id;
}

void SMI::Utils::PythonCommunicator::send(const boost::python::object& py_obj, SMI::Utils::peer_num dst, SMI::Utils::PythonData type) {
    if (type.type == INT) {
        auto val = extract_object<int>(py_obj);
        SMI::Comm::Data<int> data = val;
        comm->send<int>(data, dst);
    } else if (type.type == DOUBLE) {
        auto val = extract_object<double>(py_obj);
        SMI::Comm::Data<double> data = val;
        comm->send<double>(data, dst);
    } else if (type.type == INT_LIST) {
        auto val = extract_list<int>(py_obj);
        SMI::Comm::Data<std::vector<int>> data = val;
        comm->send<std::vector<int>>(data, dst);
    } else if (type.type == DOUBLE_LIST) {
        auto val = extract_list<double>(py_obj);
        SMI::Comm::Data<std::vector<double>> data = val;
        comm->send<std::vector<double>>(data, dst);
    } else {
        throw "Unknown type passed";
    }
}

boost::python::object SMI::Utils::PythonCommunicator::recv(SMI::Utils::peer_num src, SMI::Utils::PythonData type) {
    if (type.type == INT) {
        SMI::Comm::Data<int> data;
        comm->recv<int>(data, src);
        boost::python::object res(data.get());
        return res;
    } else if (type.type == DOUBLE) {
        SMI::Comm::Data<double> data;
        comm->recv<double>(data, src);
        boost::python::object res(data.get());
        return res;
    } else if (type.type == INT_LIST) {
        SMI::Comm::Data<std::vector<int>> data(type.num_objects);
        comm->recv<std::vector<int>>(data, src);
        boost::python::object res(to_list<int>(data.get()));
        return res;
    } else if (type.type == DOUBLE_LIST) {
        SMI::Comm::Data<std::vector<double>> data(type.num_objects);
        comm->recv<std::vector<double>>(data, src);
        boost::python::object res(to_list<double>(data.get()));
        return res;
    } else {
        throw "Unknown type passed";
    }
}

boost::python::object SMI::Utils::PythonCommunicator::bcast(const boost::python::object& py_obj, SMI::Utils::peer_num root, SMI::Utils::PythonData type) {
    if (type.type == INT) {
        SMI::Comm::Data<int> data;
        if (peer_id == root) {
            auto val = extract_object<int>(py_obj);
            data = val;
        }
        comm->bcast<int>(data, root);
        return boost::python::object(data.get());
    } else if (type.type == DOUBLE) {
        SMI::Comm::Data<double> data;
        if (peer_id == root) {
            auto val = extract_object<double>(py_obj);
            data = val;
        }
        comm->bcast<double>(data, root);
        return boost::python::object(data.get());
    } else if (type.type == INT_LIST) {
        SMI::Comm::Data<std::vector<int>> data(type.num_objects);
        if (peer_id == root) {
            auto val = extract_list<int>(py_obj);
            data = val;
        }
        comm->bcast<std::vector<int>>(data, root);
        return boost::python::object(to_list<int>(data.get()));
    } else if (type.type == DOUBLE_LIST) {
        SMI::Comm::Data<std::vector<double>> data(type.num_objects);
        if (peer_id == root) {
            auto val = extract_list<double>(py_obj);
            data = val;
        }
        comm->bcast<std::vector<double>>(data, root);
        return boost::python::object(to_list<double>(data.get()));
    } else {
        throw "Unknown type passed";
    }
}

void SMI::Utils::PythonCommunicator::barrier() {
    comm->barrier();
}
