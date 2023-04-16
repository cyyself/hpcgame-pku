#ifndef __RDMARB_COMMON__
#define __RDMARB_COMMON__
#define MAX_GID_COUNT 128
#define use_RoCE_v2 0      // Cautious: there may be some bugs when using RoCE_v2

#include <infiniband/verbs.h>

#include <memory>

static const size_t CACHELINE_SIZE = 64;

struct QPInfo {
  uint16_t lid;
  uint32_t qpn;
  uint32_t psn;
  uint64_t addr;
  uint32_t rkey;
  // Global identifier
  union ibv_gid gid;
  // GUID
  uint64_t guid;
  template <typename OStream>
  friend OStream &operator<<(OStream &os, const QPInfo &c);
};

class RingBuffer {
protected:
  std::unique_ptr<uint8_t[]> data;

  QPInfo local_qp_info, remote_qp_info;
  int max_cqe_;
  int max_send_wr_;
  int max_recv_wr_;
  ibv_device **devices = NULL;
  ibv_context *ctx = NULL;
  ibv_pd *pd = NULL;
  ibv_cq *cq = NULL;
  ibv_qp *qp = NULL;
  ibv_mr *mr = NULL;
  ibv_wc *wc_arr = new ibv_wc[10];
  int gid_index_list[MAX_GID_COUNT];
  union ibv_gid gid_list[MAX_GID_COUNT];
  size_t gid_count;
  // GUID
  uint64_t guid;
  unsigned int            gid_index;
  RingBuffer(const char *device_name, int max_cqe = 0, int max_send_wr = 0,
             int max_recv_wr = 0);
  ~RingBuffer();

  static uint32_t generatePSN();
  void allocateMemory(size_t size, int access);
  void modifyQP(unsigned qp_access_flags);
  void get_rocev2_gid_index(char *ib_dev_name, int dev_port);
};

class RingBufferConsumer : virtual public RingBuffer {
protected:
  int sock_fd;
  using RingBuffer::RingBuffer;
  void exchangeQPInfo(const char *server_name, uint16_t port,
                      const void *additional_write = NULL, size_t write_len = 0,
                      void *additional_read = NULL, size_t read_len = 0);

public:
  void close_connection();
};

class RingBufferProducer : virtual public RingBuffer {
protected:
  int sock_fd;
  using RingBuffer::RingBuffer;

  void exchangeQPInfo(const char *server_name, uint16_t port,
                      const void *additional_write = NULL, size_t write_len = 0,
                      void *additional_read = NULL, size_t read_len = 0);

public:
  void close_connection();
};

size_t write_exact(int fd, const void *buf, size_t count);
size_t read_exact(int fd, void *buf, size_t count);

template <typename T> class LocalBatch {
  T raw, *buffer;
  int upd_times;

public:
  LocalBatch(T init, const int batch_arg) : raw(init), upd_times(0){};

  void update(T _new_) { raw = _new_; }

  void set_buffer(T *b) { buffer = b; }

  T *get_buffer() { return buffer; }

  operator T() { return raw; }
};

template <typename T> class RemoteSync {
  T raw;
  T *remote_ptr = NULL;

public:
  RemoteSync(T init) : raw(init){};
  void set_remote_ptr(T *ptr) { remote_ptr = ptr; }
  void sync_from_remote(RingBuffer *rb) {
    if (remote_ptr == NULL)
      throw;
    ibv_sge list{reinterpret_cast<uint64_t>(&raw), sizeof(T), rb->mr->lkey};
    ibv_send_wr wr, *bad_wr;
    memset(&wr, 0, sizeof(ibv_send_wr));
    wr.wr_id = 1;
    wr.sg_list = &list;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_RDMA_READ;
    // // wr.send_flags = IBV_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = reinterpret_cast<uint64_t>(remote_ptr);
    wr.wr.rdma.rkey = rb->remote_qp_info.rkey;

    if (ibv_post_send(rb->qp, &wr, &bad_wr)) {
      // spdlog::error(
      //     "ibv_post_send failed errno={} errmsg=\"{}\"",
      //     errno, strerror(errno));
      throw;
    }
  }
  operator T() { return raw; }
};

#endif