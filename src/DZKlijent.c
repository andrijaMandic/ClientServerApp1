/*
	Parametri komandne linije:
		1. korisnicko ime
		2. ip-adresa na koju se spajamo
		3. port na kojeg se spajamo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "DZProtokol.h"

void obradiLOGIN( int sock, const char *ime );
void obradiPOGLEDAJ(int sock, const char *ime);
void obradiDODAJ(int sock, const char *ime);
void obradiPROMIJENI(int sock, const char *ime);
void obradiIZBRISI(int sock, const char *ime);
void obradiBOK( int sock, const char* ime );

int main (int argc, char **argv)
{
    if( argc != 4 )error2( "Upotreba: %s ime ip port\n", argv[0] );

    char mojeIme[100];
    strcpy( mojeIme, argv[1] ); 
    mojeIme[99]='\0'; //ogranicimo se na 99 slova

    char *dekadskiIP = argv[2];
	int port;
	sscanf( argv[3], "%d", &port );
    
    // socket...
	int mojSocket = socket( PF_INET, SOCK_STREAM, 0 );
	if( mojSocket == -1 )
		myperror( "socket" );

	// connect...
	struct sockaddr_in adresaServera;

	adresaServera.sin_family = AF_INET;
	adresaServera.sin_port = htons( port );

	if( inet_aton( dekadskiIP, &adresaServera.sin_addr ) == 0 )
		error2( "%s nije dobra adresa!\n", dekadskiIP );

	memset( adresaServera.sin_zero, '\0', 8 );

	if( connect(
			mojSocket,
			(struct sockaddr *) &adresaServera,
			sizeof( adresaServera ) ) == -1 )
		myperror( "connect" );

    obradiLOGIN( mojSocket, mojeIme );

    int gotovo = 0;
    while(!gotovo)
    {
        printf( "\n\nClan [%s], Vas odabir?\n", mojeIme );
        printf( "\t0. izlaz\n" );
        printf( "\t1. pregled trenutne liste\n");
        printf( "\t2. dodati novi element na kraj liste\n" );
        printf( "\t3. promijeniti postojeci element liste\n" );
        printf( "\t4. izbrisati pojedini element liste\n" );

        int izbor;
        printf("Unos: ");
        scanf( "%d",&izbor );
        
        switch( izbor )
        {
            case 0: obradiBOK( mojSocket, mojeIme ); gotovo = 1; printf("BOK\n");break;
            case 1: obradiPOGLEDAJ( mojSocket, mojeIme ); sleep(1); break;
            case 2: obradiDODAJ( mojSocket, mojeIme ); sleep(1); break;
            case 3: obradiPROMIJENI( mojSocket, mojeIme ); sleep(1); break;
            case 4: obradiIZBRISI( mojSocket, mojeIme ); sleep(1); break;
            default: printf("Ponavljanje je majka mudrosti! "); sleep(2);
        }

    }
    

return 0;
}

void obradiLOGIN( int sock, const char *ime )
{
    if( posaljiPoruku( sock, LOGIN, ime )==NIJEOK )
        error1( "Pogreska u LOGIN... izlazim.\n" );

    char *odgovor;
    int vrstaOdgovora;

    if( primiPoruku( sock, &vrstaOdgovora, &odgovor )!=OK ) 
        error1("Doslo je do pogreske u komunikaciji sa serverom!\n");
    if(vrstaOdgovora!=ODGOVOR)
        error1("Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)!\n");
    if( strcmp("OK",odgovor) !=0 )
        printf("Greska: %s\n", odgovor);
    else printf("OK");    
    
    free(odgovor);
}


void obradiBOK( int sock, const char *ime )
{
	if( posaljiPoruku( sock, BOK, "" ) == NIJEOK )
		error1( "Pogreska u BYE...izlazim.\n" );

	close( sock );
}

void obradiIZBRISI( int sock, const char* ime )
{
    int kod;
    char kod_[10];
    char poruka[100];    

    printf("Unesite kod poruke koju zelite izbrisati: ");
    fflush(stdin);
    if( scanf("%d",&kod) !=1){ char x; while ( (x = getchar()) != EOF && x != '\n' ); printf("Neispravan unos koda poruke! \n"); return; }
    printf("\t\tUneseni kod: %d\n",kod);
    sprintf(poruka,"%d", kod);


    if( posaljiPoruku( sock, IZBRISI, poruka) != OK )
        error1("Pogreska u IZBRISI... izlazim\n");

    char *odgovor;
    int vrstaOdgovora;

    if( primiPoruku( sock, &vrstaOdgovora, &odgovor )!=OK ) 
        error1("Doslo je do pogreske u komunikaciji sa serverom!\n");
    if(vrstaOdgovora!=ODGOVOR)
        error1("Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)!\n");
    if( strcmp("OK",odgovor) !=0 )
        printf("Greska: %s\n", odgovor);
    else printf("OK\n");

    free(odgovor);
}

void obradiDODAJ( int sock, const char* ime )
{
    printf("Unesite sadrzaj nove poruke koju zelite staviti na TODO listu: \n");
    char sadrzaj[100];
    
    fflush(stdin);
    scanf(" %[^\n]",sadrzaj);
//BITNOOO razmak na pocetku

    //fgets(sadrzaj, 100, stdin);
    //scanf ("%[^\n]%*c", sadrzaj);

    
    //sadrzaj[strlen(sadrzaj)-1] = '\0';

    //printf("\nSadrzaj koji smo upisali: %s\n",sadrzaj);

    //fflush( stdin );
    //scanf("%[^\n]",sadrzaj);
    //fflush( stdin );
    // vec smo obavili LOGIN, pa server zna nase ime, ne moramo ga opet slati u poruci

    if( posaljiPoruku( sock,DODAJ,sadrzaj )!=OK )
        error1("Doslo je do pogreske u komunikaciji sa serverom...\n");
    
    char *odgovor;
    int vrstaOdgovora;

    if( primiPoruku( sock, &vrstaOdgovora, &odgovor )!=OK ) 
        error1("Doslo je do pogreske u komunikaciji sa serverom!\n");
    if(vrstaOdgovora!=ODGOVOR)
        error1("Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)!\n");
    if( strcmp("OK",odgovor) !=0 )
        printf("Greska: %s\n", odgovor);
    else printf("OK\n");


    free(odgovor);    
}

void obradiPROMIJENI( int sock, const char* ime )
{   
    int kod;
    char poruka[100];    

    printf("Unesite kod poruke s TODO liste koju zelite promijeniti: ");
    int p = scanf(" %d",&kod);        
    if( p !=1) {char x; printf("Neispravan unos koda poruke! \n"); while ( (x = getchar()) != EOF && x != '\n' );}
    else {
        sprintf(poruka,"%d", kod);

    printf("\nUnesite sadrzaj nove poruke koju zelite staviti na TODO listu: \n");
    char sadrzaj[100];
    

    //fgets(sadrzaj, 100, stdin);
    scanf(" %[^\n]",sadrzaj);    
    //sadrzaj[strlen(sadrzaj)-1] = '\0';
    //scanf("%[^\n]",sadrzaj);
    // vec smo obavili LOGIN, pa server zna nase ime, ne moramo ga opet slati u poruci
    
    char poruka1[100];
    strcpy(poruka1, poruka);
    strcat(poruka1, sadrzaj);

    if( posaljiPoruku( sock,PROMIJENI,poruka1 )!=OK )
        error1("Doslo je do pogreske u komunikaciji sa serverom...\n");
    
    char *odgovor;
    int vrstaOdgovora;

    if( primiPoruku( sock, &vrstaOdgovora, &odgovor )!=OK ) 
        error1("Doslo je do pogreske u komunikaciji sa serverom!\n");
    if(vrstaOdgovora!=ODGOVOR)
        error1("Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)!\n");
    if( strcmp("OK",odgovor) !=0 )
        printf("Greska: %s\n", odgovor);
    else printf("OK\n");


    free(odgovor);  
    }  
}

void obradiPOGLEDAJ( int sock, const char* ime )
{
    //poruka: "KOLIKO_IH_IMA REDNI_BROJ_1 SADRZAJ_1 REDNI_BROJ2 SADRZAJ_2 ..."
    
    char * odgovor; int vrstaPoruke;
    

    if( posaljiPoruku( sock, POGLEDAJ, "" )!=OK )
        error1("DOSlo je do pogreske u komunikacji sa serverom!\n");

    if( primiPoruku( sock, &vrstaPoruke, &odgovor)!=OK )
        error1("DOslo je do pogreske u komunikaciji sa serverom!\n");
    if(vrstaPoruke!=ODGOVOR)
        error1("Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)!\n");
    if( strcmp("OK",odgovor) !=0 )
        printf("Greska: %s\n", odgovor);
    else printf("OK\n");
    
    free(odgovor);
    char * poruka;
    char *broj_elemenata;
    
    if ( primiPoruku( sock, &vrstaPoruke, &broj_elemenata )!=OK )
        error1("Doslo je do pogreske u komunikaciji sa serverom!\n");
    if( vrstaPoruke != INFO )
        error1("Doslo je do pogreske u komunikaciji, DODAJ nije posalo broj elemenata!\n");
    int ukupno_elemenata;
    sscanf(broj_elemenata,"%d",&ukupno_elemenata);
    free(broj_elemenata);

    printf("\n\n\nSADRZAJ TODO LISTE:\n");
    printf("____________________________\n");    
    printf("%-10s Poruka\n\n","Kod");

    for( int i=0; i<ukupno_elemenata; ++i)
    {
        int tip;
        if( primiPoruku( sock, &tip, &poruka)!=OK )
            error1("DOSLO je do pogreske u komunikaciji sa serverom 22!\n");
        if(tip!=POGLEDAJ_R)
            error1("Doslo je do pogreske u komunikaciji sa serverom (nije poslao POGLEDAJ_R)!\n");   
        
        int kod;
        char sadrzaj[100];

        sscanf(poruka, "%d %[^\n]",&kod,sadrzaj);
        printf("%-10d %s\n", kod, sadrzaj);

    }

    printf("____________________________\n");
    if(ukupno_elemenata!=0)free(poruka);

}














































