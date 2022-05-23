/*61902950 okubo ikumi*/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<limits.h>

struct buf_header {
    int blkno;  /*論理ブロック番号*/
    struct buf_header *hash_fp; /*ハッシュの順方向ポインタ*/
    struct buf_header *hash_bp; /*ハッシュの逆方向ポインタ*/
    unsigned int stat; /*バッファの状態*/
    struct buf_header *free_fp; /*フリーリストの順方向ポインタ*/
    struct buf_header *free_bp; /*フリーリストの逆方向ポインタ*/
    char *cache_data; /*キャッシュデータ領域へのポインタ*/
    int bufno;
};

void help_proc(int, char *[]);

void init_proc(int, char *[]);

void buf_proc(int, char *[]);

void hash_proc(int, char *[]);

void free_proc(int, char *[]);

void getblk_proc(int, char *[]);

void brelse_proc(int, char *[]);

void set_proc(int, char *[]);

void reset_proc(int, char *[]);

void quit_proc(int, char *[]);

void order();

struct command_table{
    char *cmd;
    void (*func)(int, char *[]);
} cmd_tbl[]={
    {"help", help_proc},
    {"init", init_proc},
    {"buf", buf_proc},
    {"hash", hash_proc},
    {"free", free_proc},
    {"getblk", getblk_proc},
    {"brelse", brelse_proc},
    {"set", set_proc},
    {"reset", reset_proc},
    {"quit", quit_proc},
    {NULL, NULL}
};

enum{
    MAXLEN = 256,
    WORD = 32
};

#define NHASH 4
#define STAT_LOCKED 0x00000001
#define STAT_VAlID 0x00000002
#define STAT_DWR 0x00000004
#define STAT_KRDWR 0x00000008
#define STAT_WAITED 0x00000010
#define STAT_OLD 0x00000020

int hash(int n){
    int m = n % NHASH;
    return m;
}

struct buf_header hash_head[NHASH];

struct buf_header free_head;

struct buf_header *hash_search(int blkno){
    int h;
    struct buf_header *p = malloc(sizeof(struct buf_header));
    h = hash(blkno);
    for(p= hash_head[h].hash_fp;p!=&hash_head[h];p=p->hash_fp){
        if(p->blkno==blkno){
            return p;
        }
    }
    return NULL;
}

struct buf_header *hash_search_bufno(int bufno){
    int h;
    struct buf_header *p = malloc(sizeof(struct buf_header));
    for(h=0;h<4;h++){
        p = hash_head[h].hash_fp;
        while(p != &hash_head[h]){
            if(p -> bufno == bufno){
                return p;
            }
            p = p->hash_fp;
        }
    }
    return NULL;
}

void insert_head(struct buf_header *h, struct buf_header *p){
    p->hash_bp = h;
    p->hash_fp = h->hash_fp;
    h->hash_fp->hash_bp = p;
    h->hash_fp = p;
}

void insert_tail(struct buf_header *h, struct buf_header *p){
    p->hash_fp = h;
    p->hash_bp = h->hash_bp;
    h->hash_bp->hash_fp = p;
    h->hash_bp = p;
}

void remove_from_hash(struct buf_header *p){
    p->hash_bp->hash_fp = p->hash_fp;
    p->hash_fp->hash_bp = p->hash_bp;
    p->hash_bp = p->hash_fp = NULL;
}

void remove_from_freelist(struct buf_header *p){
    p->free_bp->free_fp = p->free_fp;
    p->free_fp->free_bp = p->free_bp;
    p->free_bp = p->free_fp = NULL;
}

void insert_head_freelist(struct buf_header *h, struct buf_header *p){
    p->free_bp = h;
    p->free_fp = h->free_fp;
    h->free_fp->free_bp = p;
    h->free_fp = p;
}

void insert_tail_freelist(struct buf_header *h, struct buf_header *p){
    p->free_fp = h;
    p->free_bp = h->free_bp;
    h->free_bp->free_fp = p;
    h->free_bp = p;
}

