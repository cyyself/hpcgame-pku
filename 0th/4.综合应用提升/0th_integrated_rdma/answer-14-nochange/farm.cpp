#include "farm.h"
#include <thread>
#include <unistd.h>

inline static uint32_t ceil8(uint32_t num) {
  return num % 8 == 0 ? num : (num + 7) / 8 * 8;
}

inline static uint32_t ceil4(uint32_t num) {
  return num % 4 == 0 ? num : (num + 3) / 4 * 4;
}

RingBufferFarm::RingBufferFarm(const char *device_name, FarmParameter param)
    : RingBuffer(device_name), param(param) {}

RingBufferFarmConsumer::RingBufferFarmConsumer(const char *device_name,
                                               FarmParameter param)
    : RingBuffer(device_name), RingBufferFarm(device_name, param),
      RingBufferConsumer(device_name) {
  allocateMemory(param.buffer_size, IBV_ACCESS_LOCAL_WRITE |
                                        IBV_ACCESS_REMOTE_READ |
                                        IBV_ACCESS_REMOTE_WRITE);
  memset(this->data.get(), 0, param.buffer_size);

  this->head = 0;
  this->last_head = 0;
  this->buf_len = param.buffer_size;
}

RingBufferFarmConsumer::~RingBufferFarmConsumer() {
}

void RingBufferFarmConsumer::exchangeQPInfo(const char *server_name,
                                            uint16_t port) {
  RingBufferConsumer::exchangeQPInfo(server_name, port);
}

void RingBufferFarmConsumer::start() {
  modifyQP(IBV_ACCESS_REMOTE_WRITE);
}

void RingBufferFarmConsumer::wcHandler() {
  while (1) {
    int count = ibv_poll_cq(cq, 1, this->wc_arr);
    if (count < 0) {
      fprintf(stderr, "ibv_poll_wc failed @ SDConsumer: {}", strerror(errno));
      break;
    }
    if (count == 0)
      break;
    for (int i = 0; i < count; ++i) {
      ibv_wc *wc = &wc_arr[i];
      if (wc->status != IBV_WC_SUCCESS) {
        fprintf(stderr, "wc not success: wr_id={}, status=%s\n", wc->wr_id,
                      ibv_wc_status_str(wc->status));
        break;
      }
    }
  }
}

int RingBufferFarmConsumer::recv(void *message, uint32_t &length) {
  wcHandler();
  uint32_t first_data_loc = this->head + 8;
  if (first_data_loc == buf_len) first_data_loc = 0;

  const uint32_t msg_len =
      *reinterpret_cast<volatile uint32_t *>(getPointer(first_data_loc));
  if (msg_len == 0) {
    return -1;
  }
  uint32_t byte_take = ceil8(msg_len + HEADER_SIZE);
  length = msg_len;
  memcpy(message, getPointer(first_data_loc + HEADER_SIZE), msg_len);
  memset(getPointer(first_data_loc), 0, byte_take);
  this->head = (this->head + byte_take) % this->buf_len;
  if (needUpdateHeadToRemote()) {
    updateHeadToRemote();
  }
  return 0;
}

void RingBufferFarmConsumer::updateHeadToRemote() {
  wcHandler();
  ibv_send_wr wr, *bad_wr;
  memset(&wr, 0, sizeof(ibv_send_wr));
  wr.wr_id = 1;
  wr.opcode = IBV_WR_SEND_WITH_IMM;
  wr.imm_data = this->head;
  if (ibv_post_send(qp, &wr, &bad_wr)) {
    fprintf(stderr, 
        "ibv_post_send failed @ updateHeadToRemote: errno={} errmsg=\"{}\"",
        errno, strerror(errno));
    throw;
  } 
  last_head = head;
}

void RingBufferFarmConsumer::recvBlock(void *message, uint32_t &length) {
  uint32_t total = 0, now = 0;
  while (total < length) {
    now = length - total;
    while (recv(message + total, now) == -1)
      ;
    total += now;
  }
  length = total;
}

RingBufferFarmProducer::RingBufferFarmProducer(const char *device_name,
                                               FarmParameter param)
    : RingBuffer(device_name), RingBufferFarm(device_name, param),
      RingBufferProducer(device_name) {
  allocateMemory(param.buffer_size, IBV_ACCESS_LOCAL_WRITE);

  this->head = 0;
  this->tail = 8;
  this->buf_len = param.buffer_size;
  this->inflight_send_count = 0;
  this->inflight_recv_count = 0;
  memset(this->data.get(), 0, buf_len);
}

RingBufferFarmProducer::~RingBufferFarmProducer() {
}

void RingBufferFarmProducer::exchangeQPInfo(const char *server_name,
                                            uint16_t port) {
  RingBufferProducer::exchangeQPInfo(server_name, port);
}

