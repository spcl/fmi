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

std::map<std::string, std::string> redis_test_params = {
        {"host", "127.0.0.1"},
        {"port", "6379"},
        {"timeout", "1"},
        {"max_timeout", "1000"}
};

std::map<std::string, std::string> direct_test_params = {
        {"host", "192.168.0.166"},
        {"port", "10000"}
};

std::map<std::string, std::map<std::string, std::string>> backends = {
        //{"S3", s3_test_params},
        //{"Redis", redis_test_params},
        {"Direct", direct_test_params}
};

std::string comm_name = std::to_string(std::time(nullptr)) + "Tests";

BOOST_AUTO_TEST_CASE(sending_receiving) {
    for (auto const & backend_data : backends) {
        // Using C++ 17 [key, val] : map syntax here does not compile (on some clang versions) in combination with the omp section because of a clang bug:
        // https://stackoverflow.com/questions/65819317/openmp-clang-sometimes-fail-with-a-variable-declared-from-structured-binding
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second;

        int val = 42;
        int recv;
        #pragma omp parallel num_threads(2)
        {
            int tid = omp_get_thread_num();
            auto ch = SMI::Comm::Channel::get_channel(channel_name, test_params);
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
        auto test_params = backend_data.second;

        int val1 = 42;
        int val2 = 4242;
        int recv1, recv2;
        #pragma omp parallel num_threads(2)
        {
            int tid = omp_get_thread_num();
            auto ch = SMI::Comm::Channel::get_channel(channel_name, test_params);
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
        auto test_params = backend_data.second;
        SMI::Utils::peer_num root = 14;
        constexpr int num_peers = 21;
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
        auto ch = SMI::Comm::Channel::get_channel(channel_name, test_params);
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
        auto test_params = backend_data.second;
        auto ch_1 = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_1->set_peer_id(0);
        ch_1->set_num_peers(2);
        ch_1->set_comm_name(comm_name);
        std::chrono::steady_clock::time_point bef = std::chrono::steady_clock::now();
        ch_1->barrier();
        std::chrono::steady_clock::time_point after = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - bef).count();
        BOOST_TEST(elapsed_ms > std::stoi(test_params["max_timeout"]));
    }
}

BOOST_AUTO_TEST_CASE(barrier_succ) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second;
        auto ch_1 = SMI::Comm::Channel::get_channel(channel_name, test_params);
        auto ch_2 = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_1->set_peer_id(0);
        ch_1->set_num_peers(2);
        ch_1->set_comm_name(comm_name);
        ch_2->set_peer_id(1);
        ch_2->set_num_peers(2);
        ch_2->set_comm_name(comm_name);
        std::chrono::steady_clock::time_point bef = std::chrono::steady_clock::now();
        #pragma omp parallel num_threads(2)
        {
            int tid = omp_get_thread_num();
            if (tid == 0) {
                ch_1->barrier();
            } else {
                ch_2->barrier();
            }
        }


        std::chrono::steady_clock::time_point after = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(after - bef).count();

        ch_1->finalize();
        ch_2->finalize();
        BOOST_TEST(elapsed_ms < std::stoi(test_params["max_timeout"]));
    }

}