struct buf_header *getblk(int blkno)
{
    struct buf_header *found = malloc(sizeof(struct buf_header)); 
    found = hash_search(blkno);
    struct buf_header *p2 = malloc(sizeof(struct buf_header));
    int h;
    while(1/*buffer not found*/){
        if(found!=NULL/*block in hash queue*/){
            struct buf_header *found = hash_search(blkno);
            if(hash_search(blkno)->stat&STAT_LOCKED/*buffer locked*/){
                /*シナリオ5*/
                printf("exe scenario 5\n");
                hash_search(blkno)->stat |= STAT_WAITED;
                printf("Process goes to sleep\n");  /*event buffer becomes free*/
                order();
                return NULL;
            }
            /*シナリオ1*/
            printf("exe scenario 1\n");
            found->stat |= STAT_LOCKED;   /*make buffer locked*/
            remove_from_freelist(found);  /*remove buffer form free list*/
            return found;                /*return pointer to buffer*/ 
        } else {
            if(free_head.free_fp==&free_head/*not buffer on free list*/){
                /*シナリオ4*/
                printf("exe scenario 4\n");
                printf("Process goes to sleep\n"); /*event any buffer becomes free*/
                order();
                return NULL;
            }
            found = free_head.free_fp;
            if(found->stat&STAT_DWR/*buffer maked for delayed write*/){
                /*シナリオ3*/
                printf("exe scenario 3\n");
                found->stat |= STAT_LOCKED;
                found->stat |= STAT_OLD;
                found->stat |= STAT_KRDWR;
                found->stat &= ~STAT_DWR;
                remove_from_freelist(found);
                found = free_head.free_fp;
                remove_from_freelist(found);
                remove_from_hash(found);
                found->blkno = blkno;
                found->stat |= STAT_LOCKED;
                found->stat &= ~STAT_VAlID;
                h = hash(blkno);
                insert_tail(&hash_head[h], found);
                /*asynchronous write buffer to disk*/
                order();
                return found;
            }
            remove_from_freelist(found); /*remove buffer from free list*/
            remove_from_hash(found);
            /*シナリオ2*/
            printf("exe scenario 2\n");
            found->blkno = blkno;
            found->stat |= STAT_LOCKED;
            found->stat &= ~STAT_VAlID;
            h = hash(blkno);
            insert_tail(&hash_head[h], found);
            order();
            return found;
        }
    }
}

void brelse(int blkno){
    struct buf_header *p = malloc(sizeof(struct buf_header));
    printf("Wakeup processes waiteing for any buffer\n");/*wakeup()*/
    /*event, waiting for any buffer to become free;*/
    printf("Wakeup processes waiteing for buffer of blkno %d\n", blkno);/*wakeup();*/
    /*event, wainting for any buffer to become free;*/
    //raise_cpu_level;
    p = hash_search(blkno);
    if ((p->stat&STAT_VAlID) && !(p->stat&STAT_OLD)/*buffer contents valid and buffer not old*/){
        insert_tail_freelist(&free_head ,p);
        printf("insert tail of free list\n");
    } else {
        insert_head_freelist(&free_head ,p);
        printf("insert head of free list\n");
        hash_search(blkno)->stat &= ~STAT_OLD;
    }
    //lower_cpu_level();
    hash_search(blkno)->stat &= ~STAT_WAITED;
    hash_search(blkno)->stat &= ~STAT_LOCKED;  /*unloced*/
}

void getargs(char *lbuf,int *argc, char *argv[]){
    int flag_moji = 0;
    int flag_end = 0;
    int j;
    for(j=0; j<256; j++)
    {
        if(lbuf[j] == '\n'){
            lbuf[j] = '\0';
            break;
        }
        if((lbuf[j] == ' ') && (flag_end == 0)){
            lbuf[j] = '\0';
            flag_end = 1;
            flag_moji = 0;
        } else {
            if(flag_moji==0){
                argv[*argc] = (char*)malloc(sizeof(char) * WORD);
                if(argv[*argc]==NULL){
                    exit(1);
                }
                argv[*argc] = &lbuf[j];
                (*argc)++;
                flag_moji = 1;
                flag_end = 0;
            }
        }
    }
}

void help_proc(int ac, char *av[]){
    printf("command               : how to use\n\n");
    printf("init                  : initialize hash list and free list\n");
    printf("buf [n...]            : print all buf stat or buf stat whose blkno is n\n");
    printf("hash [n...]           : print all hash list or hash list whose hash is n\n");
    printf("free                  : print free lis\n");
    printf("getblk n              : execute getblk(n)\n");
    printf("brelse n              : execute brelse such as pointer bp corresponding to blkno n\n");
    printf("set n stat [stat...]  : set stat of buf that blkno is n\n");
    printf("reset n stat [stat...]: set stat of buf that blkno is n\n");
    printf("quit                  : quit this software\n");
}   

