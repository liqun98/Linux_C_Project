#include "producer.h"


int producer_conf, producer_log;
int pthreadNum;
time_t cur;
sem_t * full_sem, * empty_sem, * mutex_sem, * reproduce_sem;


void * do_produce(void * args) {
    int curStorage;
    while (1) {
        sem_getvalue(full_sem, &curStorage);
        curStorage += 1;
        if (curStorage > 200) break;
        
        sem_wait(empty_sem);
        sem_wait(mutex_sem);

        char buf[1024];
        time(&cur);
        sprintf(buf, "面包师线程号: %d 时间: %s 产品编号: %d \n", (int)pthread_self(), asctime(localtime(&cur)), curStorage);
        write(producer_log, buf, strlen(buf));

        sem_post(mutex_sem);
        sem_post(full_sem);

    }
}

int readProducerConf() {
    char buf[1024];
    int len = pread(producer_conf, buf, 2, 0);
    if (len == -1) {
        printf("error");
    }
    return atoi(buf);
}

void pthreadCreate(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_create(&tid[i], NULL, do_produce, NULL);
    }
}

void pthreadJoin(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_join(tid[i], NULL);
    }
}

int init_daemon() {
    int maxfd, fd;

    signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);

    switch(fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(0);
    }
    
    // 子进程成为进程组首进程
    if (setsid() == -1) return -1;


	//保证子进程不是进程组长，同时让该进程无法再打开一个新的终端
    switch(fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(0);
    }
    
    umask(0);

    maxfd = sysconf(_SC_OPEN_MAX);

    for (fd = 0;fd < maxfd;fd++)close(fd);
    syslog(LOG_INFO, "面包师开始生产");

    return 0;
}

void init_sem() {
    full_sem = sem_open(FULL_SEM_NAME, O_CREAT, 0666, 0);
    if (full_sem == SEM_FAILED) {
        perror("full sem open error\n");
    }

    empty_sem = sem_open(EMPTY_SEM_NAME, O_CREAT, 0666, 200);
    if (empty_sem == SEM_FAILED) {
        perror("empty sem open error\n");
    }
    
    mutex_sem = sem_open(MUTEX_SEM_NAME, O_CREAT, 0666, 1);
    if (mutex_sem == SEM_FAILED) {
        perror("mutex sem open error\n");
    }

    reproduce_sem = sem_open(REPRODUCE_SEM_NAME, O_CREAT, 0666, 1);
    if (reproduce_sem == SEM_FAILED) {
        perror("reproduce sem open error\n");
    }
}

void init_file_open() {
    producer_conf = open("../config/producer.conf", O_RDONLY);
    if (producer_conf == -1) {
        perror("producer conf error");
    }
    producer_log = open("../log/producer.log", O_RDWR|O_APPEND);
    if (producer_log == -1) {
        perror("producer log error");
    }
}

int main(int ac, char* av[]) {
    init_daemon();
    init_sem();
    init_file_open();

    while (1)
    {
        int curPthreadNum = readProducerConf();
        pthread_t tid[curPthreadNum];

        if (curPthreadNum != pthreadNum) {
            pthreadNum = curPthreadNum;
            syslog(LOG_NOTICE, "线程数量改变为 %d", pthreadNum);
        }
        sem_wait(reproduce_sem);
        pthreadCreate(tid, curPthreadNum);
        pthreadJoin(tid, curPthreadNum);
    }

    sem_close(full_sem);
    sem_close(empty_sem);
    sem_close(mutex_sem);
    sem_close(reproduce_sem);

    return 0;
}