#include "common.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>

#include <random>
#include <iomanip>

#include <dirent.h>
// Check if the gid referenced by gid_index is a ipv4-gid
static bool is_ipv4_gid(char *ib_dev_name, int dev_port, int gid_index) {
  char file_name[384] = {0};
  static const char ipv4_gid_prefix[] = "0000:0000:0000:0000:0000:ffff:";
  FILE *fp = NULL;
  ssize_t read;
  char *line = NULL;
  size_t len = 0;
  snprintf(file_name, sizeof(file_name),
           "/sys/class/infiniband/%s/ports/%d/gids/%d", ib_dev_name, dev_port,
           gid_index);
  fp = fopen(file_name, "r");
  if (!fp) {
    return false;
  }
  read = getline(&line, &len, fp);
  fclose(fp);
  if (!read) {
    return false;
  }
  return strncmp(line, ipv4_gid_prefix, strlen(ipv4_gid_prefix)) == 0;
}
static int cmpfunc(const void *a, const void *b) {
  return (*(int *)a - *(int *)b);
}
// Get the index of all the GIDs whose types are RoCE v2
// Refer to
// https://docs.mellanox.com/pages/viewpage.action?pageId=12013422#RDMAoverConvergedEthernet(RoCE)-RoCEv2
// for more details
void RingBuffer::get_rocev2_gid_index(char *ib_dev_name, int dev_port) {
  const size_t max_gid_count =
      sizeof(this->gid_index_list) / sizeof(this->gid_index_list[0]);
  int gid_index_list[max_gid_count];
  size_t gid_count = 0;

  this->gid_count = 0;

  char dir_name[128] = {0};
  snprintf(dir_name, sizeof(dir_name),
           "/sys/class/infiniband/%s/ports/%d/gid_attrs/types", ib_dev_name,
           dev_port);
  DIR *dir = opendir(dir_name);
  if (!dir) {
    fprintf(stderr, "Fail to open folder %s\n", dir_name);
    return;
  }

  struct dirent *dp = NULL;
  char file_name[384] = {0};
  FILE *fp = NULL;
  ssize_t read;
  char *line = NULL;
  size_t len = 0;
  int gid_index;

  while ((dp = readdir(dir)) && gid_count < max_gid_count) {
    gid_index = atoi(dp->d_name);
    if (gid_index < 2) {
      continue;
    }

    snprintf(file_name, sizeof(file_name), "%s/%s", dir_name, dp->d_name);
    fp = fopen(file_name, "r");
    if (!fp) {
      continue;
    }

    read = getline(&line, &len, fp);
    fclose(fp);
    if (read <= 0) {
      continue;
    }

    if (strncmp(line, "RoCE v2", strlen("RoCE v2")) != 0) {
      continue;
    }

    if (!is_ipv4_gid(ib_dev_name, dev_port, gid_index)) {
      continue;
    }

    gid_index_list[gid_count++] = gid_index;
  }

  closedir(dir);

  qsort(gid_index_list, gid_count, sizeof(int), cmpfunc);
  this->gid_count = gid_count;
  for (size_t i = 0; i < gid_count; i++) {
    this->gid_index_list[i] = gid_index_list[i];
    printf("gid index is %d\n", this->gid_index_list[i]);
  }
  // Debug
  printf("Get %lu RoCE V2 GIDs\n", this->gid_count);
}
template <typename OStream> OStream &operator<<(OStream &os, const QPInfo &c) {
  return os << "lid=" << c.lid << " qpn=" << c.qpn << " psn=" << c.psn
            << std::hex << " addr=0x" << c.addr << std::dec
            << " rkey=" << c.rkey;
}

