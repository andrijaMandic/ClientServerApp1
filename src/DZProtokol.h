#ifndef __DZPROTOCOL_H_
#define __DZPROTOCOL_H_

#define POGLEDAJ        1
#define DODAJ           2
#define PROMIJENI       3
#define IZBRISI         4
#define BOK             5
#define POGLEDAJ_R      6
#define ODGOVOR         7
#define LOGIN           8
#define INFO            9


// ovo ispod koriste i klijent i server, pa moze biti tu...
#define OK      1
#define NIJEOK  0

int primiPoruku( int sock, int *vrstaPoruke, char **poruka );
int posaljiPoruku( int sock, int vrstaPoruke, const char *poruka );

#define error1( s ) { printf( s ); exit( 0 ); }
#define error2( s1, s2 ) { printf( s1, s2 ); exit( 0 ); }
#define myperror( s ) { perror( s ); exit( 0 ); }

#endif
