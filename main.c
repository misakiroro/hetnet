#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include "network.h"

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#define NB_MBUF   8192
#define MEMPOOL_CACHE_SIZE 256
#define MAX_PKT_BURST 32

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16

#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512

#define SET_TSC                                  //定义的链路探测包发送周期
#define NET_ID_MAX                               //最大线路ID数
#define MAX_TIME                                 //链路探测包往返最大计算时延

static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

static unsigned int rx_queue_per_lcore = 1;

static volatile bool force_quit = false;

static struct ether_addr _ports_eth_addr[RTE_MAX_ETHPORTS];  

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static const struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 1, /**< CRC stripped by hardware */
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

struct rte_mempool * hetnet_pktmbuf_pool = NULL;

static int
pack_sending(struct rte_mbuf *m,unsigned int portid)
{
	struct rte_eth_dev_tx_buffer *buffer;
	buffer = tx_buffer[portid];
	struct rte_mbuf *pack;
	DATA_PACK data_pack;
	//fill data_pack
    if(business_data_packaging(pack,data_pack)==-1)
    {
    	printf("add data_pack to mbuf failed");
    	return -1;
    }
    
    PUDP_HEADER udphdr;
    //fill udp_header
    if(udp_packaging(pack,udphdr)==-1)
    {
    	printf("add udphdr to mbuf failed");
    	return -1;
    }

    PIP_HEADER iphdr;
    //fill ip_header
    if(ip_packaging(pack,iphdr)==-1)
    {
    	printf("add iphdr to mbuf failed");
    	return -1;
    }

    PMAC_HEADER machdr;
    PMAC_TAIL   mactail;
    //fill mac_header and mac_tail
    if(mac_packaging(pack,machdr,mactail)==-1)
    {
    	printf("add mac to mbuf failed");
    	return -1;
    }

    if(pack_send(pack,port_id)==-1)
    {
    	printf("send pack failed");
    	return -1;
    }
}

static void
data_pack_send_loop(void)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	struct rte_eth_dev_tx_buffer *buffer;
	unsigned lcore_id;
	struct lcore_queue_conf *qconf;
	unsigned dst_port;
    int count=0;

	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
		return;
	}
	for (i = 0; i < qconf->n_rx_port; i++) {

		portid = qconf->rx_port_list[i];
		RTE_LOG(INFO, L2FWD, " -- lcoreid=%u portid=%u\n", lcore_id,
			portid);

	}

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);
	while(!force_quit) {
		for (i = 0; i < qconf->n_rx_port; i++) {

			portid = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst((uint8_t) portid, 0,
						 pkts_burst, MAX_PKT_BURST);

			port_statistics[portid].rx += nb_rx;

			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				rte_prefetch0(rte_pktmbuf_mtod(m, void *));
                dst_port = transport_selection();
		        if(pack_sending(m,dst_port)==0)
		        	count++;
		        sendwindow[count]=m;
		    }
		}
	}
}

static int
data_pack_send_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	data_pack_send_loop();
	return 0;
}

static int
recv_pack_process(struct rte_mbuf*m,int portid)
{
	PSOCKET_ADDR sock;
    if(mac_unpackaging(m,sock)==-1)
    {
    	printf("unpackaing mac failed");
    	return -1;
    }
    //确认MAC头部信息

    if(ip_unpackaging(m,sock)==-1)
    {
    	printf("unpackaing iphdr failed");
    	return -1;
    }
    //确认IP头部信息

    if(udp_unpackaging(m,sock)==-1)
    {
    	printf("unpackaing udphdr failed");
    	return -1;
    }
    //确认UDP头部信息
    
    char type=pack_classfied(m);

    switch(type){
    	case Data_Package:bussiness_pack_process(m);  break;
    	    
    	case Data_And_Ack_Package:bussiness_and_ack_process(m); break;
    	                         
    	case Ack_Package:ack_pack_process(m); break;
    	                
    	case Reset_Package:reset_pack_process(m); break;
    	                  
        case Seek_Package:seek_pack_process(m,portid); break;

        case Reply_Package:reply_pack_process(m,portid); break;

        case Arp_Request_Package:arp_acque_process(m,portid); break;

        case Arp_Reply_Package:arp_reply_process(m); break;

        default:pritnf("Type out of range"); return -1;
    }

    return 0;
}

static void
pack_receive_loop(void)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	struct lcore_queue_conf *qconf;
	unsigned lcore_id;

	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
		return;
	}

	for (i = 0; i < qconf->n_rx_port; i++) {

		portid = qconf->rx_port_list[i];
		RTE_LOG(INFO, L2FWD, " -- lcoreid=%u portid=%u\n", lcore_id,
			portid);

	}

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	while(!force_quit) {
		for (i = 0; i < qconf->n_rx_port; i++) {

			portid = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst((uint8_t) portid, 0,
						 pkts_burst, MAX_PKT_BURST);

			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				rte_prefetch0(rte_pktmbuf_mtod(m, void *));
		        recv_pack_process(m,portid);

		    }
		}
	}
}

static int
pack_receive_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	pack_receive_loop();
	return 0;
}

