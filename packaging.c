typedef struct _udp_hdr
{
 unsigned short SourPort;    　　// 源端口号
 unsigned short DestPort;    　　// 目的端口号
 unsigned short Length;    　　　// 数据包长度
 unsigned short CheckSum;    　 // 校验和
}UDP_HDR, *PUDP_HDR;

typedef struct _ip_hdr 
{
 char VersionAndHeaderLen;     　//版本信息(前4位)，头长度(后4位)
 char TypeOfService;      　　　　//服务类型8位
 short TotalLenOfPacket;    　　　//数据包长度
 short PacketID;      　　　　　　 //数据包标识
 short Sliceinfo;      　　　　　　//分片使用
 char TTL;        　　　　　　　　 //存活时间
 char TypeOfProtocol;    　　　　 //协议类型
 short CheckSum;      　　　　　　 //校验和
 unsigned int SourIp;     　　　　//源ip
 unsigned int DestIp;     　　　　//目的ip
} IP_HDR, *PIP_HDR;

typedef struct _llc_hdr
{
	char type;                   //类型信息
	unsigned short num；         //包序号
    char frag_num;               //分片号
}LLC_HDR,*PLLC_HDR;

for(i=0;i<qconf->n_rx_port;i++)
{
	portid=qconf->n_rx_port_list[i];
	nb_rx=rte_eth_rx_burst((unit8_t)portid,0,pkts_burst,MAX_PKT_BURST);
	port_statistics[portid].rx+=nb_rx;
	for(j=0;j<nb_rx;j++)
	{
		m=pkts_burst[j];
		unpackaging(m);
		rte_prefetch0(rte_pktmbuf_mthod(m,void *));
		packaging_forward(m,portid);
	}
}

static void packaging_forward(struct rte_mbuf *m,unsigned portid)
{
	unsigned dst_port;
	int sent;
	struct rte_eth_dev_tx_buffer *buffer;
	dst_port=dst_ports[portid];
	PUDP_HDR udphdr;
	PIP_HDR iphdr;
	PLLC_HDR llchdr;
	//填充UDP/IP/LLC头部
	m->pkt.data=rte_pktmbuf_prepend(m,sizeof(LLC_HDR));
    llchdr=(PLLC_HDR)m->pkt.data;
    //设置LLC头部

    m->pkt.data=rte_pktmbuf_prepend(m,sizeof(UDP_HDR));
    udphdr=(PUDP_HDR)m->pkt.data;
    //设置UDP头部

    m->pkt.data=rte_pktmbuf_prepend(m,sizeof(IP_HDR));
    iphdr=(PIP_HDR)m->pkt.data;
    //设置IP头部

    buffer=tx_buffer[dst_port];
    sent=rte_eth_dev_tx_buffer(dst_port,0,buffer,m);
    if(sent)
    	port_statistics[dst_port].tx+=sent;
}

static void unpackaging(struct rte_mbuf *m)
{
    PUDP_HDR udphdr;
	PIP_HDR iphdr;
	PLLC_HDR llchdr;

	iphdr=(PIP_HDR)m->pkt.data;
	//处理IP头部
	m->pkt.data=rte_pktmbuf_adj(m,sizeof(IP_HDR));

	udphdr=(PUDP_HDR)m->pkt.data;
	//处理UDP头部
	m->pkt.data=rte_pktmbuf_adj(m,sizeof(UDP_HDR));

	llchdr=(PLLC_HDR)m->pkt.data;
	//处理LLC头部
	m->pkt.data=rte_pktmbuf_adj(m,sizeof(LLC_HDR));
}