void init_proc(int ac, char *av[]){
    //init hash_head[0~3] and free_head
    hash_head[0].hash_bp = hash_head[0].hash_fp = &hash_head[0];
    hash_head[1].hash_bp = hash_head[1].hash_fp = &hash_head[1];
    hash_head[2].hash_bp = hash_head[2].hash_fp = &hash_head[2];
    hash_head[3].hash_bp = hash_head[3].hash_fp = &hash_head[3];
    free_head.free_fp = free_head.free_bp = &free_head;

    //init hash_list
    struct buf_header *p_28 = malloc(sizeof(struct buf_header));
    p_28->blkno = 28;
    p_28->stat |= STAT_VAlID;
    p_28->stat &= ~STAT_OLD;
    p_28->stat &= ~STAT_KRDWR;
    p_28->stat &= ~STAT_DWR;
    p_28->stat &= ~STAT_WAITED;
    p_28->stat &= ~STAT_LOCKED;
    p_28->bufno = 0;
    insert_tail(&hash_head[0], p_28);
    struct buf_header *p_4 = malloc(sizeof(struct buf_header));
    p_4->blkno = 4;
    p_4->stat |= STAT_VAlID;
    p_4->bufno = 1;
    insert_tail(&hash_head[0], p_4);
    struct buf_header *p_64 = malloc(sizeof(struct buf_header));
    p_64->blkno = 64;
    p_64->stat |= STAT_VAlID;
    p_64->stat |= STAT_LOCKED;
    p_64->bufno = 2;
    insert_tail(&hash_head[0], p_64);
    struct buf_header *p_17 = malloc(sizeof(struct buf_header));
    p_17->blkno = 17;
    p_17->stat |= STAT_VAlID;
    p_17->stat |= STAT_LOCKED;
    p_17->bufno = 3;
    insert_tail(&hash_head[1], p_17);
    struct buf_header *p_5 = malloc(sizeof(struct buf_header));
    p_5->blkno = 5;
    p_5->stat |= STAT_VAlID;
    p_5->bufno = 4;
    insert_tail(&hash_head[1], p_5);
    struct buf_header *p_97 = malloc(sizeof(struct buf_header));
    p_97->blkno = 97;
    p_97->stat |= STAT_VAlID;
    p_97->bufno = 5;
    insert_tail(&hash_head[1], p_97);
    struct buf_header *p_98 = malloc(sizeof(struct buf_header));
    p_98->blkno = 98;
    p_98->stat |= STAT_VAlID;
    p_98->stat |= STAT_LOCKED;
    p_98->bufno = 6;
    insert_tail(&hash_head[2], p_98);
    struct buf_header *p_50 = malloc(sizeof(struct buf_header));
    p_50->blkno = 50;
    p_50->stat |= STAT_VAlID;
    p_50->stat |= STAT_LOCKED;
    p_50->bufno = 7;
    insert_tail(&hash_head[2], p_50);
    struct buf_header *p_10 = malloc(sizeof(struct buf_header));
    p_10->blkno = 10;
    p_10->stat |= STAT_VAlID;
    p_10->bufno = 8;
    insert_tail(&hash_head[2], p_10);
    struct buf_header *p_3 = malloc(sizeof(struct buf_header));
    p_3->blkno = 3;
    p_3->stat |= STAT_VAlID;
    p_3->bufno = 9;
    insert_tail(&hash_head[3], p_3);
    struct buf_header *p_35 = malloc(sizeof(struct buf_header));
    p_35->blkno = 35;
    p_35->stat |= STAT_VAlID;
    p_35->stat |= STAT_LOCKED;
    p_35->bufno = 10;
    insert_tail(&hash_head[3], p_35);
    struct buf_header *p_99 = malloc(sizeof(struct buf_header));
    p_99->blkno = 99;
    p_99->stat |= STAT_VAlID;
    p_99->stat |= STAT_LOCKED;
    p_99->bufno = 11;
    insert_tail(&hash_head[3], p_99);

    //init free_list
    insert_tail_freelist(&free_head, p_3);
    insert_tail_freelist(&free_head, p_5);
    insert_tail_freelist(&free_head, p_4);
    insert_tail_freelist(&free_head, p_28);
    insert_tail_freelist(&free_head, p_97);
    insert_tail_freelist(&free_head, p_10);

    
}

void print_stat_buf(struct buf_header *p){
    if(p->stat & STAT_OLD){
        printf("O");
    }else{
        printf("-");
    }
    if(p->stat & STAT_WAITED){
        printf("W");
    }else{
        printf("-");
    }
    if(p->stat & STAT_KRDWR){
        printf("K");
    }else{
        printf("-");
    }
    if(p->stat & STAT_DWR){
        printf("D");
    }else{
        printf("-");
    }
    if(p->stat & STAT_VAlID){
        printf("V");
    }else{
        printf("-");
    }
    if(p->stat & STAT_LOCKED){
        printf("L");
    }else{
        printf("-");
    }
}

