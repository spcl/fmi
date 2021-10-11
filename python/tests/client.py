import smi
import sys


num_nodes = int(sys.argv[1])
node_id = int(sys.argv[2])
comm = smi.Communicator(node_id, num_nodes, "../../config/smi.json", "Test", 512)

comm.barrier()

if node_id == 0:
    """comm.send(42, 1, smi.types(smi.datatypes.int))
    comm.send(14.2, 1, smi.types(smi.datatypes.double))
    comm.send([1,2], 1, smi.types(smi.datatypes.int_list, 2))
    comm.send([1.32,2.34], 1, smi.types(smi.datatypes.double_list, 2))"""
    """comm.bcast(42, 0, smi.types(smi.datatypes.int))
    comm.bcast(14.2, 0, smi.types(smi.datatypes.double))
    comm.bcast([1,2], 0, smi.types(smi.datatypes.int_list, 2))
    comm.bcast([1.32,2.34], 0, smi.types(smi.datatypes.double_list, 2))"""

elif node_id == 1:
    """print(comm.recv(0, smi.types(smi.datatypes.int)))
    print(comm.recv(0, smi.types(smi.datatypes.double)))
    print(comm.recv(0, smi.types(smi.datatypes.int_list, 2)))
    print(comm.recv(0, smi.types(smi.datatypes.double_list, 2)))"""
    """print(comm.bcast(None, 0, smi.types(smi.datatypes.int)))
    print(comm.bcast(None, 0, smi.types(smi.datatypes.double)))
    print(comm.bcast(None, 0, smi.types(smi.datatypes.int_list, 2)))
    print(comm.bcast(None, 0, smi.types(smi.datatypes.double_list, 2)))"""
