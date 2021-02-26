#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/mman.h>

int readPipe = -1, writePipe = -1;
char request[100];
int size;
void* sharedMemory;
int sharedMemorySize;
int fileDescriptor;
void *mappedFile;
int mappedFileSize;

void pingpong()
{
	//dimensiunea cuvintelor scrise
	int dim = 4;
	int number = 36686;
	write(writePipe, &dim, 1);
	write(writePipe, "PING", 4);
	write(writePipe, &dim, 1);
	write(writePipe, "PONG", 4);
	write(writePipe, &number, 4);
}

void createSharedMemory()
{
	const char* name = "/8axIyIA9";
	//int dim = 7;
	read(readPipe, &sharedMemorySize, 4);
	//creaza spatiul pentru memoria partajata
	int id = shm_open(name, O_CREAT| O_RDWR, 0664);
	//truncheaza zona de memorie la dimensiunea ceruta
	ftruncate(id, sharedMemorySize);	
	//mapeaza spatiul in memorie
	sharedMemory = mmap(0,sharedMemorySize, PROT_WRITE, MAP_SHARED, id, 0);
         if(mmap == (void*) -1)
    {
             int dim =5;
		write(writePipe,&size,1);
		write(writePipe, request, size);
		write(writePipe, &dim,1);
		write(writePipe, "ERROR", 5);

    }
    else
	{
	int dim=7;
	write(writePipe, &size, 1);
	write(writePipe, request, size);
	write(writePipe, &dim, 1);
	write(writePipe, "SUCCESS", 7);	
	}

}

void writeToSharedMemory()
{
	int offset, value;
	read(readPipe, &offset, 4);
	read(readPipe, &value, 4);
         //verific sa nu iasa din zona de memorie
	if( offset > sharedMemorySize || offset + 4 > sharedMemorySize)
	{
		int dim = 5;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "ERROR", 5);
	}
	else
	{
		int dim = 7;
		//scriu in zona de memorie partajata cu functia memcpy
		memcpy((char*)sharedMemory + offset, &value, 4);
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "SUCCESS", 7);	
	}
}

void mapFileToSharedMemory()
{
	int dim;
	//am citit dimensiunea fisierului din pipe
	read(readPipe, &dim, 1);
	char* fileName = (char*)malloc((dim+1)*sizeof(char));
	//am citit numele fisierului
	read(readPipe, fileName, dim);
	//am deschis fisierul pt citire
	fileDescriptor = open(fileName, O_RDONLY);
	//mut cursorul la finalul fisierului pt a determina dimensiunea
	mappedFileSize = lseek(fileDescriptor, 0, SEEK_END);
	//ma mut inapoi la inceputul fisierului
	lseek(fileDescriptor, 0, SEEK_SET);
	//mapez fisierulul in memorie
	mappedFile = mmap(NULL, mappedFileSize, PROT_READ, MAP_PRIVATE, fileDescriptor, 0);
	
	if(fileDescriptor == 0 || mappedFile == (void*)-1)
	{
		int dim = 5;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "ERROR", 5);
	}
	else
	{
		int dim = 7;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "SUCCESS", 7);
	}
}

void readFromFileOffset()
{
	int offset, no_of_bytes;
        //am citit din pipe offset-ul si nr de bytes
	read(readPipe, &offset, 4);
	read(readPipe, &no_of_bytes, 4);
	//verific daca offset-ul/nr de bytes depasesc dimensiunea fisierului
	if(offset > mappedFileSize || offset + no_of_bytes > mappedFileSize)
	{
		int dim = 5;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "ERROR", 5);
	}
	else
	{
                //copiez in zona de memorie partajata, nr de bytes din fisier la offset-ul dat
		memcpy(sharedMemory, mappedFile + offset, no_of_bytes);
		int dim = 7;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "SUCCESS", 7);
	}
}

void readFromFileSection()
{
	int section_no, offset, no_of_bytes;
	int headerSize, numberOfSections, sectionBegin, sectionSize;
	int sectionEnd;
	//citesc nr sectiunii, offset-ul si nr de bytes din pipe
	read(readPipe, &section_no, 4);
	read(readPipe, &offset, 4);
	read(readPipe, &no_of_bytes, 4);
	//citesc dim header-ului 
	memcpy(&headerSize, mappedFile+mappedFileSize-6,2);
	//citesc nr de sectiuni
	memcpy(&numberOfSections, mappedFile+mappedFileSize-headerSize+2,1);
	//citesc offset-ul sectiunii cautate
	memcpy(&sectionBegin, mappedFile+mappedFileSize-headerSize+3+21*(section_no-1)+13,4);
	//citesc dimensiunea sectiunii cautate
	memcpy(&sectionSize, mappedFile+mappedFileSize-headerSize+3+21*(section_no-1)+17,4);
	//calculez offset-ul de terminare a sectiunii
	sectionEnd = sectionBegin+sectionSize;
	
	//daca numarul sectiunii e prea mare sau nr de bytes depasesc dimensiunea sectiunii
	if(numberOfSections < section_no || sectionBegin+offset+ no_of_bytes > sectionEnd)
	{
		int dim = 5;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "ERROR", 5);
	}
	else
	{
		int dim = 7;
		//copiez din fisierul partajat in memorie nr de bytes in zona de mem partajata
		memcpy(sharedMemory, mappedFile+sectionBegin+offset, no_of_bytes);
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "SUCCESS", 7);
	}
}

