#include<stdio.h>
#include<stdlib.h>

int main ()
{
    
    printf("Unesite broj: ");
    int broj;
    printf("%d\n",scanf("%d",&broj));

    printf("%d\n",broj);
    fflush(stdin);
    scanf("%d",&broj);
    printf("\t%d",broj);

return 0;
}

/*
https://stackoverflow.com/questions/34552519/when-scanf-returns-0-in-c-and-just-doesnt-work

#include<stdio.h>
int main(void) {
   int val,x;
   x=scanf("%d",&val);
   if(x==1)
      printf("success!");
   else{
      // Discard everything upto and including the newline.
      while ( (x = getchar()) != EOF && x != '\n' );
      printf("try again\n");
      scanf("%d",&val);
   }
   return 0;
}

*/
