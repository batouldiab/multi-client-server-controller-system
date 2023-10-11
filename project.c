#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<time.h>
#include <sys/wait.h>

/*-----------------------------------------------supplimentary functions------------------------------------------*/

typedef struct request {
    int n;
    int port;
    int reply;
} request;

typedef struct node {
    request r;
    struct node *next;
} node;

typedef struct list {
    node *head, *tail;
    int size;
} list;

list create() {
    list l;
    l.head = l.tail = NULL;
    l.size = 0;
    return l;
}

void inserttail(list *l, request r) {
    node *n = (node *)malloc(sizeof(node));
    n->r = r;
    n->next = NULL;
    if (l->size == 0) {
        l->head = l->tail = n;
    } else {
        l->tail->next = n;
        l->tail = n;
    }
    l->size++;
}

request deletehead(list *l) {
    if (l->size == 0) {
        request r;
        r.n = r.port = r.reply = 0;
        return r;
    }
    node *n = l->head;
    request r = n->r;
    l->head = l->head->next;
    free(n);
    l->size--;
    return r;
}

long fact(int n) {
    long res = 1;
    for (int i = 1; i <= n; i++) {
        res *= i;
    }
    return res;
}

/*------------------------------------------------------variables----------------------------------------------*/

int nc = 1; // number of clients
int x = 0;  // counter
int srv;    // id of server
int onsrv = 0; // number of requests on server ( <10 )
int onwl = 0;  // number of request on waiting list in controller
list l, cl;    // list in server, waiting list in controller

// pipes
int clnctr[2];
int ctrsrv[2];
int clnsrv[2];
int timer[2];

/*----------------------------------------------Functions of clients---------------------------------------------*/

void sendrequest(int n, int port, int reply) {
    request r;
    r.n = n;
    r.port = port;
    r.reply = reply;
    write(clnctr[1], &r, sizeof(r));
    kill(getppid(), SIGUSR1);
}

void requestreceived(int id) {
    request r;
    read(clnctr[0], &r, sizeof(r));
    switch (r.reply) {
        case 0: // no place
            printf("**********\n\trequest:\t%d:%d\n\treply:\tno place\n**********\n", r.n, r.port);
            sendnewrequest();
            break;
        case -1: // waiting
            printf("**********\n\trequest:\t%d:%d\n\treply:\twaiting\n**********\n", r.n, r.port);
            signal(SIGUSR1, requestreceived); // if error message re-arrived
            pause();
            break;
        case -2: // error: time out
            printf("**********\n\trequest:\t%d:%d\n\treply:\ttime out\n**********\n", r.n, r.port);
            sendnewrequest();
            break;
        default: // received
            printf("**********\n\trequest:\t%d:%d\n\treply:\treceived\n**********\n", r.n, r.port);
            signal(SIGUSR1, requestreceived); // if error message re-arrived
            pause();
            break;
    }
}

void sendnewrequest() {
    int cont;
    srand(time(NULL));
    cont = rand() % 10; // generate number to determine if exit or continue sending
    if (cont < 6) {
        exit(0);
    } else {
        signal(SIGUSR2, resultmsg); // accepted from the server
        signal(SIGUSR1, requestreceived); // accepted from the controller
        request r;
        r.n = rand() % 10; // generate number to solve
        r.port = getpid(); // help determine pid of client sending request
        r.reply = -10; // default here since no value to pass
        while (1) { // insures it keeps sending the request until being received by controller (SIGUSR1 handled)
            sendrequest(r.n, r.port, r.reply);
            // request might be overwritten in pipe by another request so pause() won't help, we need to resend
        }
    }
}

void resultmsg(int id) {
    request r;
    read(clnsrv[0], &r, sizeof(r)); // number is request, port received is server id, reply is result
    kill(r.port, SIGUSR2); // to tell server pipe now is free
    printf("**********\n\trequest:\t%d:%d\n\tresult:\t%d\t\n**********\n", r.n, getpid(), r.reply);
    sendnewrequest();
}

