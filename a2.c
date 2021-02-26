#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

sem_t sem21,sem22;
sem_t* sem25;
sem_t* sem72;
sem_t sem412;
sem_t group,block,increment;
int count = 0;


void* process4(void* id)
{
    //facem thread-ul 4.12 sa astepte pana vin alte 4 thread-uri
    if((size_t)id == 12)
    {
        sem_wait(&sem412);
    }
    else
    {
        //dupa ce au intrat 4 thread-uri, le blochez pe restul
        //semaforul group avand 4 permisiuni initial
        sem_wait(&group);
    }
    info(BEGIN,4,(size_t)id);

    //semaforul increment are initial o permisiune
    //lasa un singur thread la un moment dat sa incrementeze variabila count
    sem_wait(&increment);
    count++;
    if(count == 4)
    {
        //cand s-au adunat 4 thread-uri, lasam thread-ul 4.12 sa intre, formand un grup de 5 thread-uri
        sem_post(&sem412);
    }
    //sa las si thread-ul urmator sa incrementeze variabila count
    sem_post(&increment);
    
    if( count <= 4 && (size_t)id !=12)
    {
        //nu lasam cele 4 thread-uri sa se termine pana thread-ul 4.12 nu-si termina executia
        //blocam toate cele 4 thread-uri care au ajuns inaintea lui 4.12
      sem_wait(&block);
    }
    
    info(END,4,(size_t)id);
    //dupa ce s-a terminat thread-ul 4.12
    if((size_t)id == 12)
    {
        //deblocam si cele 4 thread-uri ramase in grupul de 5 care ruleaza deodata
        sem_post(&block);
        sem_post(&block);
        sem_post(&block);
        sem_post(&block);
    }
    //lasam si celelalte 45 de thread-uri sa ruleze
    sem_post(&group);

    return 0;
}


void* process7(void* id)
{
    //blocam thread-ul 7.2 pana se termina 2.5
    if((size_t)id ==2){
     
     sem_wait(sem72);
    }
    info(BEGIN,7,(size_t)id);
    
    info(END,7,(size_t)id);
    
     //se termina thread-ul 7.4, anunta thread-ul 2.5 ca poate incepe
    if((size_t)id == 4){
       
        sem_post(sem25);
    }
    return 0;
}


void* process2(void* id)
{     //thread-ul 2.5 nu poate incepe, e blocat pana se termina 7.4
    if((size_t)id == 5){
       
      sem_wait(sem25);
    }
    
    //pun thread-ul 2.2 sa astepte pana cand ajunge thread-ul 2.1
    if((size_t)id==2)
    {
        sem_wait(&sem22);
    }
    info(BEGIN,2,(size_t)id);
     //a ajuns thread-ul 2.1, anuntam thread-ul 2.2 ca poate sa inceapa
     //blocam thread-ul 2.1 pana cand se termina si thread-ul 2.2
    if((size_t)id==1)
    {
        sem_post(&sem22);
        sem_wait(&sem21);
    }
    info(END,2,(size_t)id);
      
      //thread-ul 2.5 se termina, anuntam thread-ul 7.2 ca poate incepe
    if((size_t)id == 5){
        
        sem_post(sem72);
    }


   //thread-ul 2.2 si-a terminat executia
   //lasam thread-ul 2.1 sa-si continue executia
    if((size_t)id==2)
    {
        sem_post(&sem21);
    }
    return 0;
}

int main(int argc, char **argv){
    pid_t P2,P3,P4,P5,P6,P7;//procesele pt ierarhie
    init();

    sem_unlink("semaphore25");
    sem_unlink("semaphore72");


    sem25 = sem_open("semaphore25", O_CREAT, 0644, 0);
    sem72 = sem_open("semaphore72", O_CREAT, 0644, 0);
 
    info(BEGIN,1,0);

    P2=fork();
    if(P2==0)
    {
        info(BEGIN,2,0);

        //primul 0: vizibilitate semaforului fata de alte procese
        //al doilea: nr de permisiuni
        sem_init(&sem21,0,0);
        sem_init(&sem22,0,0);
        
        P3=fork();
        if(P3==0)
        {
            info(BEGIN,3,0);
            info(END,3,0);
            //exit pt a stii cand se termina
            exit(1);
        }
        
        P6=fork();
        if(P6==0)
        {
            info(BEGIN,6,0);
            info(END,6,0);
            exit(1);
        }
        
        P7=fork();

        if(P7==0)
        {
        pthread_t thread[4];
        for(int i=1;i<=4;i++){
            //size_t in loc de int ca sa eliminam warning-ul dat de compilator
            pthread_create(&thread[i],NULL,process7,(void*) (size_t) i);
        }
        for(int i=1;i<=4;i++){
            //asteapta pt thread-uri
            pthread_join(thread[i],NULL);
        }
            info(BEGIN,7,0);
            info(END,7,0);
            exit(1);
        }

        pthread_t thread[5];
        for(int i=1;i<=5;i++){
            //size_t in loc de int ca sa eliminam warning-ul dat de compilator
            pthread_create(&thread[i],NULL,process2,(void*) (size_t) i);
        }
        for(int i=1;i<=5;i++){
            //asteapta pt thread-uri
            pthread_join(thread[i],NULL);
        }
        
        waitpid(P3,NULL,0);
        waitpid(P6,NULL,0);
        waitpid(P7,NULL,0);

        info(END,2,0);
        exit(1);
    }

    P4=fork();

    
    if(P4==0)
    {
        sem_init(&increment,0,1);
        sem_init(&group,0,4);
        sem_init(&block,0,0);//trebuie blocate
        sem_init(&sem412,0,0);
        info(BEGIN,4,0);
        pthread_t thread[50];
        for(int i=1; i<=50;i++){
            pthread_create(&thread[i],NULL,process4,(void*) (size_t)i);
        }
        for(int i=1;i<=50;i++){
            pthread_join(thread[i],NULL);
        }
        

        

        P5=fork();
        if(P5==0)
        {
            info(BEGIN,5,0);
            info(END,5,0);
            exit(1);
        }
        
        waitpid(P5,NULL,0);
        info(END,4,0);
        exit(1);
    }

    waitpid(P2,NULL,0);
    waitpid(P4,NULL,0);

    info(END,1,0);
    return 0;

}