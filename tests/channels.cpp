#include <boost/test/unit_test.hpp>

#include "../include/comm/Channel.h"
#include "../include/comm/S3.h"
#include <numeric>
#include <ctime>
#include <omp.h>
#include <sys/mman.h>
#include <sys/wait.h>

BOOST_AUTO_TEST_SUITE(Channels);

std::map<std::string, std::string> s3_test_params = {
        {"bucket_name", "romanboe-uploadtest"},
        {"s3_region", "eu-central-1"},
        {"timeout", "100"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> s3_test_model_params = {
        {"bandwidth", "50.0"},
        {"overhead", "40.4"},
        {"transfer_price", "0.0"},
        {"download_price", "0.00000043"},
        {"upload_price", "0.0000054"}
};

std::map<std::string, std::string> redis_test_params = {
        {"host", "127.0.0.1"},
        {"port", "6379"},
        {"timeout", "1"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> redis_test_model_params = {
        {"bandwidth_single", "100.0"},
        {"bandwidth_multiple", "400.0"},
        {"overhead", "5.2"},
        {"transfer_price", "0.0"},
        {"instance_price", "0.0038"},
        {"requests_per_hour", "1000"},
        {"include_infrastructure_costs", "true"}
};

std::map<std::string, std::string> direct_test_params = {
        {"host", "127.0.0.1"},
        {"port", "10000"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> direct_test_model_params = {
        {"bandwidth", "250.0"},
        {"overhead", "0.34"},
        {"transfer_price", "0.0"},
        {"vm_price", "0.0134"},
        {"requests_per_hour", "1000"},
        {"include_infrastructure_costs", "true"}
};

std::map< std::string, std::pair< std::map<std::string, std::string>, std::map<std::string, std::string> > > backends = {
        //{"S3", {s3_test_params, s3_test_model_params}},
       // {"Redis", {redis_test_params, redis_test_model_params}},
        {"Direct", {direct_test_params, direct_test_model_params}}
};

std::string comm_name = std::to_string(std::time(nullptr)) + "Tests";

BOOST_AUTO_TEST_CASE(sending_receiving) {
    for (auto const & backend_data : backends) {
        // Using C++ 17 [key, val] : map syntax here does not compile (on some clang versions) in combination with the omp section because of a clang bug:
        // https://stackoverflow.com/questions/65819317/openmp-clang-sometimes-fail-with-a-variable-declared-from-structured-binding
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;

        int val = 42;
        int recv;
        #pragma omp parallel num_threads(2)
        {
            int tid = omp_get_thread_num();
            auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
            ch->set_peer_id(tid);
            ch->set_num_peers(2);
            ch->set_comm_name(comm_name);
            if (tid == 0) {
                channel_data buf {reinterpret_cast<char*>(&val), sizeof(val)};
                ch->send(buf, 1);
            } else if (tid == 1) {
                channel_data recv_buf {reinterpret_cast<char*>(&recv), sizeof(recv)};
                ch->recv(recv_buf, 0);
            }
            ch->finalize();
        }
        BOOST_CHECK_EQUAL(val, recv);
    }
}

BOOST_AUTO_TEST_CASE(sending_receiving_mult_times) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;

        int val1 = 42;
        int val2 = 4242;
        int recv1, recv2;
        #pragma omp parallel num_threads(2)
        {
            int tid = omp_get_thread_num();
            auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
            ch->set_peer_id(tid);
            ch->set_num_peers(2);
            ch->set_comm_name(comm_name);
            if (tid == 0) {
                ch->send({reinterpret_cast<char*>(&val1), sizeof(val1)}, 1);
                ch->send({reinterpret_cast<char*>(&val2), sizeof(val2)}, 1);
            } else if (tid == 1) {
                ch->recv({reinterpret_cast<char*>(&recv1), sizeof(recv1)}, 0);
                ch->recv({reinterpret_cast<char*>(&recv2), sizeof(recv2)}, 0);
            }
            ch->finalize();
        }
        BOOST_CHECK_EQUAL(val1, recv1);
        BOOST_CHECK_EQUAL(val2, recv2);
    }
}

BOOST_AUTO_TEST_CASE(bcast) {
    for (auto const & backend_data : backends) {
        // Using many threads leads to race conditions (in the AWS SDK, raw sockets, hiredis, ...), therefore processes are used for these tests
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        FMI::Utils::peer_num root = 14;
        constexpr int num_peers = 32;
        int* vals = static_cast<int*>(mmap(nullptr, num_peers * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        vals[root] = 42;
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        ch->bcast({reinterpret_cast<char*>(&vals[peer_id]), sizeof(vals[peer_id])}, root);
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(vals[i], 42);
            }
        } else {
            exit(0);
        }

    }
}

BOOST_AUTO_TEST_CASE(barrier_unsucc) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 4;
        bool* caught = static_cast<bool*>(mmap(nullptr, num_peers * sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        std::chrono::steady_clock::time_point bef = std::chrono::steady_clock::now();
        if (peer_id != 1) {
            try {
                ch->barrier();
            } catch (FMI::Utils::Timeout) {
                caught[peer_id] = true;
            }

        }
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            for (int i = 0; i < num_peers; i++) {
                if (i != 1) {
                    BOOST_CHECK_EQUAL(caught[i], true);
                }
            }
        } else {
            exit(0);
        }
    }
}

BOOST_AUTO_TEST_CASE(barrier_succ) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 2;
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        std::chrono::steady_clock::time_point bef = std::chrono::steady_clock::now();
        ch->barrier();
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            std::chrono::steady_clock::time_point after = std::chrono::steady_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - bef).count();
            BOOST_TEST(elapsed_ms < std::stoi(test_params["max_timeout"]));
        } else {
            exit(0);
        }
    }

}

