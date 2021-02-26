#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct _section
{
	char name[13];
	int type;
	int offset;
	int size;
}Section;

int checkSF_AndSection(char* filePath)
{ 
   int version = 0;
   int no_sections = 0;
   int header_size = 0;
   char magic[5];
   int seekingSection = 0;//numar cate sectiuni au tipul 76
   
   int file = -1;
   file = open(filePath, O_RDONLY);
   if(file == -1)
   {
   	 perror("Could not open the file");
   	 return -1;
   }
   
   lseek(file, -6, SEEK_END);
   read(file, &header_size, 2);
   read(file, magic, 4);
   magic[4] = '\0';
   
   if(strncmp(magic, "dWXg", 4) != 0){
	   close(file);
   		return 0; //daca fisierul nu-i bun, se opreste din a mai citi sectiunile
		
   }
   
   
   lseek(file, -header_size, SEEK_END);
   read(file, &version, 2);
   read(file, &no_sections, 1);
   
   
   if(version < 21 || version > 53){
       close(file);
   		return 0;
   }
   			
   
   if(no_sections < 5 || no_sections > 15){

      close(file);
 		return 0;
   }
   
  Section* sections = (Section*)malloc(no_sections * sizeof(Section)); 
   
   for(int i = 0; i < no_sections; i++)
   {
   		
   		read(file, sections[i].name, 12);
   		sections[i].name[12] = '\0';
   		read(file, &sections[i].type, 1 );
   		read(file, &sections[i].offset, 4 );
   		read(file, &sections[i].size, 4 );
   		
   		if(sections[i].type == 76) 
   			seekingSection++;//numar sectiunile gasite
    }
   	
    if(seekingSection > 0){
		close(file);
		free(sections);
		return 1;//am gasit sectiune cu tipul 76->afisez
	}
	else
	{
		close(file);
		free(sections);
		return 0;   
	}
}


void findAll(char* path)
{
	DIR* dir = NULL;
	struct dirent *dirEntry = NULL;
	struct stat inode;
	char path_name[1500];//salvez intregul path catre fisier

	dir = opendir(path);
	if ( dir == NULL ) 
	{
		printf("ERROR\n");
		printf("invalid directory path");
		return;
	}
          
	while ((dirEntry=readdir(dir)) != NULL ) 
	{
		if( strcmp(dirEntry->d_name,".") != 0 && strcmp(dirEntry->d_name,"..") != 0) 
		{
			snprintf(path_name, 1500, "%s/%s",path,dirEntry->d_name);
	        
	        if(lstat(path_name, &inode)==0)
			{  //extrag orice nu e director
	    		if(S_ISREG(inode.st_mode)) 
		 		{    //cauta sa fie sectionFile si sa aiba val 76
		 			if(checkSF_AndSection(path_name) == 1 ) //return 1->success
   						printf("%s\n", path_name);//afisez calea
		 		}
		 		
		 		if(S_ISDIR(inode.st_mode))//daca dau de alt director
					findAll(path_name);	//caut recursiv si in el
			}
		}
	}

	closedir(dir);	//cand am terminat, inchid directorul

}