BOOST_AUTO_TEST_CASE(gather_one) {
    for (auto const & [channel_name, test_params] : backends) {
        constexpr int num_peers = 2;
        std::vector<int> vals {1,2};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);
        std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers - 1);

        for (int i = 1; i < num_peers; i++) {
            auto ch_rcv = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch_rcv->set_peer_id(i);
            ch_rcv->set_num_peers(num_peers);
            ch_rcv->set_comm_name(comm_name);
            ch_rcv->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()}, {}, 0);
            channels[i - 1] = ch_rcv;
        }
        std::vector<int> vals_rcv(num_peers * 2, 0);
        ch_root->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()},
                        {reinterpret_cast<char*>(vals_rcv.data()), sizeof(vals[0]) * vals_rcv.size()},  0);
        std::vector<int> expected(num_peers * 2, 1);
        for (int i = 1; i < expected.size(); i += 2) {
            expected[i] = 2;
        }
        ch_root->finalize();
        for (int i = 1; i < num_peers; i++) {
            channels[i - 1]->finalize();
        }
        BOOST_TEST(vals_rcv == expected, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(gather_multiple) {
    for (auto const & [channel_name, test_params] : backends) {
        constexpr int num_peers = 4;
        std::vector<int> vals {1,2};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);
        std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers - 1);

        for (int i = 1; i < num_peers; i++) {
            auto ch_rcv = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch_rcv->set_peer_id(i);
            ch_rcv->set_num_peers(num_peers);
            ch_rcv->set_comm_name(comm_name);
            ch_rcv->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()}, {}, 0);
            channels[i - 1] = ch_rcv;
        }
        std::vector<int> vals_rcv(num_peers * 2, 0);
        ch_root->gather({reinterpret_cast<char*>(vals.data()), sizeof(vals[0]) * vals.size()},
                        {reinterpret_cast<char*>(vals_rcv.data()), sizeof(vals[0]) * vals_rcv.size()},  0);
        std::vector<int> expected(num_peers * 2, 1);
        for (int i = 1; i < expected.size(); i += 2) {
            expected[i] = 2;
        }
        ch_root->finalize();
        for (int i = 1; i < num_peers; i++) {
            channels[i - 1]->finalize();
        }
        BOOST_TEST(vals_rcv == expected, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(scatter_one) {
    for (auto const & [channel_name, test_params] : backends) {
        constexpr int num_peers = 2;
        std::vector<int> root_vals {1,2,3,4};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);

        std::vector<std::vector<int>> rcv_vals(num_peers);
        rcv_vals[0].resize(2);
        ch_root->scatter({reinterpret_cast<char*>(root_vals.data()), sizeof(root_vals[0]) * root_vals.size()},
                         {reinterpret_cast<char*>(rcv_vals[0].data()), sizeof(rcv_vals[0][0]) * rcv_vals[0].size()}, 0);
        for (int i = 1; i < num_peers; i++) {
            auto ch_rcv = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch_rcv->set_peer_id(i);
            ch_rcv->set_num_peers(num_peers);
            ch_rcv->set_comm_name(comm_name);
            rcv_vals[i].resize(2);
            ch_rcv->scatter({}, {reinterpret_cast<char*>(rcv_vals[i].data()), sizeof(rcv_vals[i][0]) * rcv_vals[i].size()}, 0);
            ch_rcv->finalize();
        }


        ch_root->finalize();
        for (int i = 0; i < num_peers; i++) {
            std::vector<int> expected(2);
            expected[0] = 2 * i + 1;
            expected[1] = 2 * i + 2;
            BOOST_TEST(rcv_vals[i] == expected, boost::test_tools::per_element());
        }
    }
}

BOOST_AUTO_TEST_CASE(scatter_multiple) {
    for (auto const & [channel_name, test_params] : backends) {
        constexpr int num_peers = 4;
        std::vector<int> root_vals {1,2,3,4,5,6,7,8};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);

        std::vector<std::vector<int>> rcv_vals(num_peers);
        rcv_vals[0].resize(2);
        ch_root->scatter({reinterpret_cast<char*>(root_vals.data()), sizeof(root_vals[0]) * root_vals.size()},
                         {reinterpret_cast<char*>(rcv_vals[0].data()), sizeof(rcv_vals[0][0]) * rcv_vals[0].size()}, 0);
        for (int i = 1; i < num_peers; i++) {
            auto ch_rcv = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch_rcv->set_peer_id(i);
            ch_rcv->set_num_peers(num_peers);
            ch_rcv->set_comm_name(comm_name);
            rcv_vals[i].resize(2);
            ch_rcv->scatter({}, {reinterpret_cast<char*>(rcv_vals[i].data()), sizeof(rcv_vals[i][0]) * rcv_vals[i].size()}, 0);
            ch_rcv->finalize();
        }


        ch_root->finalize();
        for (int i = 0; i < num_peers; i++) {
            std::vector<int> expected(2);
            expected[0] = 2 * i + 1;
            expected[1] = 2 * i + 2;
            BOOST_TEST(rcv_vals[i] == expected, boost::test_tools::per_element());
        }
    }
}

BOOST_AUTO_TEST_CASE(reduce_single) {
    for (auto const & [channel_name, test_params] : backends) {
        constexpr int num_peers = 2;
        std::vector<int> vals {1,2};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);

        std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers - 1);

        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = ((int) *a + (int) *b);
        };
        for (int i = 1; i < num_peers; i++) {
            auto ch_rcv = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch_rcv->set_peer_id(i);
            ch_rcv->set_num_peers(num_peers);
            ch_rcv->set_comm_name(comm_name);
            ch_rcv->reduce({reinterpret_cast<char*>(&vals[i]), sizeof(vals[i])}, {}, 0, {f, true, true});
            channels[i - 1] = ch_rcv;
        }
        int res;
        ch_root->reduce({reinterpret_cast<char*>(&vals[0]), sizeof(vals[0])},
                        {reinterpret_cast<char*>(&res), sizeof(res)}, 0, {f, true, true});
        for (int i = 0; i < num_peers - 1; i++) {
            channels[i]->finalize();
        }
        ch_root->finalize();
        BOOST_TEST(res == std::accumulate(vals.begin(), vals.end(), 0));
    }
}

