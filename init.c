#include<stdio.h>  
#include<stdlib.h>  
#include<ctype.h>  
#include<string.h>  
#include<unistd.h>  
#include<fcntl.h>  
#include<sys/types.h>  
#include<sys/wait.h>  
#include<dirent.h>  
  
char cmd[256];  
char args[128][128];
int count;  
enum specify{NORMAL,OUT_REDIRECT,IN_REDIRECT,OUT_REDIRECT2,PIPE};  
int a_count=0;
char a_arg[128][128];
char a_data[128][128];  
int analysis();  
int execute();
void env_set(char *s);
void alias_set(char *s);  
int find_command(char *cmd);  
int main()  
{    
    while(1){
    printf("# ");
    fflush(stdin);
    fgets(cmd, 256, stdin);  
    if(!analysis())  
       continue;
    execute();
    }  
    return 0;  
}  
  
int analysis()  
{ 
    char argt[128][128]; 
    char *s=cmd;
    int i=0,j=0,z=0,k=0,acnt=0,t=0;  
    while(*s){  
    if(z){    
           if(!isspace(*s))  
           z=0;  
           else  
           s++;
           t=0;  
           }  
          else{ 
          if((*s=='\''||*s=='\"')&&t==0)
           {
            t=1;
            args[i][j]=*s;
            j++;
           }
          else if(*s=='\''||*s=='\"')
           {
             args[i][j]=*s;
             j++;
             args[i][j]='\0';
             t=0;
             i++;
             j=0;
             z=1;
           }
          else if(t)
           {
            args[i][j]=*s;
            j++;
            }
          else if(isspace(*s)){  
           args[i][j]='\0';  
           i++;  
           j=0;  
           z=1;  
           }else{  
           args[i][j]=*s;  
           j++;  
           }  
           s++;  
    }  
    }  
    count=i;  
    if(count==0)  
    return 0;  
    
    for(i=0;i<count;i++)
       for(j=0;j<a_count;j++)
       if(strcmp(args[i],a_arg[j])==0)
            {
       strcpy(args[i],a_data[j]);
       s=args[i];
       acnt=0;
       k=0;
       j=0;
       z=0;
       while(*s){
       if(z){
           if(!isspace(*s))
           z=0;
           else
           s++;
           }
          else{
          if(isspace(*s)){
           argt[k][j]='\0';
           k++;
           j=0;
           z=1;
           }else{
           argt[k][j]=*s;
           j++;
           }
           s++;
    }
    }
    acnt=k+1;
    if(acnt==1)
       continue;
    for(j=count-1;j>=i;j--)
        strcpy(args[j+acnt-1],args[j]);
    for(j=0;j<acnt;j++)
        strcpy(args[j+i],argt[j]);
    count+=acnt-1;
    continue;
  }

    for(i=0;i<count;i++)
    {
      j=0;
      if(args[i][j]!=34&&args[i][j]!=39)
         continue;
      else {while(args[i][j+1])
              j++ ;
          if(args[i][0]==args[i][j])
           { 
             for(k=0;k<j-1;k++)
              args[i][k]=args[i][k+1];
             args[i][k]='\0'; 
           }
       }
    }

    if(strcmp(args[0],"cd")==0)
     {
            if (args[1])
                chdir(args[1]);
            return 0;
        }

    if(strcmp(args[0],"export")==0)
    {
            if(args[1])
                env_set(args[1]);
            return 0;
    }
    if(strcmp(args[0],"alias")==0)
    {
            if(args[1])
               alias_set(args[1]);
            return 0;
    }
    if( strcmp(args[0],"exit")==0)  
     {   exit(0);}
  /*    if(!find_command(args[0])){  
    puts("error:can't find command");  
    return 0;  
    }*/  
    return 1;     
}  
void env_set(char *s)
{
    char* arg2;
    int t;
    for(t=0;s[t]!='=';t++);
    s[t]='\0';
    arg2=s+t+1;
    if(*arg2)
      setenv(s,arg2,1);
    else printf("error:no value");
    return;
} 
void alias_set(char *s)
{
    char* arg2;
    int t;
    int j=0;
    int k;
    for(t=0;s[t]!='=';t++);
    s[t]='\0';
    arg2=s+t+1;
    if(arg2[j]==34||arg2[j]==39)
       {while(arg2[j+1])
              j++ ;
          if(arg2[0]==arg2[j])
           {
             for(k=0;k<j-1;k++)
              arg2[k]=arg2[k+1];
             arg2[k]='\0';
           }
       }

    strcpy(a_arg[a_count],s);
    strcpy(a_data[a_count],arg2);
    a_count++;
    return;
}
int execute()  
{  
    int i,j;  
    char* file;  
    char* arg[50];  
    char* arg2[50];  
    int f=0,back_run=0;  
    int fd,pid,fd2,pid2;  
    enum specify type=NORMAL;  
  
    for(i=0;i<count;i++){  
    arg[i]=args[i];  
    }  
    arg[i]=NULL;  
  
    if(strcmp(arg[count-1],"<")==0 || strcmp(arg[count-1],">")==0 || strcmp(arg[count-1],"|")==0){  
    printf("error:command error\n");  
    return 0;  
    }  
          
    for(i=0;i<count;i++){  
    if(strcmp(arg[i],"<")==0){  
            f++;  
        file=arg[i+1];  
        arg[i]=NULL;  
        type=IN_REDIRECT;  
    }else if(strcmp(arg[i],">>")==0){
        f++;
        file=arg[i+1];
        arg[i]=NULL;
        type=OUT_REDIRECT2;
    }
    else if(strcmp(arg[i],">")==0){  
        f++;  
        file=arg[i+1];  
        arg[i]=NULL;  
        type=OUT_REDIRECT;  
    }else if(strcmp(arg[i],"|")==0){  
        f++;  
        type=PIPE;  
        arg[i]=NULL;  
        for(j=i+1;j<count;j++){  
        arg2[j-i-1]=arg[j];  
        }  
        arg2[j-i-1]=NULL;  
       /* if(!find_command(arg2[0])){  
        printf("error:can't find command\n");  
        return 0;  
        }*/  
    }  
    }  
  
    if(strcmp(arg[count-1],"&")==0){  
    back_run=1;  
    arg[count-1]=NULL;  
    }  
  
    if(f>1){  
    printf("error:don't identify the command");  
    return 0;  
    }  
  
    pid=fork();  
    if(pid<0){  
    perror("fork error");  
        exit(0);  
    }else if(pid==0){  
        switch(type){  
            case NORMAL:  
        execvp(arg[0],arg);
        printf("command not found\n");
         exit(1);
            break;  
        case IN_REDIRECT:  
        fd=open(file,O_RDONLY);  
        dup2(fd,STDIN_FILENO);  
                execvp(arg[0],arg);  
        break;  
        case OUT_REDIRECT:  
        fd=open(file,O_WRONLY|O_CREAT|O_TRUNC,0666);  
        dup2(fd,STDOUT_FILENO);  
        execvp(arg[0],arg);  
        break;
        case OUT_REDIRECT2:
        fd=open(file,O_WRONLY|O_CREAT|O_APPEND,0666);
        dup2(fd,STDOUT_FILENO);
        execvp(arg[0],arg);
        break;  
        case PIPE:  
        pid2=fork();  
        if(pid2==0){  
            fd2=open("tempfile",O_WRONLY|O_CREAT|O_TRUNC,0600);  
            dup2(fd2,STDOUT_FILENO);  
            execvp(arg[0],arg);  
        }else{  
            waitpid(pid2,NULL,0);  
            fd=open("tempfile",O_RDONLY);  
            dup2(fd,STDIN_FILENO);  
            execvp(arg2[0],arg2);  
        }  
        break;  
        }  
    }else{  
       if(!back_run)  
           waitpid(pid,NULL,0);  
    }  
    return 1;  
}  
  
  
  
/*int find_command(char *cmd)  
{
    DIR *d;  
    struct dirent *ptr;  
    char temp[100];  
    char *dir;  
    char *path=getenv("PATH");  
    strcpy(temp,path);  
    dir=strtok(temp,":");  
    while(dir){  
        d=opendir(dir);  
        while((ptr=readdir(d)) != NULL)  
        if(strcmp(ptr->d_name,cmd) == 0){  
        closedir(d);  
        return 1;  
        }  
    closedir(d);  
    dir=strtok(NULL,":");  
    }  
    return 0;  
} */ 