void extract(char* filePath, int section, int line)
{
   int version = 0;
   int no_sections = 0;
   int header_size = 0;
   char magic[5];
   
   int file = -1;
   file = open(filePath, O_RDONLY);
   
   if(file == -1)
   {
   		printf("ERROR\n");
   		printf("invalid file\n");
   		return;
   }
   
   lseek(file, -6, SEEK_END);
   read(file, &header_size, 2);//citesc 2 bytes pt header_size
   read(file, magic, 4);//4 bytes pt magic
   magic[4] = '\0';//sa stiu unde se termina string-ul
   
   lseek(file, -header_size, SEEK_END);//merg la inceputul headerului
   read(file, &version, 2);//2 bytes care reprezinta versiunea
   read(file, &no_sections, 1);//1-dimensiunea sectiunilor
   
   if( section > no_sections)
   {
   		printf("ERROR\n");
   		printf("invalid section");
		   close(file);
   		return;
   }
   //aloc memorie pt sectiuni
   Section* sections = (Section*)malloc(no_sections * sizeof(Section));
   for(int i = 0; i < no_sections; i++)
   {
   		read(file, sections[i].name, 12);//citesc numele pt sectiunea curenta
   		sections[i].name[12] = '\0';//aici se termina string-ul
   		read(file, &sections[i].type, 1 );//dimensiunea type-ului: 1 byte
   		read(file, &sections[i].offset, 4 );
   		read(file, &sections[i].size, 4 );
   }
   //ma deplasez in fisier unde incepe sectiunea cautata
   lseek(file, sections[section-1].offset -1 , SEEK_SET);
   //initializez beginul cu offset-ul sectiunii cautate
   int begin = sections[section-1].offset;
   //imi arata unde se termina sectiunea
   int endOffset = sections[section-1].offset + sections[section-1].size;
   //numara cate linii am intalnit, unde incepe sectiunea incepe si prima linie
   int no_lines = 1;
   
   //daca pozitia in fisier e mai mica decat finalul, atunci caut
   while(lseek(file,0,SEEK_CUR) <= endOffset)
   { //citesc cate un caracter
   	 	char ch;
   	 	read(file, &ch,1);//citesc in ch un byte
   	 	if(ch == '\n')
   	 		no_lines++;//am dat de inca o linie
   	 	if(no_lines == line)
   	 		{ //tin minte inceputul liniei pe care o caut
   	 			begin = lseek(file,0,SEEK_CUR);//salvez pozitia unde incepe linia cautata
   	 			break;//daca am gasit linia, ies din while
   	 		}	
   
   }
   
   if(line > no_lines)
   {
   		printf("ERROR\n");
   		printf("wrong line\n");
		   close(file);
		   free(sections);
   		return;
   }
   
   
   lseek(file,0,begin);//ma pozitionez unde incepe linia
   printf("SUCCESS\n");
   while(lseek(file,0,SEEK_CUR) <= endOffset) //pozitia curenta <= unde se termina sectiunea
   {
   		char ch;
   		read(file,&ch,1);
   		if( ch == '\n') //am terminat linia, nimic de afisat
   			break;
   		else
   			printf("%c",ch);
   }
   free(sections);
   close(file);
}
   
void checkSF(char* filePath)
{ 
   int version = 0;
   int no_sections = 0;
   int header_size = 0;
   char magic[5];
   
   int file = -1;
   file = open(filePath, O_RDONLY);
   if(file == -1)
   {
   		perror("Could not open the file");
   }
   
   lseek(file, -6, SEEK_END);
   read(file, &header_size, 2);
   read(file, magic, 4);
   magic[4] = '\0';
   
   if(strcmp(magic, "dWXg") != 0)
   {
   		printf("ERROR\n");
   		printf("wrong magic\n");
		close(file);
   		return;
   }
   
   
   lseek(file, -header_size, SEEK_END);
   read(file, &version, 2);
   read(file, &no_sections, 1);
   
   
   if(version < 21 || version > 53)
   {
   		printf("ERROR\n");
   		printf("wrong version");
		close(file);
   		return;
   			
   }
   
   if(no_sections < 5 || no_sections > 15)
   {
   		printf("ERROR\n");
   		printf("wrong sect_nr");
		close(file);
   		return;
   
   }
   
   Section* sections = (Section*)malloc(no_sections * sizeof(Section)); //vector de variabile de tip section (alocat dinamic)
   
   for(int i = 0; i < no_sections; i++)
   {
   		read(file, sections[i].name, 12);
   		sections[i].name[12] = '\0';
   		read(file, &sections[i].type, 1 );
   		read(file, &sections[i].offset, 4 );
   		read(file, &sections[i].size, 4 );
   		
		   //verific daca sectiunea curenta e diferita de 76, 35, 37->eroare
   		if(sections[i].type != 76 && sections[i].type != 35 && sections[i].type != 37
   			&& sections[i].type != 18 && sections[i].type != 24 && sections[i].type != 30)
   			{
   				printf("ERROR\n");
   				printf("wrong sect_types");
   				free(sections);
				close(file);
   				return;
   			}
   	}
   	
   	printf("SUCCESS\n");
   	printf("version=%d\n", version);
   	printf("nr_sections=%d\n", no_sections);
   	
   	for(int i = 0; i < no_sections; i++)
   	{
   		printf("section%d: %s %d %d\n", i+1, sections[i].name, sections[i].type, sections[i].size);
   	}
	
	free(sections);
	close(file);
	return;   
}



