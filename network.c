#include "network.h"

int udp_packaging(struct rte_mbuf *m,PSOCKET_ADDR sock)
{
	m->pkt.data=rte_pktmbuf_prepend(m,sizeof(sock->udphdr));
	if(m->pkt.data==NULL)
		return -1;
    sock->udphdr=(PUDP_HEADER)m->pkt.data;
    return 0;
}

int ip_packaging(struct rte_mbuf *m,PSOCKET_ADDR sock)
{
	m->pkt.data=rte_pktmbuf_prepend(m,sizeof(sock->iphdr));
	if(m->pkt.data==NULL)
		return -1;
    sock->iphdr=(PIP_HEADER)m->pkt.data;
    return 0;
}

int mac_packaging(struct rte_mbuf *m, PSOCKET_ADDR sock)
{
	m->pkt.data=rte_pktmbuf_prepend(m,sizeof(sock->machdr));
	if(m->pkt.data==NULL)
		return -1;
    sock->machdr=(PMAC_HEADER)m->pkt.data;
    char *m_tail;
    m_tail=rte_pktmbuf_append(m,sizeof(sock->mactail));
    if(m_tail==NULL)
    	return -1;
    sock->mactail=(PMAC_TAIL)m_tail;
    return 0;
}

int business_data_packaing(struct rte_mbuf *m,DATA_PACK data_pack)
{
	memcpy(m->pkt.data,&data_pack,sizeof(data_pack));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int business_and_ack_packaing(struct rte_mbuf *m,DATA_AND_ACK data_and_ack)
{
	memcpy(m->pkt.data,&data_and_ack,sizeof(data_and_ack));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int ack_packaing(struct rte_mbuf *m,ACK_PACK ack_pack)
{
	memcpy(m->pkt.data,&ack_pack,sizeof(ack_pack));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int reset_packaing(struct rte_mbuf *m,RESET_PACK reset_pack)
{
	memcpy(m->pkt.data,&reset_pack,sizeof(reset_pack));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int seek_packaing(struct rte_mbuf *m,SEEK_PACK seek_pack)
{
	memcpy(m->pkt.data,&seek_pack,sizeof(seek_pack));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int reply_packaing(struct rte_mbuf *m,REPLY_PACK reply_pack)
{
	memcpy(m->pkt.data,&reply_pack,sizeof(reply_pack));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int arp_packaing(struct rte_mbuf *m,ARP_PACK arp_pack)
{
	memcpy(m->pkt.data,&arp_pack,sizeof(arp_pack));
	if(m->pkt.data==NULL)
		return -1;
	return 0
}

int pack_send(struct rte_mbuf *m,int portid)
{
	struct rte_eth_dev_tx_buffer *buffer;
	if(rte_eth_tx_buffer(port, 0, buffer, m)==0)   //一次发送一个mbuf到buffer
    	   return -1;
    return 0;
}

int transport_selection()
{
    int n;
    double mindelay=0;
    for(int i=0;i<NET_ID_MAX;i++)
    {
        if(tran_status.state[i]==false)
        	continue;
        else
        {
        	if(tran_status.delay[i]<mindelay)
        	{
        		mindelay=tran_status.delay[i];
        		n=i;
        	}
        }
    }
    return n;
}

int pack_receive(struct rte_mbuf **m,int portid)
{
    if(rte_eth_rx_burst(portid,0,m,MAXSIZE)<=0)       //一次接收MAXSIZE的mbuf
    	return -1;
    return 0;
}

int mac_unpackaing(struct rte_mbuf *m,PSOCKET_ADDR sock)
{
	sock->machdr=(PMAC_HEADER)m->pkt.data;
	m->pkt.data=rte_pktmbuf_adj(m,sizeof(MAC_HEADER));
	if(m->pkt.data==NULL)
		return -1;
	if(rte_pktmbuf_trim(m,sizeof(MAC_TAIL))==0)
		return 0;
	else
		return -1;
}

int ip_unpackaing(struct rte_mbuf *m,PSOCKET_ADDR sock)
{
	sock->iphdr=(PIP_HEADER)m->pkt.data;
	m->pkt.data=rte_pktmbuf_adj(m,sizeof(IP_HEADER));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

int udp_unpackaing(struct rte_mbuf *m,PSOCKET_ADDR sock)
{
	sock->udphdr=(PUDP_HEADER)m->pkt.data;
	m->pkt.data=rte_pktmbuf_adj(m,sizeof(UDP_HEADER));
	if(m->pkt.data==NULL)
		return -1;
	return 0;
}

unsigned char pack_classfied(struct rte_mbuf *m)
{
	char *type;
    type=(char *)m->pkt.data;
    if(m->pkt.data==NULL)
    	return -1;
    return (*type);
}

int business_pack_process(struct rte_mbuf *m)
{
 	short *pack_num;
 	pack_num=(short *)m->pkt.data;
 	m->pkt.data=rte_pktmbuf_adj(m,sizeof(short));
 	if(m->pkt.data==NULL)
 	{
 		printf("remove pack_num from pack failed");
 		return -1;
 	}
 	if(*(pack_num)==NULL)
 	{
 		printf("read pack_num failed");
 		return -1;
 	}
 	char *frag_num;
 	frag_num=(char *)m->pkt.data;
 	m->pkt.data=rte_pktmbuf_adj(m,sizeof(char));
 	if(m->pkt.data==NULL)
 	{
 		printf("remove frag_num from pack failed");
 		return -1;
 	}
 	if(*(frag_num)!=NULL)
 	{
        frag[frag_num]=m;
 	}
    recvwindow[pack_num]=m;
    int check_num;
    for(int i=0;i<MAXSIZE;i++)
    {
        if(recvwindow[i]==NULL)
        {
        	check_num=i-1;
        	break;
        }
    }
    ACK_PACK ack_pack;
    ack_pack.Type=ACK;
    ack_pack.SeqNum=check_num;
    struct rte_mbuf *m;
    if(ack_packaing(m,ack_pack)==-1)
    {
    	printf("Create ack_pack failed");
    	return -1;
    }
    PSOCKET_ADDR sock;
    //添加UDP头部
    if(udp_packaging(m,sock)==-1)
    {
    	printf("Add udp header failed");
    	return -1;
    }
    
    //添加IP头部
    if(ip_packaging(m,sock)==-1)
    {
    	printf("Add ip header failed");
    	return -1;
    }
   
    //添加MAC头部
    
    //添加MAC尾部
    if(mac_packaging(m,sock)==-1)
    {
        printf("Add mac failed");
        return -1;
    }
    int port;
    port=transport_selection();
    if(pack_send(m,port)==-1)
    {
    	printf("Send package failed");
    	return -1;
    }
    printf("Send ack_pack success!\n");
    return 0;
}

int ack_pack_process(struct rte_mbuf *m)
{
    short *ack_num;
    ack_num=(short *)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(short));
    if(*ack_num==NULL)
    {
    	printf("Read ack_num failed");
    	return -1;
    }
    if(*ack_num<sendwindow[0]||*ack_num>sendwindow[MAXSIZE])
    {
    	printf("Ack confirm failed");
    	return -1;
    }
    int count=1;
    for(int i=0;i<MAXSIZE;i++)
    {
    	if(sendwindow[i]==*ack_num)
    		break;
    	count++;
    }
    //remove pack_num from send window
    for(int i=0;i<MAXSIZE-count;i++)
    	sendwindow[i]=sendwindow[i+count];
    int j=1;
    //fill new pack_num into send window
    for(int i=MAXSIZE-count;i<MAXSIZE;i++)
    {
    	sendwindow[i]=sendwindow[MAXSIZE]+j;
    	j++;
    }
    return 0;
}

int business_and_ack_process(struct rte_mbuf *m)
{
	if(bussiness_pack_process(m)==-1)
	{
		printf("process bussiness part failed");
		return -1;
	}
	if(ack_pack_process(m)==-1)
	{
		printf("process ack part failed");
		return -1;
	}
	return 0;
}

int reset_pack_process(struct rte_mbuf *m)
//处理复位包

int seek_pack_process(struct rte_mbuf *m,int portid)
{
    unsigned int *length;
    length=(unsigned int *)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(unsigned int));

    unsigned short *num;
    num=(unsigned short *)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(unsigned short));

    REPLY_PACK reply_pack;
    reply_pack.Type=REPLY;
    reply_pack.Value=*num;
    reply_pack.length=sizeof(Type)+sizeof(Value)+sizeof(unsigned int);
    
    struct rte_mbuf *m;
    if(reply_packaing(m,reply_pack)==-1)
    {
    	printf("Create reply_pack failed");
    	return -1;
    }

    PSOCKET_ADDR sock;
    //modified udp header
    if(udp_packaging(m,sock)==-1)
    {
    	printf("Add udp header failed");
    	return -1;
    }

    //modified ip header
    if(ip_packaging(m,sock)==-1)
    {
    	printf("Add ip header failed");
    	return -1;
    }
    
    //modified mac header
   
    //modified mac tailer;
    if(mac_packaging(m,sock)==-1)
    {
        printf("Add mac failed");
        return -1;
    }
    
    if(pack_send(m,portid)==-1)
    {
    	printf("Send reply_pack failed");
    	return -1;
    }
    printf("Send reply_pack success!\n");
    return 0;
}

int reply_pack_process(struct rte_mbuf *m,int portid)
{
    unsigned int *length;
    length=(unsigned int *)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(unsigned int));

    unsigned short *num;
    num=(unsigned short *)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(unsigned short));

    tran_status.reply_pack_time[portid][*num]=//记录当前系统时间
    tran_status.delay[portid]=new_delay_time(tran_status.send_pack_time[portid][*num],tran_status.reply_pack_time[portid][*num],portid);
    if(tran_status.state[portid]==false)
    	tran_status.state[portid]==true;
    tran_status.reply_pack_state[portid][*num]=true;
    for(int i=0;i<*num;i++)
    {
    	if(tran_status.reply_pack_state[portid][i]==false)
    		tran_status.reply_pack_state[portid][i]==true;
    }
    return 0;
}

double new_delay_time(double old_time,double new_time,int portid)
{
    if((new_time-old_time)<MAX_TIME)
        tran_status.delay[portid]=tran_status.delay[portid]*0.8+(new_time-old_time)*0.2;
    else
    	tran_status.delay[portid]=tran_status.delay[portid]*0.8+MAX_TIME*0.2;
    return tran_status.delay[portid];
}

int arp_acque_process(struct rte_mbuf *m,int portid)
{
    PARP_PACK arp_pack;
    arp_pack=(PARP_PACK)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(ARP_PACK));

    mac_addr//获取当前MAC地址mac_addr
    if(arp_pack->DestMacAddr[6]==mac_addr)
    {
    	struct rte_mbuf *n;
        ARP_PACK arp_reply_pack;
        //填充ARP响应包
        if(arp_packaing(n,arp_reply_pack)==-1)
        {
        	printf("Packaing arp_reply_pack failed!");
        	return -1
        }
        
        PSOCKET_ADDR sock;
        //填充MAC头部

        if(mac_packaging(n,sock)==-1)
        {
        	printf("Packaing mac_header failed!");
        	return -1;
        }

        if(pack_send(n,portid)==-1)
        {
        	printf("Send package failed!");
        	return -1;
        }
        return 0;
    }
}

int arp_reply_process(struct rte_mbuf *m)
{
	PARP_PACK arp_pack;
	arp_pack=(PARP_PACK)m->pkt.data;
    m->pkt.data=rte_pktmbuf_adj(m,sizeof(ARP_PACK));

    //更新ARP缓存表
}