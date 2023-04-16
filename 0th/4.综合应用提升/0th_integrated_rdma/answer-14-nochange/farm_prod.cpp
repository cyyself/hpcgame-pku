#include <getopt.h>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <unistd.h>

#include "demo.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
  if(argc > 1) {
    _validation_needed = 1;
  }
  std::ofstream fout("prod.csv", std::ios_base::out);

  const char *server_name = consumer_name;

  for(auto msgsize : msg_size_list) {
    printf("producer(%lu msgsize) running\n", msgsize);

    param.buffer_size = get_ringbuffer_size(msgsize);
    uint8_t *message = new uint8_t[msgsize]();

    { /* thput test */
      uint64_t iterations = get_iterations(msgsize);
      RingBufferFarmProducer rb(device_name, param);
      rb.exchangeQPInfo(server_name, port);
      rb.start();
      fout << "\t\t\t msg_size = " << msgsize << endl;


      printf("=== Start ThroughPut Test ===\n");
      fout << "Size of message = " << msgsize << endl;
      auto start = std::chrono::system_clock::now();

      void *msgend = message + msgsize;
      for (uint64_t round = 0; round < iterations; ++round) {
        *(uint64_t*)message = round;
        *(uint64_t*)(message + msgsize - 8) = round;

        if(_validation_needed)
          for(void *z = message; z < msgend; z += 8) {
            *(uint64_t*)z = round;
          }

        rb.sendBlock(message, msgsize);
      }
      auto end = std::chrono::system_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      printf("=== Finish ===\n");
      double trans_size = 1.0 * msgsize * iterations;
      printf("size %.0lf B,  %.2lf ms, %.2lf MiB/s\n", trans_size, duration.count() / 1e3,
                  trans_size / (1 << 20) / (duration.count() / 1e6));
      printf("=== Cleaning ===\n");


      rb.close_connection();
      
      fout << std::fixed << std::setprecision(6)
           << trans_size / (1 << 20) / (duration.count() / 1e6)
           << std::endl;
    }
  }
  
}