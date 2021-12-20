#include "consumer.h"

struct out_msg_st {
    long int my_msg_type;
    int pid;
    int bread_num;
};

struct in_msg_st {
    long int my_msg_type;
    char message[512];
};

struct out_msg_st outbuf;
struct in_msg_st inbuf;
int msgid;
sem_t* consumer_sem;
int exit_num;


void messageQueueInit(){
    if ((msgid = msgget((key_t)1895, IPC_CREAT|0666)) < 0) {
        perror("Could not get queue");
        exit(1);
    }
}
int getRandNum(int min, int max) {
    srand((unsigned)time(NULL));
    return (rand()%(max - min + 1) + min);
}

int get_message_num() {
    struct msqid_ds buf;
    msgctl(msgid, IPC_STAT, &buf);
    return buf.msg_qnum;
}

void buy_bread() {
    outbuf.my_msg_type = 1;
    int rand_num = getRandNum(1, 5);
    outbuf.bread_num = rand_num;
    outbuf.pid = getpid();
    printf("线程 %d 购买了 %d 个面包 \n", outbuf.pid, outbuf.bread_num);
    
    int waiting_num = get_message_num();
    if (waiting_num >= exit_num ) {
        printf("等待人数太多， 退出");
        exit(1);
    }

    sem_wait(consumer_sem);
    if (msgsnd(msgid, &outbuf, 8, 0) != 0) {
        perror("msg snd");
        exit(1);
    }
    printf("售货员工作，请等待\n");
    if (msgrcv(msgid, &inbuf, 512, getpid(), 0) == -1) {
        perror("msg rcv");
        exit(1);
    }
    printf("拿到货物 %s\n", inbuf.message);

    sem_post(consumer_sem);
}

void get_saler_thread_num() {
    int fd = shm_open(SALER_SHARED_NAME, O_RDONLY, 0);
    if (fd == -1) {
        perror("shm open error");
    }
    int value;
    read(fd, &value, sizeof(value));
    close(fd);
    exit_num = 3*value;
}

void init_sem() {
    consumer_sem = sem_open(CONSUMER_SEM_NAME, 0);
    if (consumer_sem == SEM_FAILED) {
        perror("consumer sem open error\n");
    }
}

int main(int ac, char* av[]){
    messageQueueInit();
    get_saler_thread_num();
    init_sem();
    
    if (ac == 2) {
        for (int i = 0;i < atoi(av[1]);i++) {
            buy_bread();
        }
    } else {
        buy_bread();
    }

    return 0;
}