//异构网络编程接口
//1.	帧结构
//1.1 MAC帧头结构体
typedef struct _mac_hdr{
unsigned char SourceMacAddr[6];               //6字节，源MAC地址
unsigned char DestMacAddr [6];                //6字节，目的MAC地址
unsigned short Type；                         //2字节，上一层的协议类型
}MAC_HEADER,*PMAC_HEADER;

typedef struct _mac_tail{
unsigned int Checksum;                       //4字节，数据帧尾校验和
}MAC_TAIL,*PMAC_TAIL;
 
//1.2 IP头结构体
typedef struct  _iphdr{
unsigned char Lenver;                        //1字节，4位首部长度+4位IP版本号
unsigned char Tos;                           //1字节，8位服务类型TOS
unsigned short Total_Len;                    //2字节，16位总长度
unsigned short Ident；                       //2字节，16位标识
unsigned short Frag_And_Flags;               //2字节，3位标志位+13位分片偏移
unsigned char TTL;                           //1字节，8位生存期
unsigned char Proto;                         //1字节，8位协议（TCP，UDP或其他）
unsigned short Checksum；                    //2字节，16位头部校验和
unsigned int SourceIP；                      //4字节，32位源IP地址
unsigned int DestIP；                        //4字节，32位目的IP地址
}IP_HEADER,*PIP_HEADER;

//1.3 UDP头结构体
typedef struct _udphdr{
unsigned short SourcePort;                    //2字节，16位源端口号
unsigned short DestPort;                      //2字节，16位目的端口号
unsigned short Length;                        //2字节，16位数据包长度
unsigned short Checksum；                     //2字节，16位校验和
}UDP_HEADER,*UDP_HEADER;

//1.4 TCP头结构体
typedef struct _tcphdr{
unsigned short SourcePort;                   //2字节，16位源端口号
unsigned short DestPort;                     //2字节，16位目的端口号
unsigned int SeqNum;                         //4字节，32位序列号
unsigned int AcknowNum；                     //4字节，32位确认号
unsigned short HeaderlenAndFlag;             //2字节，前4位：TCP头部长度，中4位：保留，后6位：标志位
unsigned short Windowsize；                  //2字节，16位窗口大小
unsigned short Checksum；                    //2字节，16位TCP校验和
unsigned short Urgent_Point;                 //2字节，16位紧急指针
}TCP_HEADER,*PTCP_HEADER;

//1.5 数据帧头结构体
typedef struct data_frame{
MAC_HEADER machdr;                          //14字节，MAC帧头部
IP_HEADER iphdr;                            //20字节，IP帧头部
UDP_HEADER udphdr;                          //8字节，UDP帧头部
LLC_HEADER llchdr;                          //4字节，LLC帧头部
Struct rte_mbuf data;                      //若干字节，业务数据
MAC_TAIL mactail;                           //4字节，MAC帧尾部
}DATA_FRAME;

//1.5.1业务数据包结构体
typedef struct data_pack{
unsigned char Type;                        //1字节，数据类
unsigned short DataSeqNum;                 //2字节，数据包序号
unsigned char FragNum；                    //1字节，数据包分片号
struct rte_mbuf data;                     //若干字节，IP数据包
}DATA_PACK;

//1.5.2 ACK包结构体
typedef struct _ack_pack{
unsigned char Type;                        //1字节，数据类型
unsigned short SeqNum;                     //2字节，ACK确认序号
}ACK_PACK；

//1.5.3业务数据包与ACK包结构体
typedef struct _data_and_ack{
unsigned char Type;                        //1字节，数据类型
unsigned short DataSeqNum;                 //2字节，数据包序号
unsigned char FragNum；                    //1字节，数据包分片号
struct rte_mbuf data;                      //若干字节，IP数据包
unsigned short AckSeqNum;                  //2字节，ACK确认序号
}DATA_AND_ACK;
          
//1.5.4复位包结构体
typedef struct _reset_pack{
unsigned char Type;                        //1字节，数据类型
unsigned char Parameter[3];                //3字节，重置的参数类型
}RESET_PACK;

//1.6 控制帧结构体
typedef struct link_control_frame{
MAC_HEADER machdr;                         //14字节，MAC帧头部
IP_HEADER iphdr;                           //20字节，IP帧头部
UDP_HEADER udphdr;                         //8字节，UDP帧头部
Struct rte_mbuf data;                      //若干字节，链路控制包
MAC_TAIL mactail;                          //4字节，MAC帧尾部
}LINK_CONTROL_FRAME;

