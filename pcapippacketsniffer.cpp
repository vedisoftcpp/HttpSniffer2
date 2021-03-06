#include "pcapippacketsniffer.h"
#include "ippacket.h"
#include "types.h"
#include "utils.h"
#include <stdio.h>

namespace HttpSniffer
{

PcapIpPacketSniffer::PcapIpPacketSniffer()
{
}

void PcapIpPacketSniffer::init()
{
    pcap_if_t *alldevs;
    pcap_if_t *d;
    int inum;
    int i=0;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_int netmask;
    char packet_filter[] = "ip";
    struct bpf_program fcode;

    /* Retrieve the device list */
    if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
    {
        fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
        exit(1);
    }

    /* Print the list */
    for(d=alldevs; d; d=d->next)
    {
        printf("%d. %s", ++i, d->name);
        if (d->description)
            printf(" (%s)\n", d->description);
        else
            printf(" (No description available)\n");
    }

    if(i==0)
    {
        printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
        return;
    }

    printf("Enter the interface number (1-%d):",i);
    scanf_s("%d", &inum);

    if(inum < 1 || inum > i)
    {
        printf("\nInterface number out of range.\n");
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return;
    }

    /* Jump to the selected adapter */
    for(d=alldevs, i=0; i< inum-1 ;d=d->next, i++);

    /* Open the adapter */
    if ( (_adhandle= pcap_open(d->name,  // name of the device
                              65536,     // portion of the packet to capture.
                              // 65536 grants that the whole packet will be captured on all the MACs.
                              PCAP_OPENFLAG_PROMISCUOUS,         // promiscuous mode
                              1000,      // read timeout
                              NULL,      // remote authentication
                              errbuf     // error buffer
                              ) ) == NULL)
    {
        fprintf(stderr,"\nUnable to open the adapter. %s is not supported by WinPcap\n");
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return;
    }

    /* Check the link layer. We support only Ethernet for simplicity. */
    if(pcap_datalink(_adhandle) != DLT_EN10MB)
    {
        fprintf(stderr,"\nThis program works only on Ethernet networks.\n");
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return;
    }

    if(d->addresses != NULL)
        /* Retrieve the mask of the first address of the interface */
        netmask=((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
    else
        /* If the interface is without addresses we suppose to be in a C class network */
        netmask=0xffffff;


    //compile the filter
    if (pcap_compile(_adhandle, &fcode, packet_filter, 1, netmask) <0 )
    {
        fprintf(stderr,"\nUnable to compile the packet filter. Check the syntax.\n");
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return;
    }

    //set the filter
    if (pcap_setfilter(_adhandle, &fcode)<0)
    {
        fprintf(stderr,"\nError setting the filter.\n");
        /* Free the device list */
        pcap_freealldevs(alldevs);
        return;
    }

    printf("\nlistening on %s...\n", d->description);

    /* At this point, we don't need any more the device list. Free it */
    pcap_freealldevs(alldevs);
}

IpPacket PcapIpPacketSniffer::get_ip_packet()
{
    int res;
    pcap_pkthdr* header;
    const u_char *pkt_data;
    while((res = pcap_next_ex(_adhandle, &header, &pkt_data)) >= 0)
    {
        if(res == 1)
            break;
    }
    ip_header_t *ih;
    ih = (ip_header_t *) (pkt_data + 14);
    return create_ip_packet_from_raw(ih);
}

}