RingBuffer::RingBuffer(const char *device_name, int max_cqe, int max_send_wr,
                       int max_recv_wr) {
  int num_devices;
  devices = ibv_get_device_list(&num_devices);
  ibv_device *device = NULL;
  for (int i = 0; i < num_devices; ++i) {
    if (!strcmp(ibv_get_device_name(devices[i]), device_name)) {
      device = devices[i];
      break;
    }
  }

  ctx = ibv_open_device(device);
  if (ctx == nullptr) {
    fprintf(stderr, "open device error\n");
  }
  // Get GUID (Node global unique identifier)
  this->guid = ibv_get_device_guid(device);
  this->get_rocev2_gid_index(const_cast<char *>(device_name), 1);
  // Get the index of GIDs whose types are RoCE v2
  if (use_RoCE_v2 && this->gid_count == 0) {
    fprintf(stderr, "Cannot find any RoCE v2 GID\n");
  }
  // Get RoCE v2 GIDs
  for (size_t i = 0; i < this->gid_count; i++) {
    if (ibv_query_gid(this->ctx, 1, this->gid_index_list[i],
                      &(this->gid_list[i])) != 0) {
      fprintf(stderr, "Cannot read GID of index %d\n", this->gid_index_list[i]);
    }
  }
  this->gid_index = this->gid_index_list[0];
  this->local_qp_info.gid = this->gid_list[0];
  this->local_qp_info.guid = this->guid;

  pd = ibv_alloc_pd(ctx);
  if (pd == nullptr) {
    fprintf(stderr, "create pd error\n");
  }
  ibv_device_attr dev_attr;
  ibv_query_device(ctx, &dev_attr);
  if (max_send_wr == 0) {
    max_send_wr = dev_attr.max_qp_wr / 4;
  } else {
    max_send_wr = std::min(max_send_wr, dev_attr.max_qp_wr / 4);
  }
  if (max_recv_wr == 0) {
    max_recv_wr = dev_attr.max_qp_wr / 4;
  } else {
    max_recv_wr = std::min(max_recv_wr, dev_attr.max_qp_wr / 4);
  }
  if (max_cqe == 0) {
    max_cqe = std::min(dev_attr.max_cqe - 10, max_send_wr + max_recv_wr + 10);
  } else {
    max_cqe = std::min(max_cqe, dev_attr.max_cqe - 10);
    max_cqe = std::min(max_cqe, max_send_wr + max_recv_wr + 10);
  }
  this->max_cqe_ = max_cqe;
  this->max_send_wr_ = max_send_wr - 10;
  this->max_recv_wr_ = max_recv_wr - 10;
  cq = ibv_create_cq(ctx, max_cqe, NULL, NULL, 0);
  if(cq == nullptr) {
    fprintf(stderr, "create cq error\n");
    // print the error code
    fprintf(stderr, "error code is %d\n", errno);
    fprintf(stderr, "error message is %s\n", strerror(errno));
    // run hostname and print the result
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    fprintf(stderr, "hostname is %s\n", hostname);
    // run ifconfig and print the result
    FILE *fp = popen("ifconfig", "r");
    char buf[1024];
    while (fgets(buf, 1024, fp) != NULL) {
      fprintf(stderr, "%s", buf);
    }
    pclose(fp);
    //get the infiniband device name
    char *ibdev_name = getenv("IBV_DEVICE_NAME");
    fprintf(stderr, "IBV_DEVICE_NAME is %s\n", ibdev_name);
    //get the infiniband device port
    char *ibdev_port = getenv("IBV_PORT");
    fprintf(stderr, "IBV_PORT is %s\n", ibdev_port);
    //get the infiniband device gid index
    char *ibdev_gid_index = getenv("IBV_GID_INDEX");
    fprintf(stderr, "IBV_GID_INDEX is %s\n", ibdev_gid_index);
    //get the infiniband device gid
    char *ibdev_gid = getenv("IBV_GID");
    fprintf(stderr, "IBV_GID is %s\n", ibdev_gid);
    //get the infiniband device guid
    char *ibdev_guid = getenv("IBV_GUID");
    fprintf(stderr, "IBV_GUID is %s\n", ibdev_guid);
    //get the infiniband device ip
    char *ibdev_ip = getenv("IBV_IP");
    fprintf(stderr, "IBV_IP is %s\n", ibdev_ip);
    //get the infiniband device mac
    char *ibdev_mac = getenv("IBV_MAC");
    fprintf(stderr, "IBV_MAC is %s\n", ibdev_mac);
    //get the infiniband device mtu
    char *ibdev_mtu = getenv("IBV_MTU");
    fprintf(stderr, "IBV_MTU is %s\n", ibdev_mtu);
    //get the infiniband device lid
    char *ibdev_lid = getenv("IBV_LID");
    fprintf(stderr, "IBV_LID is %s\n", ibdev_lid);

    // print the infiniband device info
    ibv_device **dev_list = ibv_get_device_list(NULL);
    if (!dev_list) {
      fprintf(stderr, "Failed to get IB devices list\n");
    }
    for (int i = 0; dev_list[i]; ++i) {
      ibv_device *ib_dev = dev_list[i];
      fprintf(stderr, "IB device %d: %s\n", i, ibv_get_device_name(ib_dev));
      ibv_context *ctx = ibv_open_device(ib_dev);
      if (!ctx) {
        fprintf(stderr, "Failed to open device %s\n", ibv_get_device_name(ib_dev));
        continue;
      }
      ibv_device_attr device_attr;
      if (ibv_query_device(ctx, &device_attr)) {
        fprintf(stderr, "Failed to query device %s\n", ibv_get_device_name(ib_dev));
        continue;
      }
      fprintf(stderr, "max_cqe %d, max_qp_wr %d, max_sge %d, max_qp %d",
              device_attr.max_cqe, device_attr.max_qp_wr, device_attr.max_sge,
              device_attr.max_qp);
      ibv_free_device_list(dev_list);
      ibv_close_device(ctx);
    }

    // get memory info
    FILE *fp2 = popen("cat /proc/meminfo", "r");
    char buf2[1024];
    while (fgets(buf2, 1024, fp2) != NULL) {
      fprintf(stderr, "%s", buf2);
    }
    pclose(fp2);

    // get the usable memory info
    FILE *fp3 = popen("free -h", "r");
    char buf3[1024];
    while (fgets(buf3, 1024, fp3) != NULL) {
      fprintf(stderr, "%s", buf3);
    }
    pclose(fp3);
    exit(1);
  }
  
  wc_arr = new ibv_wc[max_cqe];
  ibv_qp_init_attr init_attr{
      NULL,
      cq,
      cq,
      NULL,
      {(uint32_t)max_send_wr, (uint32_t)max_recv_wr, 1, 1, 0},
      IBV_QPT_RC,
      1,
  };
  printf("max_cqe %d, max_send_wr %d, max_recv_wr %d\n", max_cqe,
               max_send_wr, max_recv_wr);
  qp = ibv_create_qp(pd, &init_attr);
  if (qp == nullptr) {
    fprintf(stderr, "create qp fail");
  }
}