//1.6.1链路探测包结构体
typedef struct _seek_pack{
unsigned char Type;                        //1字节，数据类型
unsigned int Length;                       //4字节，各部分长度
unsigned short Value;                      //2字节，链路探测包序号
}SEEK_PACK;

//1.6.2链路探测响应包结构体
typedef struct _reply_pack{
unsigned char Type;                        //1字节，数据类型
unsigned int Length;                       //4字节，各部分长度
unsigned short Value;                      //2字节，链路探测响应包序号
}REPLY_PACK;

//1.6.3 ARP包结构体
typedef struct _arp_pack{
unsigned short Hardware_Type;              //2字节，硬件类型
unsigned short Proto_Type;                 //2字节，协议类型
unsigned char Hardware_Length;             //1字节，硬件地址长度
unsigned char Proto_Length;                //1字节，协议地址长度
unsigned short Type;                       //2字节，ARP数据包类型
unsigned char SourceMacAddr[6];            //6字节，发送端MAC地址
unsigned int SourceIPAddr;                 //4字节，发送端IP地址
unsigned char DestMacAddr[6];              //6字节，目的端MAC地址
unsigned int DsetIPAddr;                   //4字节，目的端IP地址
}ARP_PACK;

//1.6.4 链路状态和时延结构体
typedef struct _transline_status{
bool state[NET_ID_MAX];
unsigned int delay[NET_ID_MAX];
bool seek_pack_state[NET_ID_MAX][SEEK_MAX_NUM];
bool reply_pack_state[NET_ID_MAX][SEEK_MAX_NUM];
unsigned int seek_pack_time[NET_ID_MAX][SEEK_MAX_NUM];
unsigned int reply_pack_time[NET_ID_MAX][SEEK_MAX_NUM];
}TRAN_STATUS;

static struct rte_mbuf *frag[MAXSIZE];            //建立用于分片数组段重组的数组
static struct rte_mbuf *sendwindow[MAXSIZE];      //发送端发送窗口
static struct rte_mbuf *recvwindow[MAXSIZE];      //接收端接收窗口

/*2.通用包处理函数接口
  2.1添加UDP头
 * @param m
 *   传入的要封装的mbuf
 * @param udphdr
 *   传入的对应mbuf的UDP头部
 * @return
 *   0 ：成功 
     -1：失败
 */
int udp_packaging(struct rte_mbuf *m,PUDP_HEADER udphdr);
   
/*2.2添加IP头
 * @param m
 *   传入的要封装的mbuf
 * @param iphdr
 *   传入的对应mbuf的IP头部
 * @return
 *   0 ：成功 
     -1：失败
 */
int ip_packaging(struct rte_mbuf *m,PIP_HEADER iphdr);

/*2.3添加MAC头
 *  @param m
 *   传入的要封装的mbuf
 * @param machdr
 *   传入的对应mbuf的MAC头部
 * @param mactail
 *   传入的对应mbuf的MAC尾部
 * @return
 *   0 ：成功 
     -1：失败
 */
int mac_packaging(struct rte_mbuf *m, PMAC_HEADER machdr,PMAC_TAIL mactail);

/*2.4构造数据帧函数
  2.4.1构造业务数据包函数
 * @param m
 *   传入的要构造的mbuf
 * @param data_pack
 *   传入的业务数据包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int business_data_packaing(struct rte_mbuf *m,DATA_PACK data_pack);

/*2.4.2构造业务数据与ACK包函数
 * @param m
 *   传入的要构造的mbuf
 * @param data_and_ack
 *   传入的业务数据与ACK包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int business_and_ack_packaing(struct rte_mbuf *m,DATA_AND_ACK data_and_ack);

/*2.4.3构造ACK包函数
 * @param m
 *   传入的要构造的mbuf
 * @param ack_pack
 *   传入的ACK包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int ack_packaing(struct rte_mbuf *m,ACK_PACK ack_pack);

/*2.4.4构造复位包函数
 * @param m
 *   传入的要构造的mbuf
 * @param reset_pack
 *   传入的复位包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int reset_packaing(struct rte_mbuf *m,RESET_PACK reset_pack);

/*2.5构造链路控制帧函数
  2.5.1构造链路探测包函数
 * @param m
 *   传入的要构造的mbuf
 * @param seek_pack
 *   传入的链路探测包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int seek_packaing(struct rte_mbuf *m,SEEK_PACK seek_pack);

/*2.5.2构造链路响应包函数
 * @param m
 *   传入的要构造的mbuf
 * @param reply_pack
 *   传入的链路响应包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int reply_packaing(struct rte_mbuf *m,REPLY_PACK reply_pack);

/*2.5.3构造ARP帧函数
 * @param m
 *   传入的要构造的mbuf
 * @param arp_pack
 *   传入的ARP包结构体
 * @return
 *   0 ：成功 
     -1：失败
 */