BOOST_AUTO_TEST_CASE(reduce_multiple) {
    for (auto const & [channel_name, test_params] : backends) {
        constexpr int num_peers = 4;
        std::vector<int> vals {1,2,3,4};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);

        std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers - 1);

        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = ((int) *a * (int) *b);
        };
        for (int i = 1; i < num_peers; i++) {
            auto ch_rcv = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch_rcv->set_peer_id(i);
            ch_rcv->set_num_peers(num_peers);
            ch_rcv->set_comm_name(comm_name);
            ch_rcv->reduce({reinterpret_cast<char*>(&vals[i]), sizeof(vals[i])}, {}, 0, {f, true, true});
            channels[i - 1] = ch_rcv;
        }
        int res;
        ch_root->reduce({reinterpret_cast<char*>(&vals[0]), sizeof(vals[0])},
                        {reinterpret_cast<char*>(&res), sizeof(res)}, 0, {f, true, true});
        for (int i = 0; i < num_peers - 1; i++) {
            channels[i]->finalize();
        }
        ch_root->finalize();
        BOOST_TEST(res == std::accumulate(vals.begin(), vals.end(), 1, std::multiplies<int>()));
    }
}

BOOST_AUTO_TEST_CASE(allreduce) {
    for (auto const & backend_data : backends) {
        // Using C++ 17 [key, val] : map syntax here does not compile (on some clang versions) in combination with the omp section because of a clang bug:
        // https://stackoverflow.com/questions/65819317/openmp-clang-sometimes-fail-with-a-variable-declared-from-structured-binding
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second;
        constexpr int num_peers = 4;
        std::vector<int> vals {1,2,3,4};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);

        std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers);
        std::vector<int> results(num_peers);

        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = ((int) *a * (int) *b);
        };

        #pragma omp parallel num_threads(num_peers)
        {
            int tid = omp_get_thread_num();
            auto ch = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch->set_peer_id(tid);
            ch->set_num_peers(num_peers);
            ch->set_comm_name(comm_name);
            channels[tid] = ch;
            ch->allreduce({reinterpret_cast<char*>(&vals[tid]), sizeof(vals[tid])},
                          {reinterpret_cast<char*>(&results[tid]), sizeof(results[tid])}, {f, true, true});
        }


        for (int i = 0; i < num_peers; i++) {
            channels[i]->finalize();
            BOOST_TEST(results[i] == std::accumulate(vals.begin(), vals.end(), 1, std::multiplies<int>()));
        }
    }
}

BOOST_AUTO_TEST_CASE(scan) {
    for (auto const & backend_data : backends) {
        auto channel_name = backend_data.first;
        auto test_params = backend_data.second;
        constexpr int num_peers = 4;
        std::vector<int> vals {1,2,3,4};
        auto ch_root = SMI::Comm::Channel::get_channel(channel_name, test_params);
        ch_root->set_peer_id(0);
        ch_root->set_comm_name(comm_name);
        ch_root->set_num_peers(num_peers);

        std::vector<std::shared_ptr<SMI::Comm::Channel>> channels(num_peers);
        std::vector<int> results(num_peers);

        auto f = [] (char* a, char* b) {
            int* dest = reinterpret_cast<int*>(a);
            *dest = ((int) *a + (int) *b);
        };
    #pragma omp parallel num_threads(num_peers)
        {
            int tid = omp_get_thread_num();
            auto ch = SMI::Comm::Channel::get_channel(channel_name, test_params);
            ch->set_peer_id(tid);
            ch->set_num_peers(num_peers);
            ch->set_comm_name(comm_name);
            channels[tid] = ch;
            ch->scan({reinterpret_cast<char*>(&vals[tid]), sizeof(vals[tid])},
                     {reinterpret_cast<char*>(&results[tid]), sizeof(results[tid])}, {f, true, true});
        }


        int prefix_sum = 0;
        for (int i = 0; i < num_peers; i++) {
            channels[i]->finalize();
            prefix_sum += vals[i];
            BOOST_TEST(results[i] == prefix_sum);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END();