void buf_proc(int ac, char *av[]){
    int i;
    int j=0;
    struct buf_header *p;
    if(ac==1){
        for(i=0;i<4;i++){
            p = hash_head[i].hash_fp;
            while(p != &hash_head[i]){
                printf("[ %2d: %2d ", j, p->blkno);
                if(p->stat & STAT_OLD){
                    printf("O");
                }else{
                    printf("-");
                }
                if(p->stat & STAT_WAITED){
                    printf("W");
                }else{
                    printf("-");
                }
                if(p->stat & STAT_KRDWR){
                    printf("K");
                }else{
                    printf("-");
                }
                if(p->stat & STAT_DWR){
                    printf("D");
                }else{
                    printf("-");
                }
                if(p->stat & STAT_VAlID){
                    printf("V");
                }else{
                    printf("-");
                }
                if(p->stat & STAT_LOCKED){
                    printf("L");
                }else{
                    printf("-");
                }
                printf("]\n");
                p = p->hash_fp;
                j++;
            }
        }
    } else {
        for (i=0;i<ac-1;i++){
            if(strcmp(av[i+1], "0") == 0){
                p = hash_head[0].hash_fp;
                printf("[ %2d: %2d ", p->bufno, p->blkno);
                print_stat_buf(p);
                printf("]");
                p = p->hash_fp;
                printf("\n");
            }else{
                int a = atoi(av[i+1]);
                if(a == 0){
                    fprintf(stderr,"buf [n...] : please input int argument\n");
                }else{
                    p = hash_search_bufno(a);
                    if(p==NULL){
                        printf("not existing bufno %d\n", a);
                    }else{
                        printf("[ %2d: %2d ", p->bufno, p->blkno);
                        print_stat_buf(p);
                        printf("]\n");
                    }
                }
            }
        }
    }
}

void hash_proc(int ac, char *av[]){
    int i;
    struct buf_header *p = malloc(sizeof(struct buf_header));
    if(ac==1){
        for(i=0;i<4;i++){
            printf("%d: ",i);
            p = hash_head[i].hash_fp;
            while(p != &hash_head[i]){
                printf("[ %2d: %2d ", p->bufno, p->blkno);
                print_stat_buf(p);
                p = p->hash_fp;
                printf("]");
            }
            printf("\n");
        }
    }else{
        for (i=0;i<ac-1;i++){
            if(strcmp(av[i+1], "0") == 0){
                p = hash_head[0].hash_fp;
                printf("0: ");
                while(p != &hash_head[0]){
                        printf("[ %2d: %2d ", p->bufno, p->blkno);
                        print_stat_buf(p);
                        printf("]");
                        p = p->hash_fp;
                }
                printf("\n");
            }else{
                int a = atoi(av[i+1]);
                if(a == 0){
                    printf("please input int argument\n");
                }else{
                    p = hash_head[a].hash_fp;
                    if(p==NULL){
                        printf("not exitsting hash no %d\n", a);
                    }else if(a >= 0 && a <= 3){
                        printf("%d: ", a);
                        while(p != &hash_head[a]){
                            printf("[ %2d: %2d ", p->bufno, p->blkno);
                            print_stat_buf(p);
                            printf("]");
                            p = p->hash_fp;
                        }
                        printf("\n");
                    }
                }   
            }
        }       
    }
    
}

void free_proc(int ac, char *av[]){
    struct buf_header *p = malloc(sizeof(struct buf_header));
    p = free_head.free_fp;
    while(p != &free_head){
        printf("[ %2d: %2d ", p->bufno, p->blkno);
        print_stat_buf(p);
        printf("]");
        p = p->free_fp;
    }
    printf("\n");
}

void getblk_proc(int ac, char *av[]){
    if(ac==2){
        if(strcmp(av[1], "0") == 0){
            getblk(0);
        }else{
            int a = atoi(av[1]);
            getblk(a);
        }
    }else if(ac==1){
        fprintf(stderr, "please input one argument\n");
    }else{
        fprintf(stderr,"only one argument is required\n");
    }
}

void brelse_proc(int ac, char *av[]){
    if(ac==2){
        if(strcmp(av[1], "0") == 0){
            brelse(0);
        }else{
            int a = atoi(av[1]);
            brelse(a);
        }
    }else if(ac==1){
        fprintf(stderr, "please input one argument\n");
    }else{
        fprintf(stderr,"only one argument is required\n");
    }
}