BOOST_AUTO_TEST_CASE(gather_one) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 2;
        std::vector<int> vals {1,2,3,4};
        FMI::Utils::peer_num root = 1;

        int* rcv_vals = static_cast<int*>(mmap(nullptr, num_peers * 2 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        if (peer_id == root) {
            ch->gather({reinterpret_cast<char*>(vals.data() + 2 * peer_id), sizeof(vals[0]) * 2},
                        {reinterpret_cast<char*>(rcv_vals), sizeof(int) * num_peers * 2}, root);
        } else {
            ch->gather({reinterpret_cast<char*>(vals.data() + 2 * peer_id), sizeof(vals[0]) * 2}, {}, root);
        }
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(rcv_vals[i], i + 1);
            }
        } else {
            exit(0);
        }
    }
}

BOOST_AUTO_TEST_CASE(gather_multiple) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 14;
        std::vector<int> vals(2 * num_peers);
        for (int i = 0; i < vals.size(); i++) {
            vals[i] = i + 1;
        }
        FMI::Utils::peer_num root = 0;

        int* rcv_vals = static_cast<int*>(mmap(nullptr, num_peers * 2 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        if (peer_id == root) {
            ch->gather({reinterpret_cast<char*>(vals.data() + 2 * peer_id), sizeof(vals[0]) * 2},
                       {reinterpret_cast<char*>(rcv_vals), sizeof(int) * num_peers * 2}, root);
        } else {
            ch->gather({reinterpret_cast<char*>(vals.data() + 2 * peer_id), sizeof(vals[0]) * 2}, {}, root);
        }
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(rcv_vals[i], i + 1);
            }
        } else {
            exit(0);
        }
    }
}

BOOST_AUTO_TEST_CASE(scatter_one) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 2;
        std::vector<int> root_vals {1,2,3,4};
        FMI::Utils::peer_num root = 0;

        int* rcv_vals = static_cast<int*>(mmap(nullptr, num_peers * 2 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        if (peer_id == root) {
            ch->scatter({reinterpret_cast<char*>(root_vals.data()), sizeof(root_vals[0]) * root_vals.size()},
                             {reinterpret_cast<char*>(rcv_vals + peer_id * 2), sizeof(int) * 2}, root);
        } else {
            ch->scatter({}, {reinterpret_cast<char*>(rcv_vals + peer_id * 2), sizeof(int) * 2}, root);
        }
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(rcv_vals[i], i + 1);
            }
        } else {
            exit(0);
        }
    }
}

BOOST_AUTO_TEST_CASE(scatter_multiple) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 14;
        std::vector<int> root_vals(2 * num_peers);
        for (int i = 0; i < root_vals.size(); i++) {
            root_vals[i] = i + 1;
        }
        FMI::Utils::peer_num root = 3;

        int* rcv_vals = static_cast<int*>(mmap(nullptr, num_peers * 2 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        if (peer_id == root) {
            ch->scatter({reinterpret_cast<char*>(root_vals.data()), sizeof(root_vals[0]) * root_vals.size()},
                        {reinterpret_cast<char*>(rcv_vals + peer_id * 2), sizeof(int) * 2}, root);
        } else {
            ch->scatter({}, {reinterpret_cast<char*>(rcv_vals + peer_id * 2), sizeof(int) * 2}, root);
        }
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(rcv_vals[i], i + 1);
            }
        } else {
            exit(0);
        }
    }
}