RingBuffer::~RingBuffer() {
  if (mr)
    ibv_dereg_mr(mr);
  if (qp)
    ibv_destroy_qp(qp);
  if (cq)
    ibv_destroy_cq(cq);

  if (pd)
    ibv_dealloc_pd(pd);
  if (ctx)
    ibv_close_device(ctx);
  if (devices)
    ibv_free_device_list(devices);
  delete[] wc_arr;
}

uint32_t RingBuffer::generatePSN() {
  std::random_device rd;
  std::uniform_int_distribution<uint32_t> d(0, 0xffffff);
  return d(rd);
}

void RingBuffer::allocateMemory(size_t size, int access) {
  data = std::move(std::unique_ptr<uint8_t[]>(new uint8_t[size]));
  mr = ibv_reg_mr(pd, data.get(), size, access);
  if (!mr) {
    fprintf(stderr, "Failed to register mr: %s", strerror(errno));
    throw;
  }
  assert(size == mr->length);
}

void RingBuffer::modifyQP(unsigned qp_access_flags) {
  ibv_qp_attr qp_attr;

  // INIT
  qp_attr.qp_state = IBV_QPS_INIT;
  qp_attr.pkey_index = 0;
  qp_attr.port_num = 1;
  qp_attr.qp_access_flags = qp_access_flags;
  if (ibv_modify_qp(qp, &qp_attr,
                    IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT |
                        IBV_QP_ACCESS_FLAGS)) {
    fprintf(stderr, "Modify qp to Init failed: errno=%d, errmsg=\"%s\"", errno,
                  strerror(errno));
    throw;
  }

  // RTR
  qp_attr.qp_state = IBV_QPS_RTR;
  qp_attr.path_mtu = IBV_MTU_512;
  memset(&qp_attr.ah_attr, 0, sizeof(ibv_ah_attr));
  qp_attr.ah_attr.is_global = 0;
  qp_attr.ah_attr.dlid = remote_qp_info.lid;
  qp_attr.ah_attr.port_num = 1;
  qp_attr.dest_qp_num = remote_qp_info.qpn;
  qp_attr.rq_psn = remote_qp_info.psn;
  qp_attr.max_dest_rd_atomic = 1;
  qp_attr.min_rnr_timer = 12;


  if (use_RoCE_v2) { /* RoCE v2 switch it on */
    qp_attr.ah_attr.is_global = 1;
    if (this->remote_qp_info.gid.global.interface_id) {
      // Set attributes of the Global Routing Headers (GRH)
      // When using RoCE, GRH must be configured!
      qp_attr.ah_attr.grh.hop_limit = 1;
      qp_attr.ah_attr.grh.dgid = this->remote_qp_info.gid;
      qp_attr.ah_attr.grh.sgid_index = this->gid_index;
    }
  }

  if (ibv_modify_qp(qp, &qp_attr,
                    IBV_QP_STATE | IBV_QP_PATH_MTU | IBV_QP_AV |
                        IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                        IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER)) {
    fprintf(stderr, "Modify qp to RTR failed: errno=%d, errmsg=\"%s\"", errno,
                  strerror(errno));
    throw;
  }

  // RTS
  qp_attr.qp_state = IBV_QPS_RTS;
  qp_attr.timeout = 14;
  qp_attr.retry_cnt = 7;
  qp_attr.rnr_retry = 7;
  qp_attr.sq_psn = local_qp_info.psn;
  qp_attr.max_rd_atomic = 1;
  if (ibv_modify_qp(qp, &qp_attr,
                    IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                        IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN |
                        IBV_QP_MAX_QP_RD_ATOMIC)) {
    fprintf(stderr, "Modify qp to RTS failed: errno=%d, errmsg=\"%s\"", errno,
                  strerror(errno));
    throw;
  }
}

