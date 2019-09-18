/*
	Parametri komandne linije:
		1. port na kojeg se spajamo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "DZProtokol.h"

int obradiPOGLEDAJ(int sock, const char *ime, const char *poruka);
int obradiDODAJ(int sock, const char *ime, const char *poruka);
int obradiPROMIJENI(int sock, const char *ime, const char *poruka);
int obradiIZBRISI(int sock, const char *ime, const char *poruka);
//int obradiBOK();
int obradiLOGIN(int sock, const char *ime);

#define MAXDRETVI 3

typedef struct _poruka
{
    char tijelo[100];
    int kod; //jedinstveni identifikator
    char *ime_autora;
    struct _poruka *next;
}poruka;
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//paziti da oslobodimo memoriju za ime_autora, te za vezanu listu

typedef struct
{
    int commSocket;
    int indexDretve;
}obradiKlijenta__parametar;

int broj_clanova; //koji sudjeluju u kreiranju TO DO liste
char **clanovi;

int generator = 0; // broj_poruka = 0; mozda nam ne treba jer pamtimo prvu i zadnju
poruka *first = NULL, *last = NULL;



int aktivneDretve[MAXDRETVI] = {0};
obradiKlijenta__parametar parametarDretve[MAXDRETVI];

pthread_mutex_t lokot_aktivneDretve = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lokot_filesystem = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lokot_poruka = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lokot_clanovi = PTHREAD_MUTEX_INITIALIZER;

//dodati lokote 


void krajKomunikacije(void *parametar, const char *ime)
{
    obradiKlijenta__parametar *param = (obradiKlijenta__parametar*)parametar;
    int commSocket = param->commSocket;
    int indexDretve = param->indexDretve;

    printf("User %s odlazi!\nKoristena dretva: [%d]\n\n", ime, indexDretve);
    
    pthread_mutex_lock( &lokot_aktivneDretve );
    aktivneDretve[indexDretve] = 2;
    pthread_mutex_unlock( &lokot_aktivneDretve );

    close( commSocket );
    

}

void *obradiKlijenta(void *parametar)
{

    obradiKlijenta__parametar *param = (obradiKlijenta__parametar*) parametar;
    int commSocket = param->commSocket;

    int vrstaPoruke;
    char *poruka;

    //prvo mora ici LOGIN
    if( primiPoruku( commSocket, &vrstaPoruke, &poruka )!= OK )
    {
        krajKomunikacije( parametar, "" );
        return NULL;
    }

    if( vrstaPoruke != LOGIN )
    {
        krajKomunikacije( parametar, "" );
        return NULL;
    }
    
    char *imeKlijenta = (char*)malloc( (strlen(poruka)+1)*sizeof(char) );
    strcpy( imeKlijenta, poruka );

    if( obradiLOGIN( commSocket, imeKlijenta)!= OK )
    {
        krajKomunikacije( parametar, imeKlijenta );
        return NULL;
    }
    free(poruka);

    //obradujemo ostale zahtjeve koje bira korisnik
    int gotovo = 0;
    while( gotovo==0 )
    {
        if( primiPoruku( commSocket, &vrstaPoruke, &poruka )!=OK )
        {
            krajKomunikacije( parametar, imeKlijenta );
            printf("Error: primi  poruku (1)\n");
            gotovo = 1;
            continue;        
        }

        switch( vrstaPoruke )
        {

            case BOK: 
                krajKomunikacije( parametar, imeKlijenta ); gotovo = 1; break;

            case DODAJ:
                if( obradiDODAJ( commSocket, imeKlijenta, poruka )!= OK )
                {
                    krajKomunikacije(parametar, imeKlijenta); gotovo=1;            
                } 
                break;
            
            case POGLEDAJ:
                if( obradiPOGLEDAJ( commSocket, imeKlijenta, poruka )!= OK )
                {
                    krajKomunikacije(parametar, imeKlijenta); gotovo=1;            
                } 
                break;

            case PROMIJENI:
                if( obradiPROMIJENI( commSocket, imeKlijenta, poruka )!= OK )
                {
                    krajKomunikacije(parametar, imeKlijenta); gotovo=1;            
                } 
                break;

            case IZBRISI:
                if( obradiIZBRISI( commSocket, imeKlijenta, poruka )!= OK )
                {
                    krajKomunikacije(parametar, imeKlijenta); gotovo=1;            
                } 
                break;
            default: krajKomunikacije( parametar, imeKlijenta ); gotovo = 1; break;            
        }

        free(poruka);
        //sleep(5);
    }
    free(imeKlijenta);
    return NULL;
}


int main (int argc, char **argv)
{

    if( argc != 2 )
		error2( "Upotreba: %s port\n", argv[0] );

	int port;
	sscanf( argv[1], "%d", &port );

    // socket...
	int listenerSocket = socket( PF_INET, SOCK_STREAM, 0 );
	if( listenerSocket == -1 )
	    perror( "socket" );
    
    // bind...
	struct sockaddr_in mojaAdresa;

	mojaAdresa.sin_family      = AF_INET;
	mojaAdresa.sin_port        = htons( port );
	mojaAdresa.sin_addr.s_addr = INADDR_ANY;
	memset( mojaAdresa.sin_zero, '\0', 8 );

	if( bind(listenerSocket,(struct sockaddr *) &mojaAdresa,
        sizeof( mojaAdresa ) ) == -1 ) perror( "bind" );
    
    // listen...
	if( listen( listenerSocket, 10 ) == -1 )
		perror( "listen" );

	// ovo mozemo bolje rijesiti i pomocu liste dretvi, ali dobro...
	pthread_t dretve[10];

    while(1)
    {
        // accept...
		struct sockaddr_in klijentAdresa;
		unsigned int lenAddr = sizeof( klijentAdresa );
		int commSocket = accept( listenerSocket,
                         (struct sockaddr *) &klijentAdresa,
                         &lenAddr );

		if( commSocket == -1 )
			perror( "accept" );

        char *dekadskiIP = inet_ntoa( klijentAdresa.sin_addr );
		printf( "Prihvatio konekciju od %s ", dekadskiIP );
    
        pthread_mutex_lock ( &lokot_aktivneDretve );
        int i, indexNeaktivne = -1;
        for( i=0; i<MAXDRETVI; ++i )
            if( aktivneDretve[i]==0 ) indexNeaktivne = i;
            else if ( aktivneDretve[i]==2 )
            {
                pthread_join( dretve[i], NULL );
                aktivneDretve[i] = 0;
                indexNeaktivne = i;            
            }

        if( indexNeaktivne == -1) 
        {
            close( commSocket ); //sve dretve zauzete
            printf("Odbijena konekcija jer su zauzete sve dretve.\n");
        }
        else
        {
            aktivneDretve[indexNeaktivne] = 1;
            parametarDretve[indexNeaktivne].commSocket = commSocket;
            parametarDretve[indexNeaktivne].indexDretve = indexNeaktivne;
            printf("... koristim dretvu broj %d\n", indexNeaktivne);

            pthread_create( &dretve[indexNeaktivne], NULL,
                            obradiKlijenta, &parametarDretve[indexNeaktivne] );

         }

         pthread_mutex_unlock( &lokot_aktivneDretve );
        
    }

return 0;
}

int obradiLOGIN(int sock, const char *ime)
{

    int postoji = 0;

    pthread_mutex_lock( &lokot_clanovi );
    
    //provjera ima li vec tog clana u memoriji
    for( int i=0; i<broj_clanova; ++i )
    {
        if( strcmp( clanovi[i], ime )==0 )
        {
            postoji = 1;
            break;
        }
    }

    if( !postoji )
    {
        clanovi = (char **)realloc(clanovi, (++broj_clanova)*sizeof(char*) );
        if(clanovi==NULL)
        {
            pthread_mutex_unlock( &lokot_clanovi );
            //tu ce ostat poremeceno char **clanovi
            posaljiPoruku( sock, ODGOVOR, "Greska u memoriji kod LOGIN\n" );
            return NIJEOK;    
        }


        clanovi[broj_clanova-1] = (char*)malloc( (strlen(ime)+1)*sizeof(char) );
        strcpy( clanovi[broj_clanova-1],ime );
    }
    
    pthread_mutex_unlock( &lokot_clanovi );
    posaljiPoruku( sock, ODGOVOR, "OK" );
    return OK;

}

int obradiDODAJ(int sock, const char *ime, const char *poruka)
{
    //dodajemo nesto na TO DO listu, pazimo da zabiljezimo autora poruke
    //MOGUCA POMUTNJA STRUKTURE PORUKA I const char *poruka

    struct _poruka *nova;
    nova = (struct _poruka*)malloc(sizeof(struct _poruka));
    if(nova==NULL)
    {
       // pthread_mutex_unlock( &lokot_poruka );
        posaljiPoruku( sock, ODGOVOR, "Greska u memoriji kod DODAJ\n" );
        return NIJEOK;
    }


    strcpy(nova->tijelo, poruka);
    
    pthread_mutex_lock( &lokot_poruka );
    nova->kod = ++generator;
    
    nova->ime_autora = (char*)malloc( (strlen(ime)+1)*sizeof(char) );
    if(nova->ime_autora==NULL)
    {
        pthread_mutex_unlock( &lokot_poruka );
        posaljiPoruku( sock, ODGOVOR, "Greska u memoriji kod DODAJ\n" );
        return NIJEOK;
    }    
    strcpy( nova->ime_autora, ime );
    
    if(first) //last = last->next = nova;
    { //tu vec postoji neka poruka

        last->next = nova;
        last = nova;
    }
    else// last = first = nova; //prvi u nizu prouka
    {
        first = nova;
        last = nova;
    }    
    last->next = NULL;

    pthread_mutex_unlock( &lokot_poruka );
    
    posaljiPoruku( sock, ODGOVOR, "OK" );
    
    printf("Dodali smo novu poruku:\n\tautor: %s\n\tsadrzaj: %s\n\tkod: %d\n",nova->ime_autora,nova->tijelo, nova->kod);

    //free(nova->ime_autora);
    //free(nova);
    
    return OK;
}

int obradiPOGLEDAJ(int sock, const char *ime, const char *poruka)
{
    //tu cemo isto zakljucavati podatke
    //trebamo poslati POGLEDAJ_R, no prije toga ODGOVOR!
    //mozemo prije pravih podataka u poruci poslati dio koji kaze koliko ima na listi
    pthread_mutex_lock( &lokot_poruka );
    
    struct _poruka *temp;

    int duljina = 10; // pretp da ce nam to biti dovoljno da spremimo na pocetku koliko ima poruka u nasoj TOdo listi
    int brojac = 0; //da vidimo koliko ukupno poruka ima

    for( temp=first; temp; temp = temp->next ) brojac++;
    //mozemoi binarno, ali idemo preko stringa
    char info[100];
    sprintf( info, "%d", brojac );
    
    if( posaljiPoruku( sock,ODGOVOR,"OK" )!= OK){printf("greska 1");     pthread_mutex_unlock( &lokot_poruka ); return NIJEOK;}
    //IDEJA: saljemo prvo koliko ima poruka, pa posebno svaku, "KOD_PORUKE TIJELO_PORUKE"
    if( posaljiPoruku( sock, INFO, info )!= OK ){printf("greska 2");    pthread_mutex_unlock( &lokot_poruka ); return NIJEOK;} 

    for( temp=first; temp; temp=temp->next )
    {
        char broj[12];
        sprintf( broj, "%d", temp->kod );
        char *saljemo;
        saljemo = (char*)malloc( (strlen(broj)+strlen(temp->tijelo)+1 )*sizeof(char) );

        //pakiramo poruku
        strcpy(saljemo, broj);
        strcat(saljemo, " ");
        strcat(saljemo, temp->tijelo);

        if( posaljiPoruku( sock, POGLEDAJ_R, saljemo )!= OK)                             {free(saljemo);printf("greska 2"); pthread_mutex_unlock( &lokot_poruka ); return NIJEOK;} 

        free(saljemo);
    }
    //ili mozda tek tu free(saljemo)
    
    pthread_mutex_unlock( &lokot_poruka );

    return OK;
}


int obradiPROMIJENI(int sock, const char *ime, const char *poruka)
{
    //poslati ODGOVOR
    //ne treba paziti tko mijenja listu

    //poruka primljena od klijenta u obliku : "KOD_ELEMENTA NOVA_PORUKA"

    int kod;
    char sadrzaj[100];
    if(sscanf(poruka,"%d %[^\n]",&kod,sadrzaj) != 2 )
    {
        posaljiPoruku( sock, ODGOVOR, "Greska kod slanja promjene\n" );
        return OK;
    }

    pthread_mutex_lock( &lokot_poruka );    

    struct _poruka *temp;
    for(temp=first; temp; temp=temp->next)
    {
        if(temp->kod==kod)
        {
            strcpy(temp->tijelo, sadrzaj);
            //PROVJERA
            //printf("Promjena:\n\tautor: %s\n\tsadrzaj: %s\n\tkod: %d\n",temp->ime_autora,temp->tijelo,temp->kod);
            break;
        }
    }

    posaljiPoruku(sock, ODGOVOR, "OK");
    pthread_mutex_unlock( &lokot_poruka );

    return OK;
    
}

int obradiIZBRISI(int sock, const char *ime, const char *poruka)
{
    //u poruci ce biti identifikacijski kod poruke koju treba izbrisat
    //treba provjeriti smije li osoba koja trazi uslugu izbrisati tu poruku
    //tj je li njen izvorni autor

    // MOGUCI PROBLEMI PRI BRISANJU PRVOG/ZADNJEG ELEMENTA S TO DO LISTE

    int kod;
    if(sscanf(poruka,"%d",&kod)!=1)
    {
        posaljiPoruku( sock, ODGOVOR, "Neispravno nesto kod KODA elementa\n" );
        return NIJEOK;
    }

    pthread_mutex_lock( &lokot_poruka );

    struct _poruka* temp;
    int nasli = 0;
    for(temp=first; temp; temp = temp->next)
    {
        if(temp->kod==kod)
        {
            nasli = 1;
            if(strcmp(ime,temp->ime_autora)==0)
            {
                //brisemo poruku, moramo paziti brisemo li prvi element liste
                if(temp==first)
                {
                    first = temp->next;
                    if(temp==last)last=NULL;
                    //imali smo samo jedan element u listi, sad su nam i first i last == NULL
                    free(temp->ime_autora);
                    free(temp);
                    
                }
                else 
                {
                    //brisemo zadnji element koji nije ujedno i prvi
                    //sigurno ima bolji nacin od sljedeceg
                    struct _poruka *pom;
                    for(pom=first; pom->next!=temp; pom=pom->next);
                    //sad u pom imamo adresu prethodnog clana liste
                    pom->next = temp->next;
                    
                    if(last==temp) last = pom;
                    free(temp->ime_autora);
                    free(temp);
                }
                            
            }
            else 
            {
                char odg[100];
                sprintf(odg,"Jedino osoba %s smije izbrisati tu poruku!\n",temp->ime_autora);
                posaljiPoruku( sock,ODGOVOR,odg);
                pthread_mutex_unlock( &lokot_poruka );
                return OK;
                //jos uvijek vracamo OK
                
            }

        break;
        }
        

    }

    pthread_mutex_unlock( &lokot_poruka );

    if(!nasli)
    {
        posaljiPoruku( sock, ODGOVOR, "Ne postoji poruka s trazenim kodom.\n" );
        
    }
    else posaljiPoruku( sock, ODGOVOR, "OK" );

    return OK;
}

