void set_proc(int ac, char *av[]){
    struct buf_header *p = malloc(sizeof(struct buf_header)); 
    if(ac<2){
        fprintf(stderr, "please input n(blkno) and stat(e.g. set n stat [stat...])\n");
    }else if(ac==2){
        fprintf(stderr, "please input stat(e.g. set n stat [stat...])\n");
    }else{
        int i;
        int n;
        if(strcmp(av[1],"0") == 0){
            n = 0;/*これ以降に、論理ブロック0用のコードを記述*/
            p = hash_search(0);
        }else{
            n = atoi(av[1]);
            if(n==0){
                printf("please input blkno(int) at second argument");
            }else{
                p = hash_search(n);
                if(p==NULL){
                    printf("not exitsting hash no %d\n", n);
                }else{
                    for(i=0;i<ac-2;i++){
                        if(strcmp(av[i+2], "L") == 0){
                            p->stat |= STAT_LOCKED;
                        }else if(strcmp(av[i+2], "V") == 0){
                            p->stat |= STAT_VAlID;
                        }else if(strcmp(av[i+2], "D") == 0){
                            p->stat |= STAT_DWR;
                        }else if(strcmp(av[i+2], "K") == 0){
                            p->stat |= STAT_KRDWR;
                        }else if(strcmp(av[i+2], "W") == 0){
                            p->stat |= STAT_WAITED;
                        }else if(strcmp(av[i+2], "O") == 0){
                            p->stat |= STAT_OLD;
                        }else{
                            printf("please input status(L V D K W O)");
                        }
                    }
                }
            }
        }
        for(i=0;i<ac-1;i++){
            if(strcmp(av[i+1], "0") == 0){
                n = 0;/*これ以降に、論理ブロック0用のコードを記述*/
            }else{
                n = atoi(av[i+1]);
                p = hash_search(n);
            }
        }
        
    }
}

void reset_proc(int ac, char *av[]){
    struct buf_header *p = malloc(sizeof(struct buf_header)); 
    if(ac<2){
        fprintf(stderr, "please input n(blkno) and stat(e.g. set n stat [stat...])\n");
    }else if(ac==2){
        fprintf(stderr, "please input stat(e.g. set n stat [stat...])\n");
    }else{
        int i;
        int n;
        if(strcmp(av[1],"0") == 0){
            n = 0;/*これ以降に、論理ブロック0用のコードを記述*/
            p = hash_search(0);
        }else{
            n = atoi(av[1]);
            if(n==0){
                printf("please input blkno(int) at second argument");
            }else{
                p = hash_search(n);
                if(p==NULL){
                    printf("not exitsting hash no %d\n", n);
                }else{
                    for(i=0;i<ac-2;i++){
                        if(strcmp(av[i+2], "L") == 0){
                            p->stat &= ~STAT_LOCKED;
                        }else if(strcmp(av[i+2], "V") == 0){
                            p->stat &= ~STAT_VAlID;
                        }else if(strcmp(av[i+2], "D") == 0){
                            p->stat &= ~STAT_DWR;
                        }else if(strcmp(av[i+2], "K") == 0){
                            p->stat &= ~STAT_KRDWR;
                        }else if(strcmp(av[i+2], "W") == 0){
                            p->stat &= ~STAT_WAITED;
                        }else if(strcmp(av[i+2], "O") == 0){
                            p->stat &= ~STAT_OLD;
                        }else{
                            printf("please input status(L V D K W O)");
                        }
                    }
                }
            }
        }
        for(i=0;i<ac-1;i++){
            if(strcmp(av[i+1], "0") == 0){
                n = 0;/*これ以降に、論理ブロック0用のコードを記述*/
            }else{
                n = atoi(av[i+1]);
                p = hash_search(n);
            }
        }
        
    }
}

void quit_proc(int ac, char *av[]){
    exit(0);
}

void order(){
    int count=0;
    int i;
    struct buf_header *p = malloc(sizeof(struct buf_header));
    for(i = 0; i < 4; i++){
        p = hash_head[i].hash_fp;
        while(p !=& hash_head[i]){
            p->bufno = count;
            count++;
            p = p->hash_fp;
        }
    }
}

int main(void){
    struct command_table *p = malloc(sizeof(struct command_table));
    char *av[WORD];
    char lbuf[MAXLEN];
    int n=0;
    int *ac;
    ac = &n;
    int i;
    for(i=0;;i++){
        printf("$ ");
        fgets(lbuf,sizeof(lbuf),stdin);

        getargs(lbuf, ac, av);

        for(p=cmd_tbl;p->cmd;p++){
            if(strcmp(av[0],p->cmd)==0){
                (*p->func)(*ac, av);
                break;
            }
        }
        if(p->cmd==NULL){
            fprintf(stderr, "unknown command\n");
        }
        (*ac) = 0;
    }
}