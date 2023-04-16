#ifndef __RDMARB_FARM__
#define __RDMARB_FARM__
#include "common.h"
#include <atomic>
#include <cstdint>
#include <cstdio>
struct FarmParameter {
  uint32_t buffer_size;
  uint32_t inflight_post_threshold;
};

/// size of msg header (in Byte)
static int HEADER_SIZE = 4;

class RingBufferFarm : virtual public RingBuffer {
protected:
  FarmParameter param;
  inline uint8_t *getPointer(uint32_t index) { return data.get() + index; }
  RingBufferFarm(const char *device_name, FarmParameter param);
};
class RingBufferFarmConsumer final : public RingBufferFarm,
                                     public RingBufferConsumer {
private:

  /// head hold by producer
  uint32_t last_head;
  /// current head: the first place have no data.
  /// the first valid byte is placed at (head + 1) % buf_len
  
  uint32_t head;
  uint32_t buf_len;
  inline bool needUpdateHeadToRemote() {
    return (last_head < head && head - last_head > buf_len / 2) ||
           (last_head > head && buf_len - last_head + head > buf_len / 2);
  }
  void updateHeadToRemote();
  void wcHandler();

public:
  RingBufferFarmConsumer(const char *device_name, FarmParameter param);
  ~RingBufferFarmConsumer();
  
  /** Exchanges necessary information via out-of-band TCP connection to
   * establish connection between RDMA QPs. */
  void exchangeQPInfo(const char *server, uint16_t port);

  /** Modifies QP state and starts messaging. */
  void start();

  /** Receives a message and saves its length.
   * @return Returns 0 if a message is successfully received. Returns -1 if
   * there is temporarily no message to receive. */
  int recv(void *message, uint32_t &length);

  /** Receives a message and saves its length. 
   * Blocks if there is temporarily no message to receive, 
   * until a message is available. */
  void recvBlock(void *message, uint32_t &length);
};

class RingBufferFarmProducer final : public RingBufferFarm,
                                     public RingBufferProducer {
private:
  /// the first place has no data.
  uint32_t head;

  /// next place to put data
  uint32_t tail;

  uint32_t buf_len;
  int inflight_send_count;
  int inflight_recv_count;
  void postSend(uint32_t begin, uint32_t end);
  inline uint64_t getRemotePointer(unsigned index) {
    return remote_qp_info.addr + index;
  }
  inline uint8_t *getPointer(uint32_t index) { return data.get() + index; }
  void __postSend(uint32_t begin, uint32_t length);
  void postRecv();
  void wcHandler();

public:
  RingBufferFarmProducer(const char *device_name, FarmParameter param);
  ~RingBufferFarmProducer();
  /** Exchanges necessary information via out-of-band TCP connection to
   * establish connection between RDMA QPs. */
  void exchangeQPInfo(const char *server, uint16_t port);

  /** Modifies QP state and starts messaging. */
  void start();

  /** Sends a message of length bytes.
   * @return Returns 0 if the message is successfully posted. Returns -1 if it
   * is temporarily unavailable to send the message */
  int send(const void *message, uint32_t length);

  /** Sends a message of length bytes. Blocks if it is temporarily unavailable
   * to send the message, until the message is posted. */
  void sendBlock(const void *message, uint32_t length);
};

#endif