int arp_packaing(struct rte_mbuf *m,ARP_PACK arp_pack);

/*2.6MAC帧发送函数
 * @param m
 *   传入的要发送的mbuf
 * @param portid
 *   发送mbuf的端口号
 * @return
 *   0 ：成功 
     -1：失败
 */
int pack_send(struct rte_mbuf *m,int portid);
/*2.7策略选择函数
 * @return
 *  n:传输线路号
 */
 int transport_selection();
//3.接收包处理函数接口
/*3.1接收包函数
* @param m
 *   传入的接收包的mbuf数组
 * @portid
 *   接收包的端口
 * @return
 *   0 ：成功 
     -1：失败
 */
 int pack_receive(struct rte_mbuf **m,int portid);
/*3.2除去MAC头函数
 * @param m
 *   传入的要处理的mbuf
 * @param mac_hdr
 *   传入的存储MAC帧头信息的结构体指针
 * @param mac_tail
 *   传入的存储MAC帧尾信息的结构体指针
 * @return
 *   0 ：成功 
     -1：失败
 */
int mac_unpackaing(struct rte_mbuf *m,PMAC_HEADER mac_hdr,PMAC_TAIL mac_tail);

/*3.3除去IP头函数
 * @param m
 *   传入的要处理的mbuf
 * @param ip_hdr
 *   传入的存储IP帧头信息的结构体指针
 * @return
 *   0 ：成功 
     -1：失败
 */
int ip_unpackaing(struct rte_mbuf *m,PIP_HEADER ip_hdr);

/*3.4除去UDP头函数
 * @param m
 *   传入的要处理的mbuf
 * @param udp_hdr
 *   传入的存储UDP帧头信息的结构体指针
 * @return
 *    0 ：成功 
      -1：失败
 */
int udp_unpackaing(struct rte_mbuf *m,PUDP_HEADER udp_hdr);

/*3.5帧分类函数
 * @param m
 *   传入的要分类的mbuf
 * @return
 *    0： 成功
     -1:  错误
 */
int frame_classfied(struct rte_mbuf *m);

/*3.6接收数据帧处理函数
3.6.1接收业务数据包处理函数
 * @param m
 *   传入的要处理的业务数据包mbuf
 * @return
 *    0 ：处理成功 
     -1:  处理失败
 */
int business_pack_process(struct rte_mbuf *m);

/*3.6.2接收ACK包处理函数
 * @param m
 *   传入的要处理的ACK包mbuf
 * @return
 *    0 ：处理成功 
     -1:  处理失败
 */
int ack_pack_process(struct rte_mbuf *m);

/*3.6.3接收业务数据ACK包处理函数
 * @param m
 *   传入的要处理的业务数据和ACK包mbuf
 * @return
 *    0 ：处理成功 
     -1:  处理失败
 */
int business_and_ack_process(struct rte_mbuf *m);

/*3.6.4复位包处理函数
 * @param m
 *   传入的要处理的复位包mbuf
 * @return
 *    0 ：处理成功 
     -1:  处理失败
 */
int reset_pack_process(struct rte_mbuf *m);

/*3.7接收链路控制帧处理函数
3.7.1接收链路探测包处理函数
 * @param m
 *   传入的要处理的链路探测包mbuf
 * @return
 *   0 ：处理成功 
     -1:  处理失败
 */
int seek_pack_process(struct rte_mbuf *m);

/*3.7.2接收链路响应包处理函数
 * @param m
 *   传入的要处理的链路响应包mbuf
 * @return
 *   0 ：处理成功 
     -1:  处理失败
 */
int reply_pack_process(struct rte_mbuf *m);
/* @param old_time
 *   传入发送链路探测包时记录的时间
 * @param new_time
 *   传入收到链路响应包时记录的时间
 * @param portid
 *   收包的端口号
 * @return
 *   new_delay:算法算出的SRTT
 */
inline unsigned int new_delay_time(unsigned int old_time,unsigned int new_time,int portid)

/*3.7.3接收ARP请求包处理函数
 * @param m
 *   传入的要处理的ARP请求包mbuf
 * @return
 *   0 ：处理成功 
     -1:  处理失败
 */
int arp_acque_process(struct rte_mbuf *m);

/*3.7.4接收ARP响应包处理函数
 * @param m
 *   传入的要处理的ARP响应包mbuf
 * @return
 *   0 ：处理成功 
     -1:  处理失败
 */
int arp_reply_process(struct rte_mbuf *m);
