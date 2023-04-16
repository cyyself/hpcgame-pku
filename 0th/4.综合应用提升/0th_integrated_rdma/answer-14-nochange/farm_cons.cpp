#include <getopt.h>
#include <unistd.h>

#include <iomanip>
#include <fstream>

#include "demo.h"
#include <iostream>
#include <chrono>

using namespace std;


int main(int argc, char **argv) {
  
  if(argc > 1) {
    _validation_needed = 1;
  }

  const char *server_name = producer_name;

  for(auto msgsize : msg_size_list) {
    printf("consumer(%lu msgsize) running\n", msgsize);

    param.buffer_size = get_ringbuffer_size(msgsize);
    uint64_t iterations = get_iterations(msgsize);


    uint8_t *message = new uint8_t[msgsize]();

    { /* thput test */
        RingBufferFarmConsumer rb(device_name, param);
        rb.exchangeQPInfo("0.0.0.0", port);
        rb.start();

        auto start = std::chrono::system_clock::now();
        void *msgend = message + msgsize;
        for (uint64_t round = 0; round < iterations; ++round) {
          uint32_t _msgsize;
          rb.recvBlock(message, _msgsize = msgsize);
          if (_msgsize != msgsize) {
            printf("recv len mismatch at round %lu\n", round);
            exit(1);
          }
          if (*(uint64_t*)message != round ||
              *(uint64_t*)(message + msgsize - 8) != round) {
            printf("validate error at round %lu\n", round);
            exit(1);
          }

          if(_validation_needed)
            for(void *z = (void*)message; z < msgend; z += 8) {
              if (*(uint64_t*)z != round) {
                printf("validate error at round %lu\n", round);
                exit(1);
              }
            }
        }
        auto end = std::chrono::system_clock::now();
        auto duration =
                  std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        printf("=== Finish ===\n");
        double trans_size = 1.0 * msgsize * iterations;
        printf("  %.3lf s, %.2lf MiB/s\n", duration.count() / 1e6,
                    trans_size / (1 << 20) / (duration.count() / 1e6));
        time_t tt = std::chrono::system_clock::to_time_t(end);
        auto formatted_date = std::string(30, 0);
        std::strftime(&formatted_date[0], formatted_date.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));

        char host_name[255];
        gethostname(host_name, sizeof(host_name));
        printf("Tuple size(B)       %lu\n", msgsize);
        printf("Tuples count        %lu\n", iterations);
        printf("Time(s)             %.3lf\n", duration.count() / 1e6);
        printf("Speed(MiB/s)        %.3lf\n", trans_size / (1 << 20) / (duration.count() / 1e6));

        printf("=== Cleaning ===\n");
        rb.close_connection();
        sleep(1);
    }
  }
}