/*---------------------------------------------Functions of controller-------------------------------------------*/

void getrequest(int id) {
    request r;
    read(clnctr[0], &r, sizeof(r));
    if (onwl == 5) { // no place
        r.reply = 0;
        write(clnctr[1], &r, sizeof(r));
        kill(r.port, SIGUSR1);
    } else {
        if (onsrv == 10) { // to put it on waiting list
            r.reply = -1;
            inserttail(&l, r);
            onwl++;
            write(clnctr[1], &r, sizeof(r));
            kill(r.port, SIGUSR1);
        } else { // received
            r.reply = 1;
            write(clnctr[1], &r, sizeof(r));
            kill(r.port, SIGUSR1);
            write(ctrsrv[1], &r, sizeof(r));
            kill(srv, SIGUSR1);
            onsrv++;
            signal(SIGUSR2, done);
            r.reply = -3;
            write(timer[1], &r, sizeof(r));
            signal(SIGALRM, timeout);
            alarm(5);
            pause();
        }
    }
}

void timeout(int id) {
    request r;
    read(timer[0], &r, sizeof(r));
    if (r.reply == -3) { // no done is processed yet
        r.reply = -2;
        write(timer[1], &r, sizeof(r));
        write(clnctr[1], &r, sizeof(r));
        kill(r.port, SIGUSR1);
    }
}

void done(int id) {
    request r;
    read(timer[0], &r, sizeof(r));
    if (r.reply == -3) { // no done is processed yet
        r.reply = -4;
        write(timer[1], &r, sizeof(r)); // tell alarm it is already done
    }
}

/*-----------------------------------------------Functions of server---------------------------------------------*/

void solve1(int id) {
    request r;
    int idclient;
    long nbr = 0;
    read(ctrsrv[0], &r, sizeof(r));
    int i = 1;
    while (i <= r.n) {
        nbr += fact(i);
        i++;
    }
    idclient = r.port;
    r.port = getpid(); // to help client get pid of server to send SIGUSR2 signal
    r.reply = nbr;
    write(clnsrv[1], &r, sizeof(r));
    kill(idclient, SIGUSR2); // tell that result is sent
    kill(getppid(), SIGUSR2); // tell controller it is done
}

void solve(int id) {
    request r;
    read(ctrsrv[0], &r, sizeof(r));
    inserttail(&l, r); // add new request to the list in server ready to solve
    signal(SIGUSR1, solve);
}

void solveeqn(int id) {
    long nbr = 0;
    request r = deletehead(&l);
    int i = 1;
    while (i <= r.n) {
        nbr += fact(i);
        i++;
    }
    r.port = getpid(); // to help client get pid of server to send SIGUSR2 signal
    r.reply = nbr;
    write(clnsrv[1], &r, sizeof(r));
    kill(r.port, SIGUSR2); // tell that result is sent
    kill(getppid(), SIGUSR2); // tell controller it is done
}

/*----------------------------------------------------- MAIN ----------------------------------------------------*/

int main() {
    pipe(clnctr), pipe(clnsrv), pipe(ctrsrv);

    do {
        printf("Enter the number of clients to create:\t");
        scanf("%d", &nc);
    } while (nc < 1 || nc > 10);

    for (int i = 0; i < nc; i++) {
        if (fork() == 0) {
            // Client process
            srand(time(NULL));
            request r;
            r.n = rand() % 10; // Generate number to solve
            r.port = getpid(); // Determine pid of client sending request
            r.reply = -10; // Default value
            while (1) {
                sendrequest(r.n, r.port, r.reply);
            }
        }
    }

    if ((srv = fork()) == 0) {
        // Server process
        sleep(5); // Wait until clients are prepared
        signal(SIGUSR1, solve1);
        pause();
        while (1) {
            signal(SIGUSR2, solveeqn);
            signal(SIGUSR1, solve);
        }
    } else {
        // Controller process
        while (x < nc) {
            x++;
            signal(SIGUSR2, done);
            signal(SIGUSR1, getrequest);
            pause();
        }
    }

    return 0;
}