void list_recursive(char* path, int size, int recursive, char* permissions)
{
	 DIR* dir= NULL;//structura in care deschid directorul
	 struct dirent *dirEntry= NULL;//pt fiecare element din director(unde pastrez)
	 struct stat inode;//informatii despre fiecare elem din director: size, permissions
	 char path_name[500];

		dir = opendir(path);//deschid directorul si ii dau calea
		if ( dir == NULL ) 
		{
			perror ("Error opening directory");
			return;
		}
          
	while ((dirEntry=readdir(dir)) != NULL ) 
	{
		if( strcmp(dirEntry->d_name,".") != 0 && strcmp(dirEntry->d_name,"..") != 0) 
		{
			snprintf(path_name, 500, "%s/%s",path,dirEntry->d_name);//la path_name afiseaza path-ul si numele elem din director
	          
            //preluam informatii despre fisier(dimensiunea,permisiunile)
			if(lstat(path_name, &inode)==0)//in inode returnez informatiile
			{
	    		//printf("size %d",size);  
	   			if(S_ISREG(inode.st_mode) && size != -1 && inode.st_size > size) 
        			printf("%s\n",path_name);
		 		
		 		else if( permissions != NULL) 
				{
					char* modeval = malloc(sizeof(char)*9 + 1);
					
					mode_t perm = inode.st_mode;
					modeval[0] = (perm & S_IRUSR) ? 'r' : '-';
					modeval[1] = (perm & S_IWUSR) ? 'w' : '-';
					modeval[2] = (perm & S_IXUSR) ? 'x' : '-';
					modeval[3] = (perm & S_IRGRP) ? 'r' : '-';
					modeval[4] = (perm & S_IWGRP) ? 'w' : '-';
					modeval[5] = (perm & S_IXGRP) ? 'x' : '-';
					modeval[6] = (perm & S_IROTH) ? 'r' : '-';
					modeval[7] = (perm & S_IWOTH) ? 'w' : '-';
					modeval[8] = (perm & S_IXOTH) ? 'x' : '-';
					modeval[9] = '\0';//sa stie unde se termina cuv
					if(strcmp(modeval,permissions)==0)
					printf("%s\n",path_name);
					free(modeval);
				}
				else if(size == -1 && permissions == NULL)
				{
					printf("%s\n", path_name);
				}
				
	   			if(S_ISDIR(inode.st_mode) && recursive !=0)
					list_recursive(path_name, size, recursive, permissions);	
		
	   		}
		}
	}

	closedir(dir);
}


int main(int argc, char **argv){
    
    if(argc >= 2)
    {
        if(strcmp(argv[1], "variant") == 0)
        {
            printf("36686\n");
        }
		
		if(strncmp(argv[1], "list",4) == 0)
		{
			int recursive=0,path=0,size=0,permissions=0;//daca le gasesc sunt >0
			
			for(int i=1; i < argc; i++) 
			{				
			 	if(strncmp(argv[i],"recursive",9) == 0)
					recursive = i;   
		   	  	if(strncmp(argv[i],"path=",5) == 0)
					path = i;  
			   if(strncmp(argv[i],"size_greater=",13) == 0)
					size = i; 
			   if(strncmp(argv[i],"permissions=",12) == 0)
					permissions = i; 
		 	}
			  
			printf("SUCCESS\n");
			
			if(size != 0) //daca ramanea pe 0 insemna ca nu l-am gasit
			{
				int sizeValue;
				sscanf(argv[size]+13,"%d",&sizeValue);//conversie pt valoarea marimii de la char la int
				list_recursive(argv[path]+5,sizeValue,recursive, NULL) ;
			}	
			else if(permissions != 0) 
			{
				list_recursive(argv[path]+5,-1,recursive, argv[permissions]+12) ;
			}
			else
				list_recursive(argv[path]+5,-1,recursive, NULL) ;
			
		   
         }

		if(strncmp(argv[1],"parse",5) == 0)
		{
			checkSF(argv[2]+5);//argv[2]-> path=
		}
		
		if(strncmp(argv[1], "extract", 7) == 0)
		{
			int path = 0, section = 0, line = 0;
			
			for(int i = 0; i < argc; i++)
			{
				if(strncmp(argv[i], "path=", 5) == 0)
					path = i;
				
				if(strncmp(argv[i], "section=", 8) == 0)
					section = i;
				
				if(strncmp(argv[i], "line=", 5) == 0)
					line = i;		
			}
			
			//atoi converteste de la char la numar (int)
			//tot ce se primeste in linia de comanda avem ca si text
			//daca vrem sa scoatem numere, trebuie sa facem conversie
			extract(argv[path]+5, atoi(argv[section]+8), atoi(argv[line]+5));
		
		}
		
		if(strncmp(argv[1],"findall",7) == 0)
		{
			printf("SUCCESS\n");
			findAll(argv[2]+5);
			
		}

			
    return 0;
	}
}
