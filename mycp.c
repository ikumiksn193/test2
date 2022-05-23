#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<limits.h>
#define WORD 32
#define MAXLEN 1024
void mycp_proc(int, char *[]);
void quit_proc(int, char *[]);
struct command_table{
    char *cmd;
    void (*func)(int, char *[]);
} cmd_tbl[]={
    {"mycp", mycp_proc},
    {"quit", quit_proc},
    {NULL, NULL}
};
void quit_proc(int, char *[]);

int existFile(const char* path){
    FILE *fp = fopen(path, "r");
    if(fp == NULL){
        return 0;
    }
    fclose(fp);
    return 1;
}

void getargs(char *lbuf,int *argc, char *argv[]){
    int flag_moji = 0;
    int flag_end = 0;

    for(int j=0; j<256; j++)
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

void mycp_proc(int ac, char *av[]){
    FILE *fp1;
    FILE *fp2;
    char buf[MAXLEN];
    /*引数の数が正しいか*/
    if(ac != 3){
        fputs("usage: mycp <infile> <outfile> \n", stderr);
        exit(1);
    }
    /*ファイルが存在するかどうか確認
    存在する場合０以外、存在しない場合０*/
    if(existFile(av[2])){
        fputs("Outfile already exists!\nDo you want to change the file? [y/n]\n", stdout);
        char c;
        c = getchar();
        //存在する場合上書きしてよいか尋ねる
        if(c != EOF){
            if(c == 'y'){
                fp1 = fopen(av[1], "r");
                fp2 = fopen(av[2], "w");
                while(fgets(buf, sizeof(buf), fp1) != NULL){
                    if(fputs(buf, fp2) == EOF){
                        perror("fputs");
                    }
                }
            }else if(c == 'n'){
                fputs("changing nothing\n",stdout);
            }
        }
    }else{
        //存在しない場合
        fp1 = fopen(av[1], "r");
        fp2 = fopen(av[2], "w");
        while(fgets(buf, sizeof(buf), fp1) != NULL){
            if(fputs(buf, fp2) == EOF){
                perror("fputs");
            }
        }
    }
    fclose(fp1);
    fclose(fp2);
}

void quit_proc(int ac, char *av[]){
    exit(0);
}

int main(void){
    struct command_table *p = malloc(sizeof(struct command_table));
    char *av[WORD];
    char lbuf[MAXLEN];
    int n = 0;
    int  *ac;
    ac = &n;
    int i;
    for(i=0;;i++){
        printf("$ ");
        fgets(lbuf, sizeof(lbuf), stdin);
        getargs(lbuf, ac, av);

        for(p=cmd_tbl;p->cmd;p++){
            if(strcmp(av[0], p->cmd) == 0){
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