void RingBufferConsumer::exchangeQPInfo(const char *server_name, uint16_t port,
                                        const void *additional_write,
                                        size_t write_len, void *additional_read,
                                        size_t read_len) {
  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  int on = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
  sockaddr_in server_addr{AF_INET, htons(port), {inet_addr(server_name)}, {}};
  bind(sockfd, (sockaddr *)&server_addr, sizeof(server_addr));
  printf("listen to %s : %d\n", server_name, port);
  listen(sockfd, 10);
  sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(sockaddr_in);
  int fd = -1, retry_times = 10;
  while (fd < 0) {
    fd = accept(sockfd, (sockaddr *)&client_addr, &client_addr_len);
    if (fd < 0) {
      fprintf(stderr, "accept error %s", strerror(errno));
      if (--retry_times < 0) throw;
    }
  }
  ibv_port_attr port_attr;
  ibv_query_port(ctx, 1, &port_attr);
  
  this->local_qp_info.lid = port_attr.lid;
  this->local_qp_info.qpn = qp->qp_num;
  this->local_qp_info.psn = generatePSN();
  this->local_qp_info.addr = reinterpret_cast<uint64_t>(mr->addr);
  this->local_qp_info.rkey = mr->rkey;
  
  write_exact(fd, &local_qp_info, sizeof(QPInfo));

  read_exact(fd, &remote_qp_info, sizeof(QPInfo));

  if (additional_write && write_len) {
    write_exact(fd, additional_write, write_len);
  }
  if (additional_read && read_len) {
    read_exact(fd, additional_read, read_len);
  }

  this->sock_fd = fd;
  // close(fd);
  close(sockfd);
}
void RingBufferConsumer::close_connection() {
  uint8_t c = 42;
  write_exact(sock_fd, &c, sizeof(uint8_t));
  close(sock_fd);
}
void RingBufferProducer::exchangeQPInfo(const char *server_name, uint16_t port,
                                        const void *additional_write,
                                        size_t write_len, void *additional_read,
                                        size_t read_len) {
  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  sockaddr_in server_addr{AF_INET, htons(port), {inet_addr(server_name)}, {}};
  
  printf("connect to %s : %d\n", server_name, port);
  int retry_times = 10;
  while (connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    sleep(1);
    if ((--retry_times) < 0) {
      fprintf(stderr, "Connect consumer failed: errno=%d errmsg=\"%s\"", errno,
                    strerror(errno));
      close(sockfd);
      throw;
    }
  }

  ibv_port_attr port_attr;
  ibv_query_port(ctx, 1, &port_attr);
  this->local_qp_info.lid = port_attr.lid;
  this->local_qp_info.qpn = qp->qp_num;
  this->local_qp_info.psn = generatePSN();
  this->local_qp_info.addr = reinterpret_cast<uint64_t>(mr->addr);
  this->local_qp_info.rkey = mr->rkey;
  write_exact(sockfd, &local_qp_info, sizeof(QPInfo));

  read_exact(sockfd, &remote_qp_info, sizeof(QPInfo));

  if (additional_write && write_len) {
    write_exact(sockfd, additional_write, write_len);
  }
  if (additional_read && read_len) {
    read_exact(sockfd, additional_read, read_len);
  }
  this->sock_fd = sockfd;
  // close(sockfd);
}
void RingBufferProducer::close_connection() {
  uint8_t c;
  read_exact(this->sock_fd, &c, sizeof(uint8_t));
  close(sock_fd);
}
size_t write_exact(int fd, const void *buf, size_t count) {
  // current buffer loccation
  char *cur_buf = NULL;
  // # of bytes that have been written
  size_t bytes_wrt = 0;
  int n;

  if (!buf) {
    return 0;
  }

  cur_buf = (char *)buf;

  while (count > 0) {
    n = write(fd, cur_buf, count);

    if (n < 0) {
      fprintf(stderr, "write error\n");
      throw;

    } else {
      bytes_wrt += n;
      count -= n;
      cur_buf += n;
    }
  }

  return bytes_wrt;
}
size_t read_exact(int fd, void *buf, size_t count) {
  // current buffer loccation
  char *cur_buf = NULL;
  // # of bytes that have been read
  size_t bytes_read = 0;
  int n;

  if (!buf) {
    return 0;
  }

  cur_buf = (char *)buf;

  while (count > 0) {
    n = read(fd, cur_buf, count);

    if (n < 0) {
      fprintf(stderr, "read error\n");
      throw;

    } else {
      bytes_read += n;
      count -= n;
      cur_buf += n;
    }
  }

  return bytes_read;
}
