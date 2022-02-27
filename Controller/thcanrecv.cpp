#include <QDebug>

// C Header
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "thcanrecv.h"

thCanRecv::thCanRecv()
{

}

void thCanRecv::run()
{


    int can_fd;

    struct sockaddr_can addr;
    struct can_frame can_recv;
    struct ifreq ifr;

    ssize_t recv_bytes=0;
    char data[13]={0,};

    const char *ifname = "can0";

    if ((can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) == -1) {
        perror("Error while opening socket");
        return;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(can_fd, SIOCGIFINDEX, &ifr);

    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    printf("%s at index %d\n", ifname, ifr.ifr_ifindex);
    if (bind(can_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Error in socket bind");
        return;
    }


    while(1)
    {
        recv_bytes = read(can_fd, &can_recv, sizeof(struct can_frame));
        if(recv_bytes)
        {
            int getCanID = (can_recv.can_id & CAN_EFF_FLAG) ? (can_recv.can_id & CAN_EFF_MASK) : (can_recv.can_id & CAN_SFF_MASK);

            data[0] = (unsigned char)((unsigned int)getCanID) >> 24;
            data[1] = (unsigned char)(getCanID & 0x00ff0000) >> 16;
            data[2] = (unsigned char)(getCanID & 0x0000ff00) >> 8;
            data[3] = (unsigned char)getCanID & 0x000000ff;
            data[4] = can_recv.can_dlc;
            data[5] = can_recv.data[0];
            data[6] = can_recv.data[1];
            data[7] = can_recv.data[2];
            data[8] = can_recv.data[3];
            data[9] = can_recv.data[4];
            data[10] = can_recv.data[5];
            data[11] = can_recv.data[6];
            data[12] = can_recv.data[7];



            switch(getCanID)
            {
            case 0x10 :
                qDebug() << "ID 0x10 - Data Received\n";
                break;
            case 0x20 :
                qDebug() << "ID 0x20 - Data Received\n";
                break;
            case 0x30 :
                qDebug() << "ID 0x30 - Data Received\n";
                break;
            case 0x40 :
                qDebug() << "ID 0x40 - Data Received\n";
                break;
            case 0x50 :
                qDebug() << "ID 0x50 - Data Received\n";
                break;
            case 0x60 :
                qDebug() << "ID 0x60 - Data Received\n";
                break;
            default :
                qDebug() << "unknown device detected\n";
                break;
            }
            //memset(data,0x31,sizeof(data));
            //emit sendCanData(data);
        }
        usleep(1000000);
    }

}