static void
seek_pack_send_loop(void)
{
	unsigned short seq_num=0;
    while(!force_quit)
    {
    	cur_tsc = rte_rdtsc();
    	diff_tsc = cur_tsc - prev_tsc;
    	if(diff_tsc==SET_TSC)//定时发送链路探测包
    	{
    		for(int i=0;i<NET_ID_MAX;i++)
    		{
    		   struct rte_mbuf *m;

    		   SEEK_PACK seek_pack;
    		   seek_pack.Type=Seek_Package;
    		   seek_pack.Value=seq_num;
    		   seek_pack.Length=sizeof(Type)+sizeof(Value)+sizeof(unsigned int);
               
               seek_packaing(m,seek_pack);

               if(pack_sending(m,i)==0)
               {
               	   seq_num++;
               	   tran_status.seek_pack_state[i][seq_num]=true;
               	   tran_status.seek_pack_time[i][seq_num]=//获取当前时间;
               }
    		}
    		prev_tsc=cur_tsc;
    	}
    }
}

static int
seek_pack_send_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	seek_pack_send_loop();
	return 0;
}

static void
trans_line_check_loop(void)
{
	unsigned int count;
	while(!force_quit)
	{
		cur_tsc=rte_rdtsc();
		diff_tsc=cur_tsc-prev_tsc;
		if(diff_tsc==SET_TSC)
		{
			for(int i=0;i<NET_ID_MAX;i++)
			{
				count=0;
				for(int j=0;tran_status.seek_pack_state[i][j]!=false;j++)
					count++;
				for(int j=0;j<count-4;j++)
				{
					if(tran_status.reply_pack_state[i][j]==false)
					{
						//等待一定时延，判断接下来四个链路响应包状态
						if(tran_status.reply_pack_state[i][j]==false&&
						tran_status.reply_pack_state[i][j+1]==false&&
						tran_status.reply_pack_state[i][j+2]==false&&
						tran_status.reply_pack_state[i][j+3]==false&&
						tran_status.reply_pack_state[i][j+4]==false)
							tran_status.state[i]=false;
					}
				}
			}
		prev_tsc=cur_tsc;
		}
	}
}

static int
trans_line_check_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	trans_line_check_loop();
	return 0;
}

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

int
main(int argc, char **argv)
{
	int ret;
	uint8_t nb_ports;
	uint8_t nb_ports_available;
	uint8_t portid;
	unsigned lcore_id, rx_lcore_id;
	struct lcore_queue_conf *qconf;
	struct TRAN_STATUS tran_status;
	//struct rte_eth_dev_info dev_info;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	hetnet_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
		rte_socket_id());

	if (hetnet_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	nb_ports = rte_eth_dev_count();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	rx_lcore_id = 0;
	qconf = NULL;

	for (portid = 0; portid < nb_ports; portid++) {

		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
		       rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}

		if (qconf != &lcore_queue_conf[rx_lcore_id])
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];

		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;

		printf("Lcore %u: RX port %u\n", rx_lcore_id, (unsigned) portid);
	}

	nb_ports_available = nb_ports;

	for (portid = 0; portid < nb_ports; portid++) {

		printf("Initializing port %u... ", (unsigned) portid);
		fflush(stdout);
		ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
				  ret, (unsigned) portid);

		rte_eth_macaddr_get(portid,&_ports_eth_addr[portid]);

		fflush(stdout);
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     rte_eth_dev_socket_id(portid),
					     NULL,
					     hetnet_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, (unsigned) portid);

		fflush(stdout);
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),
				NULL);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				ret, (unsigned) portid);

		tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					(unsigned) portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
				rte_eth_tx_buffer_count_callback,
				&port_statistics[portid].dropped);
		if (ret < 0)
				rte_exit(EXIT_FAILURE, "Cannot set error callback for "
						"tx buffer on port %u\n", (unsigned) portid);

		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
				  ret, (unsigned) portid);

		printf("done: \n");

		//rte_eth_promiscuous_enable(portid);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
				(unsigned) portid,
				_ports_eth_addr[portid].addr_bytes[0],
				_ports_eth_addr[portid].addr_bytes[1],
				_ports_eth_addr[portid].addr_bytes[2],
				_ports_eth_addr[portid].addr_bytes[3],
				_ports_eth_addr[portid].addr_bytes[4],
				_ports_eth_addr[portid].addr_bytes[5]);

		memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		rte_exit(EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}

	//check_all_ports_link_status(nb_ports, l2fwd_enabled_port_mask);

	ret = 0;

	rte_eal_remote_launch(data_pack_send_launch_one_lcore, NULL, 1);
	rte_eal_remote_launch(pack_receive_launch_one_lcore, NULL, 2);
	rte_eal_remote_launch(seek_pack_send_launch_one_lcore, NULL, 3);
	rte_eal_remote_launch(trans_line_check_launch_one_lcore, NULL, 4);
	rte_eal_mp_wait_lcore();

	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}

	for (portid = 0; portid < nb_ports; portid++) {
		//if ((_enabled_port_mask & (1 << portid)) == 0)
		//	continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}

	printf("Bye...\n");

	return ret;
}