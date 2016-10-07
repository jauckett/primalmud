#include <stdio.h>
struct store_type  
{
  char name[60];
  int freq;
};
typedef struct store_type store;

int main(){
  FILE* pl;
  char name[40];
  char sex;
  char day[5],month[5];
  int date,hours,mins,secs;
  store s[1000];
  int found=0;
  int total=0;
  int i,j;  
  pl=fopen("force","r");
  
  while (!feof(pl))
  {
    fscanf(pl,"%s %s %d %d:%d:%d :: %s force-rented and extracted (idle).\n",day,month,&date,&hours,&mins,&secs,name);
    found=0;
    for (i=0;i<total;i++)
      if (!strcmp(s[i].name,name))
      {
        s[i].freq++;
        found=1; 
        break; 
      } 
    if (!found)
    {
      s[total].freq=1;
      strcpy(s[total].name,name); 
      total++;
    } 
  }
  for (i=100;i>=3;i--)
  {
    for (j=0;j<total;j++)
     if(s[j].freq==i)
       printf("%d %s\n",s[j].freq,s[j].name);
  }
  printf("Found %d names\n",total);
  fclose(pl);
}
