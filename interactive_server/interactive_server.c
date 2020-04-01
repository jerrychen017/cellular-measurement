#include "message.h"
#include "sp.h"
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char User[80];
static char Spread_name[80];

static char Private_group[MAX_GROUP_NAME];
static const char group[80] = "jerry-minqi-group";
static mailbox Mbox;

static int num_messages;
static int num_processes;
static int process_index;
static int num_sent = 0;
static int num_delivered = 0;
static bool finished[10];
static FILE *fd;
//  starting time of transfer
static struct timeval start_time;

#define MAX_MESSLEN 102400
#define MAX_MEMBERS 100

static void receive_messages();
static struct timeval diffTime(struct timeval left, struct timeval right);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Mcast usage: mcast <num_of_messages> <process_index> "
               "<number_of_processes>\n");
        exit(1);
    }

    num_messages = atoi(argv[1]);
    if (num_messages < 0)
    {
        perror("Mcast: invalid number of messages.\n");
        exit(1);
    }

    num_processes = atoi(argv[3]);
    if (num_processes < 0 || num_processes > 10)
    {
        perror("Mcast: invalid number of processes.\n");
        exit(1);
    }

    process_index = atoi(argv[2]);
    if (!(process_index >= 1 && process_index <= num_processes))
    {
        perror("Mcast: invalid number of processes or invalid process index.\n");
        exit(1);
    }

    sprintf(User, "process_%d", process_index);
    sprintf(Spread_name, "4803");

    int ret;
    int mver, miver, pver;
    sp_time test_timeout;

    test_timeout.sec = 5;
    test_timeout.usec = 0;

    if (!SP_version(&mver, &miver, &pver))
    {
        printf("main: Illegal variables passed to SP_version()\n");
        exit(1);
    }

    ret = SP_connect_timeout(Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout);
    if (ret != ACCEPT_SESSION)
    {
        SP_error(ret);
        exit(1);
    }

    ret = SP_join(Mbox, group);
    if (ret < 0)
    {
        SP_error(ret);
        exit(1);
    }

    // initialize finished array
    for (int i = 0; i < num_processes; i++)
    {
        finished[i] = false;
    }

    // initialize and open file descriptor
    char filename[7];
    filename[6] = '\0';
    sprintf(filename, "%d.out", process_index);
    fd = fopen(filename, "w");

    E_init();

    E_attach_fd(Mbox, READ_FD, receive_messages, 0, NULL, HIGH_PRIORITY);

    E_handle_events();
    return (0);
}

static void receive_messages()
{
    struct message mess;
    char sender[MAX_GROUP_NAME];
    char target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    membership_info memb_info;
    int num_groups;
    int service_type;
    int16 mess_type;
    int endian_mismatch;
    int i;
    int ret;

    service_type = 0;

    ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                     &mess_type, &endian_mismatch, sizeof(struct message), (char *)&mess);

    if (ret < 0)
    {
        if ((ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT))
        {
            service_type = DROP_RECV;
            printf("\n========Buffers or Groups too Short=======\n");
            ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups, target_groups,
                             &mess_type, &endian_mismatch, sizeof(struct message), (char *)&mess);
        }
    }

    struct message data_packet;
    if (Is_regular_mess(service_type))
    { /* receive message packet*/
        if (Is_agreed_mess(service_type))
        {
            switch (mess_type)
            {
            case TAG_DATA:
            {
                if (mess.process_index == process_index)
                {
                    num_delivered++; // index that all machines have delivered up to
                    fprintf(fd, "%2d, %8d, %8d\n", mess.process_index, mess.message_index,
                            mess.random_number);

                    if (num_delivered % SEND_SIZE == 0)
                    {
                        for (int i = 0; i < SEND_SIZE && num_sent < num_messages; i++)
                        {
                            data_packet.process_index = process_index;
                            data_packet.message_index = num_sent + 1;
                            data_packet.random_number = (rand() % 999999) + 1;
                            ret = SP_multicast(Mbox, AGREED_MESS, group, TAG_DATA, sizeof(struct message), (char *)&data_packet);
                            num_sent++;
                        }
                    }

                    if (num_delivered == num_messages)
                    {
                        data_packet.process_index = process_index;
                        ret = SP_multicast(Mbox, AGREED_MESS, group, TAG_END, sizeof(struct message), (char *)&data_packet);
                        break;
                    }
                }
                else
                { // receive messages from other machines
                    fprintf(fd, "%2d, %8d, %8d\n", mess.process_index, mess.message_index,
                            mess.random_number);
                }
                break;
            }
            case TAG_END:
            {
                finished[mess.process_index - 1] = true;
                bool all_finished = true;
                for (int i = 0; i < num_processes; i++)
                {
                    all_finished = all_finished && finished[i];
                }
                if (all_finished)
                {
                    // record ending time of transfer
                    struct timeval end_time;
                    gettimeofday(&end_time, NULL);
                    struct timeval diff_time = diffTime(end_time, start_time);
                    double seconds = diff_time.tv_sec + ((double)diff_time.tv_usec) / 1000000;
                    printf("Trasmission time is %.2f seconds\n", seconds);
                    exit(0);
                }
                break;
            }
            }
        }
        else
        {
            printf("Warning: didn't receive AGREED \n");
        }
    }
    else if (Is_membership_mess(service_type))
    { /* receive membership packet*/
        ret = SP_get_memb_info((char *)&mess, service_type, &memb_info);
        if (ret < 0)
        {
            printf("BUG: membership message does not have valid body\n");
            SP_error(ret);
            exit(1);
        }
        if (Is_reg_memb_mess(service_type))
        {
            printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
                   sender, num_groups, mess_type);
            for (i = 0; i < num_groups; i++)
                printf("\t%s\n", &target_groups[i][0]);

            if (num_groups == num_processes)
            {
                gettimeofday(&start_time, NULL);
                printf("Start sending data packets.\n");
                int num_to_send = num_messages;
                if (INIT_SEND_SIZE < num_to_send)
                {
                    num_to_send = INIT_SEND_SIZE;
                }

                for (int i = 0; i < num_to_send; i++)
                {
                    data_packet.process_index = process_index;
                    data_packet.message_index = num_sent + 1;
                    data_packet.random_number = (rand() % 999999) + 1;
                    ret = SP_multicast(Mbox, AGREED_MESS, group, TAG_DATA, sizeof(struct message), (char *)&data_packet);
                    if (ret < 0)
                    {
                        printf("multicast error\n");
                    }

                    num_sent++;
                }

                if (num_sent == num_messages)
                {
                    data_packet.process_index = process_index;
                    ret = SP_multicast(Mbox, AGREED_MESS, group, TAG_END, sizeof(struct message), (char *)&data_packet);
                }
            }
        }
    }
}

struct timeval diffTime(struct timeval left, struct timeval right)
{
    struct timeval diff;

    diff.tv_sec = left.tv_sec - right.tv_sec;
    diff.tv_usec = left.tv_usec - right.tv_usec;

    if (diff.tv_usec < 0)
    {
        diff.tv_usec += 1000000;
        diff.tv_sec--;
    }

    if (diff.tv_sec < 0)
    {
        printf("WARNING: diffTime has negative result, returning 0!\n");
        diff.tv_sec = diff.tv_usec = 0;
    }

    return diff;
}