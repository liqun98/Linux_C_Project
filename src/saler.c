#include <saler.h>

struct in_msg_st {
    long int my_msg_type;
    int pid;
    int bread_num;
};

struct out_msg_st {
    long int my_msg_type;
    char message[512];
};

int msgid;
int saler_log;
int th_num;
struct in_msg_st inbuf;
struct out_msg_st outbuf;
time_t cur;
pthread_mutex_t lock;
sem_t* consumer_sem, * full_sem, * empty_sem, * mutex_sem, * reproduce_sem;

int getRandNum(int min, int max) {
    srand((unsigned)time(NULL));
    return (rand()%(max - min + 1) + min);
}

void * do_sale(void * args) {
    while (1) {
        if (msgrcv(msgid, &inbuf, sizeof(int)*2, 1, 0) == -1) {
            perror("msg rcv error");
        }   
        int num = inbuf.bread_num;
        int cur_bread;
        sem_getvalue(full_sem, &cur_bread);
        if (cur_bread < 2 || cur_bread < num) {
            sem_post(reproduce_sem);
        }
        outbuf.my_msg_type = inbuf.pid;
        while (num != 0) {

            sem_wait(full_sem);
            sem_wait(mutex_sem);


            char buf[1024];
            time(&cur);
            int productId;
            sem_getvalue(full_sem, &productId);
            sprintf(buf, "售货员线程号: %d 时间: %s 产品编号: %d \n", (int)pthread_self(), asctime(localtime(&cur)), productId+1);
            write(saler_log, buf, strlen(buf));

            sem_post(mutex_sem);
            sem_post(empty_sem);
            
            num--;
        }

        pthread_mutex_lock(&lock);
        char buf[1024];
        time(&cur);
        sprintf(buf, "售货员线程号: %d 时间: %s 卖面包数量: %d \n", (int)pthread_self(), asctime(localtime(&cur)), inbuf.bread_num);
        write(saler_log, buf, strlen(buf));
        pthread_mutex_unlock(&lock);
        strcpy(outbuf.message, "success");
        if (msgsnd(msgid, &outbuf, 512, IPC_NOWAIT) == -1) {
            perror("msg snd error");
        }
    }
}

void pthreadCreate(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_create(&tid[i], NULL, do_sale, NULL);
    }
}

void pthreadJoin(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_join(tid[i], NULL);
    }
}

void init_sem() {
    full_sem = sem_open(FULL_SEM_NAME, 0);
    if (full_sem == SEM_FAILED) {
        perror("full sem open error\n");
    }

    empty_sem = sem_open(EMPTY_SEM_NAME, 0);
    if (empty_sem == SEM_FAILED) {
        perror("empty sem open error\n");
    }

    mutex_sem = sem_open(MUTEX_SEM_NAME, 0);
    if (mutex_sem == SEM_FAILED) {
        perror("mutex sem open error\n");
    }

    reproduce_sem = sem_open(REPRODUCE_SEM_NAME, 0);
    if (reproduce_sem == SEM_FAILED) {
        perror("reproduce sem open error\n");
    }
    
    printf("%d 线程数量\n", th_num);
    consumer_sem = sem_open(CONSUMER_SEM_NAME, O_CREAT, 0666, th_num);
    if (consumer_sem == SEM_FAILED) {
        perror("consumer sem open error\n");
    }
}

void init_msgqueue() {
    if ((msgid = msgget((key_t)1895, IPC_CREAT|0666)) < 0){
        perror("Could not create queue");
    }
}

void init_file_open() {
    saler_log = open("../log/saler.log", O_RDWR|O_APPEND);
}

void share_saler_thread_num() {
    int fd = shm_open(SALER_SHARED_NAME, O_CREAT|O_RDWR, 0666);
    void * addr;
    if (fd < 0) {
        perror("shm open error");
        exit(1);
    }
    if (ftruncate(fd, sizeof(int)) == -1) {
        perror("ftruncate error");
    }
    write(fd, &th_num, sizeof(th_num));
    close(fd);
}

int main(int ac, char *av[]) {
    th_num = getRandNum(2, 10);
    init_sem();
    init_msgqueue();
    init_file_open();
    pthread_mutex_init(&lock, NULL);
    share_saler_thread_num();

    pthread_t tid[th_num];
    pthreadCreate(tid, th_num);
    pthreadJoin(tid, th_num);

    sem_close(full_sem);
    sem_close(empty_sem);
    sem_close(mutex_sem);
    sem_close(reproduce_sem);
    sem_close(consumer_sem);

    msgctl(msgid, IPC_RMID, NULL);
    
    return 0;
}