void readFromLogicalSpace()
{
	int logical_offset, no_of_bytes;
	int headerSize = 0, numberOfSections = 0;
	int currentLogicalOffset = 0;
	int currentFileOffset = 0;
	int index = 0;
	
	//citesc parametrii comenzii
	read(readPipe, &logical_offset, 4);
	read(readPipe, &no_of_bytes, 4);
	memcpy(&headerSize, mappedFile+mappedFileSize-6,2);
	memcpy(&numberOfSections, mappedFile+mappedFileSize-headerSize+2,1);
	
	//retine date despre fiecare sectiune
	int sectionsOffset[numberOfSections];
	int sectionsSize[numberOfSections];
	int sectionsLogicalOffset[numberOfSections];
	//ma pozitionez in fisier in hearder unde sunt informatiile despre sectiuni
	currentFileOffset = mappedFileSize-headerSize+3;
	sectionsLogicalOffset[0] = 0;
	
	for(int i = 0; i < numberOfSections; i++)
	{
		memcpy(&sectionsOffset[i],mappedFile+currentFileOffset+13,4);
		memcpy(&sectionsSize[i],mappedFile+currentFileOffset+17,4);
		currentFileOffset += 21;
		
		if( sectionsSize[i] % 5120 != 0) 
			//offset-ul curent este nr de blocuri de mem 5120 folosite * 5120
			currentLogicalOffset += (sectionsSize[i]/5120+1)*5120;
		else
			currentLogicalOffset += (sectionsSize[i]/5120)*5120;	 
		//cat timp n-am ajuns la finalul vectorului, calculez offset-ul calculat mai sus
		if( i < numberOfSections - 1)
			sectionsLogicalOffset[i+1] = currentLogicalOffset;
		//la care sectiune am ajuns
		index++;
		
		if(currentLogicalOffset > logical_offset)
			break;
	}
	//ca sa nu depasesc vectorul
	if(index == 0)
		index = 1;
	//ma pozitionez in offset-ul normal al sectiunii, nu cel logic
	//x=offset-ul de unde trebuie sa citesc
	int x = sectionsOffset[index-1] + logical_offset - sectionsLogicalOffset[index-1];
	//daca nr de bytes depasesc offset-ul de final al sectiunii
	if( x +no_of_bytes > sectionsOffset[index-1] + sectionsSize[index-1] )
	{
		int dim = 5;
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "ERROR", 5);
	}
	else
	{
		int dim = 7;
		memcpy(sharedMemory, mappedFile+x, no_of_bytes);
		write(writePipe, &size, 1);
		write(writePipe, &request, size);
		write(writePipe, &dim, 1);
		write(writePipe, "SUCCESS", 7);
	}
}

int main()
{
         //creates the response pipe
	if(mkfifo("RESP_PIPE_36686", 0600) < 0)
	{
		printf("ERROR\ncannot create the reponse pipe\n");
		exit(1);
	}
	readPipe = open("REQ_PIPE_36686", O_RDONLY);
	if( readPipe < 0)
	{
		printf("ERROR\ncannot open the request pipe\n");
		exit(1);
	}
	writePipe = open("RESP_PIPE_36686", O_WRONLY);
	if( writePipe < 0)
	{
		printf("ERROR\ncannot open the response pipe\n");
		exit(1);
	}
       
	int dim = 7;
	write(writePipe, &dim, 1);
	write(writePipe, "CONNECT", 7);
	
	if(readPipe != -1)
	{
		printf("SUCCESS\n");
	}
	
        // while true
	while(0==0)
	{
                //citesc dimensiunea cuvantului
		read(readPipe, &size, 1);
                //citesc numele
		read(readPipe, request, size);
		if(strncmp(request, "PING", 4) == 0)
			pingpong();
			
		if(strncmp(request, "CREATE_SHM", 10) == 0)
			createSharedMemory();
			
		if(strncmp(request, "WRITE_TO_SHM",12) == 0)
			writeToSharedMemory();
		
		if(strncmp(request, "MAP_FILE", 8) == 0)
			mapFileToSharedMemory();
			
		if(strncmp(request, "READ_FROM_FILE_OFFSET", 21) == 0)
			readFromFileOffset();					
			
		if(strncmp(request, "READ_FROM_FILE_SECTION", 22) == 0)
			readFromFileSection();	
	
		if(strncmp(request, "READ_FROM_LOGICAL_SPACE_OFFSET", 30) == 0)
			readFromLogicalSpace();		
			
		if(strncmp(request, "EXIT", 4 ) == 0)
			{
				close(fileDescriptor);
				shm_unlink(sharedMemory);
				close(readPipe);
				close(writePipe);
				break;
			}
		}
	return 0;
}