BOOST_AUTO_TEST_CASE(reduce_multiple) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        FMI::Utils::peer_num root = 5;
        constexpr int num_peers = 13;
        int* res = static_cast<int*>(mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = *((int*) a) * *((int*) b);
        };
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        int val = peer_id + 1;
        if (peer_id == root) {
            ch->reduce({reinterpret_cast<char*>(&val), sizeof(int)}, {reinterpret_cast<char*>(res), sizeof(int)}, root, {f, true, true});
        } else {
            ch->reduce({reinterpret_cast<char*>(&val), sizeof(int)}, {}, root, {f, true, true});
        }

        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            int expected = 1;
            for (int i = 1; i < num_peers; i++) {
                expected *= (i + 1);
            }
            BOOST_CHECK_EQUAL(expected, *res);
        } else {
            exit(0);
        }

    }
}

BOOST_AUTO_TEST_CASE(reduce_multiple_ltr) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        FMI::Utils::peer_num root = 0;
        constexpr int num_peers = 8;
        int* res = static_cast<int*>(mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = *((int*) a) - *((int*) b);
        };
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        int val = peer_id + 1;
        if (peer_id == root) {
            ch->reduce({reinterpret_cast<char*>(&val), sizeof(int)}, {reinterpret_cast<char*>(res), sizeof(int)}, root, {f, false, false});
        } else {
            ch->reduce({reinterpret_cast<char*>(&val), sizeof(int)}, {}, root, {f, false, false});
        }

        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            int expected = 1;
            for (int i = 1; i < num_peers; i++) {
                expected -= (i + 1);
            }
            BOOST_CHECK_EQUAL(expected, *res);
        } else {
            exit(0);
        }

    }
}

BOOST_AUTO_TEST_CASE(allreduce_multiple) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 8;
        int* res = static_cast<int*>(mmap(nullptr, num_peers * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = *((int*) a) + *((int*) b);
        };
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        int val = peer_id + 1;
        ch->allreduce({reinterpret_cast<char*>(&val), sizeof(int)}, {reinterpret_cast<char*>(res + peer_id), sizeof(int)}, {f, true, true});

        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            int expected = 1;
            for (int i = 1; i < num_peers; i++) {
                expected += (i + 1);
            }
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(expected, res[i]);
            }
        } else {
            exit(0);
        }

    }
}

BOOST_AUTO_TEST_CASE(allreduce_multiple_ltr) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        FMI::Utils::peer_num root = 0;
        constexpr int num_peers = 8;
        int* res = static_cast<int*>(mmap(nullptr, num_peers * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = *((int*) a) - *((int*) b);
        };
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        int val = peer_id + 1;
        ch->allreduce({reinterpret_cast<char*>(&val), sizeof(int)}, {reinterpret_cast<char*>(res + peer_id), sizeof(int)}, {f, false, false});

        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            int expected = 1;
            for (int i = 1; i < num_peers; i++) {
                expected -= (i + 1);
            }
            for (int i = 0; i < num_peers; i++) {
                BOOST_CHECK_EQUAL(expected, res[i]);
            }
        } else {
            exit(0);
        }

    }
}

BOOST_AUTO_TEST_CASE(scan) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;
        
        constexpr int num_peers = 32;
        int* res = static_cast<int*>(mmap(nullptr, sizeof(int) * num_peers, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = *((int*) a) + *((int*) b);
        };
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        int val = peer_id + 1;
        ch->scan({reinterpret_cast<char*>(&val), sizeof(int)}, {reinterpret_cast<char*>(res + peer_id), sizeof(int)}, {f, true, true});
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            int prefix_sum = 0;
            for (int i = 0; i < num_peers; i++) {
                prefix_sum += (i + 1);
                BOOST_CHECK_EQUAL(prefix_sum, res[i]);
            }
        } else {
            exit(0);
        }


    }
}

BOOST_AUTO_TEST_CASE(scan_ltr) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second.first;
        auto model_params = backend_data.second.second;

        constexpr int num_peers = 8;
        int* res = static_cast<int*>(mmap(nullptr, sizeof(int) * num_peers, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        int peer_id = 0;
        for (int i = 1; i < num_peers; i ++) {
            int pid = fork();
            if (pid == 0) {
                peer_id = i;
                break;
            }
        }
        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = *((int*) a) - *((int*) b);
        };
        auto ch = FMI::Comm::Channel::get_channel(channel_name, test_params, model_params);
        ch->set_peer_id(peer_id);
        ch->set_num_peers(num_peers);
        ch->set_comm_name(comm_name);
        int val = peer_id;
        ch->scan({reinterpret_cast<char*>(&val), sizeof(int)}, {reinterpret_cast<char*>(res + peer_id), sizeof(int)}, {f, false, false});
        ch->finalize();
        if (peer_id == 0) {
            int status = 0;
            while (wait(&status) > 0);
            int result = 0;
            for (int i = 0; i < num_peers; i++) {
                result = result - i;
                BOOST_CHECK_EQUAL(result, res[i]);
            }
        } else {
            exit(0);
        }


    }
}

BOOST_AUTO_TEST_SUITE_END();