void RingBufferFarmProducer::postRecv() {
  static ibv_recv_wr wr{2, NULL, NULL, 0}, *bad_wr;
  ibv_post_recv(qp, &wr, &bad_wr);
}

void RingBufferFarmProducer::wcHandler() {
  while (1) {
    int count = ibv_poll_cq(cq, 1, wc_arr);
    if (count < 0) {
      fprintf(stderr, "ibv_poll_wc failed @ FarmProducer: {}", strerror(errno));
      break;
    }
    if (count == 0)
      break;
    for (int i = 0; i < count; ++i) {
      ibv_wc *wc = &wc_arr[i];
      if (wc->status != IBV_WC_SUCCESS) {
        fprintf(stderr, "wc not success: wr_id={:x}, status={}, remote={:#x}",
                      wc->wr_id, ibv_wc_status_str(wc->status),
                      remote_qp_info.addr);
        break;
      }
      if (wc->wr_id == 1) { // RDMA_WRITE
        --inflight_send_count;
      } else if (wc->wr_id == 2) { // recv head update
        head = wc->imm_data;
        postRecv();
      }
    }
  }
}

void RingBufferFarmProducer::start() {
  modifyQP(IBV_ACCESS_REMOTE_WRITE);
  for (unsigned i = 0; i < 10; i++) {
    postRecv();
  }
}

int RingBufferFarmProducer::send(const void *message, uint32_t length) {
  wcHandler();

  uint32_t byte_needed = length + HEADER_SIZE;
  if (head == tail) return -1; // full queue

  if (head < tail) {
    uint32_t free_space_to_end = buf_len - tail; // at least 8
    if (free_space_to_end >= byte_needed) {
      // one msg
      *(uint32_t*)getPointer(tail) = length;
      memcpy(getPointer(tail + HEADER_SIZE), message, length);
      postSend(tail, byte_needed);

      tail += ceil8(byte_needed);
      if (tail == buf_len) tail = 0;
      return 0;
    } else {
      // over the boundary, two message

      uint32_t capacity = free_space_to_end + head;
      if (capacity < HEADER_SIZE * 2 + length) {
        // need more space
        return -1;
      }

      uint32_t msg1_len = free_space_to_end - HEADER_SIZE;
      *(uint32_t*)getPointer(tail) = msg1_len;
      memcpy(getPointer(tail + HEADER_SIZE), message, msg1_len);
      postSend(tail, free_space_to_end);

      tail = 0;
      uint32_t msg2_len = length - msg1_len;
      *(uint32_t*)getPointer(tail) = msg2_len;
      memcpy(getPointer(tail + HEADER_SIZE), message + msg1_len, msg2_len);      
      postSend(tail, msg2_len + HEADER_SIZE);

      tail += ceil8(HEADER_SIZE + msg2_len);
      if (tail == buf_len) tail = 0;
      return 0;
    }
  } else {
    // head > tail situation
    uint32_t capacity = head - tail;
    if (capacity < byte_needed) 
      return -1;
    *(uint32_t*)getPointer(tail) = length;
    memcpy(getPointer(tail + HEADER_SIZE), message, length);
    postSend(tail, byte_needed);

    tail += ceil8(byte_needed);
    if (tail == buf_len) tail = 0;
    return 0;
  }
}

void RingBufferFarmProducer::__postSend(uint32_t begin, uint32_t length) {
  wcHandler();
  ibv_sge sg_list;
  sg_list.addr = reinterpret_cast<uint64_t>(getPointer(begin));
  sg_list.length = length;
  sg_list.lkey = mr->lkey;
  ibv_send_wr wr;
  ibv_send_wr *bad_wr;
  memset(&wr, 0, sizeof(wr));
  wr.wr_id = 1;
  wr.sg_list = &sg_list;
  wr.num_sge = 1;
  wr.opcode = IBV_WR_RDMA_WRITE;
  wr.wr.rdma.remote_addr = getRemotePointer(begin);
  wr.wr.rdma.rkey = remote_qp_info.rkey;
  wr.next = NULL;
  while (inflight_send_count >= max_send_wr_) {
  }
  if (ibv_post_send(qp, &wr, &bad_wr)) {
    fprintf(stderr, "ibv_post_send failed @ postSend: errno={} errmsg=\"{}\"",
                  errno, strerror(errno));
    throw;
  }
  inflight_send_count++;
}

void RingBufferFarmProducer::postSend(uint32_t begin, uint32_t length) {
  __postSend(begin + HEADER_SIZE, length - HEADER_SIZE);
  __postSend(begin, HEADER_SIZE);
}

void RingBufferFarmProducer::sendBlock(const void *message, uint32_t length) {
  while (send(message, length) == -1)
    ;
}