#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#define KYEL	"\x1B[33m"
#define KRED	"\x1B[31m"
#define KGRN	"\x1B[32m"
#define RESET	"\x1B[0m"

#define SIZE_BUFFER 250

using namespace std;

int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

struct datos{
	const char* puertoPadre;
	u_short puerto;
	const char* codigoOperacion;
};

struct datosCliente{
	
	u_short puerto;
	int *idCanal;
};

char* getIP(){
	
	char hostbuffer[256];
	char *IPbuffer;
	struct hostent *host_entry;
	int hostname;
	
	hostname = gethostname(hostbuffer,sizeof(hostbuffer));
	
	host_entry = gethostbyname(hostbuffer);
	
	IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
	
	return IPbuffer;
}



int puerto,idSocket;
string ip,operacion;
struct sockaddr_in servidor;

int realizarOperacionS(const char* codigo,u_short puerto);
void realizarOperacion(const char* codigo);
void correHiloNuevo(int clienteSocket);
void * peticionCliente( void* );
void * interfaz( void* );
void * conectarPeer( void* );
void * acceptCiclo(void* );

int main( int argc, char const *argv[] ){
    
	int primero,reuse,aux;
	
	idSocket = socket(AF_INET,SOCK_STREAM,0);
	
	if(idSocket == -1){
		
		perror("Error al crear el socket");
		exit(EXIT_FAILURE);	
	}
	
	memset(&servidor,sizeof(servidor),0);
	
	if(argc < 1){
		
		perror("Error falta un argumento");
		exit(EXIT_FAILURE);
	}
	
	primero = atoi( argv[1] );
	
	if(primero == 1){
		
		int as = atoi( argv[2] );
		servidor.sin_family = AF_INET;
		servidor.sin_port = htons( as );
		servidor.sin_addr.s_addr = INADDR_ANY;
			
	}else{
		
		ifstream nodos;
		nodos.open("nodos.txt");
		servidor.sin_family = AF_INET;
		nodos >> aux;
		
		if(aux == 0){
			
			int pAux;
			nodos >> pAux;
			servidor.sin_port = htons(pAux);
			servidor.sin_addr.s_addr = INADDR_ANY;
			nodos.close();	
			ip = to_string(servidor.sin_addr.s_addr);
			puerto = servidor.sin_port;
			ip = to_string(servidor.sin_addr.s_addr);
			puerto = ntohs(servidor.sin_port);
		}else{
			
			perror("Error al leer el archivo nodos.dat");
			exit(EXIT_FAILURE);
		}
		
	}
	
	reuse = 1;
	if(setsockopt(idSocket,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int))<0){
		
		perror("Error al realizar el SO_REUSEADDR");
		exit(EXIT_FAILURE);	
	}
	
	if(setsockopt(idSocket,SOL_SOCKET,SO_REUSEPORT,&reuse,sizeof(int))<0){
		
		perror("Error al realizar el SO_REUSEPORT");
		exit(EXIT_FAILURE);	
	}
	
	if(bind(idSocket,(struct sockaddr *)&servidor,sizeof(servidor))==-1){
		
		perror("Error al realizar el bind");
		exit(EXIT_FAILURE);		
	}
	
	if(primero == 1){
		
		ofstream nodos;
		nodos.open("nodos.txt");
		struct sockaddr_in sin;
		socklen_t len = sizeof(sin);
		
		if (getsockname(idSocket, (struct sockaddr *)&sin, &len) == -1)
	    	perror("getsockname");
		else
			puerto = ntohs(sin.sin_port);
		
		ip = to_string(servidor.sin_addr.s_addr);
		puerto = ntohs(servidor.sin_port);
		nodos << "0 " << puerto << " " << ip << endl;
		nodos.close();
	}
	
	if(listen(idSocket,10)==-1){
		
		perror("Error al realizar el listen");
		exit(EXIT_FAILURE);
	}
	
	pthread_t interfazHilo;
	pthread_create(&interfazHilo,NULL,interfaz,NULL);
	operacion = "x";
	
	pthread_t acceptHilo;
	pthread_create(&acceptHilo,NULL,acceptCiclo,NULL);
	
	pthread_join(interfazHilo,NULL);
	pthread_join(acceptHilo,NULL);
	
	return 0;
}
vector<string> filesNew;
void * interfaz( void* ){
	
	
	string atributo;
	system("clear");
	printf("%s----------------------------------------------------%s\n",KYEL,RESET);
	printf("\t%sPEER%s---------->%sIP:%s LOCAL_IP %sPUERTO:%s %d\n",KRED,RESET,KYEL,RESET,KYEL,RESET,puerto);
	printf("%s----------------------------------------------------%s\n",KYEL,RESET);
	
	while(operacion.compare("salir")!=0){
				
			if(operacion.compare("status")==0){
				
				string a = "0";
				realizarOperacion(a.c_str());
				
			}else if(operacion.compare("actualiza")==0){
				
				printf("Actualiza:\n Nodos \n Lista \n");
				printf("> ");
				cin >> atributo;
				if(atributo.compare("Nodos")==0){
					
					//actualiza la lista de nodos
				}else if(atributo.compare("Lista")==0){
					
					ofstream archivoLista;
					archivoLista.open("lista.txt");
					
					string dir = string("archivos");
    				
					filesNew = vector<string>();
					
    				getdir(dir,filesNew);

    				for (unsigned int i = 0;i < filesNew.size()-2;i++) {
        				archivoLista << filesNew[i] << endl;
    				}
					archivoLista.close();
					string a = "1";
					realizarOperacion(a.c_str());	
				}
			}else if(operacion.compare("descargar")==0){
			
				//descargar archivo
		}
		printf("> ");
		cin >> operacion;
	}
	
	exit(0);
	pthread_exit(NULL);
}

void realizarOperacion(const char* codigo){
	
	string basura;
	ifstream nodos;
	int numeroNodo;
	nodos.open("nodos.txt");
	nodos >> numeroNodo;
	string puertoTemp;			
	while(nodos){
					
		struct datos *datosPeer = (struct datos *) malloc(sizeof(struct datos));
		if(numeroNodo!= 0){
			datosPeer->puertoPadre = puertoTemp.c_str();
			nodos >> datosPeer->puerto;
			nodos >> basura;
			datosPeer->codigoOperacion = codigo;
			pthread_t conectarHilo;
			pthread_create(&conectarHilo, NULL, conectarPeer,datosPeer);
					
		}else{
						
			nodos >> puertoTemp;
			nodos >> basura;
		}
					
			nodos >> numeroNodo;
	}
    nodos.close();
}

int realizarOperacionS(const char* codigo,u_short puerto){
	
	
		struct datos *datosPeer = (struct datos *) malloc(sizeof(struct datos));
			datosPeer->puertoPadre = to_string(puerto).c_str();
			datosPeer->puerto = puerto;
			datosPeer->codigoOperacion = codigo;
			pthread_t conectarHilo;
			pthread_create(&conectarHilo, NULL, conectarPeer,datosPeer);
	int *idSo;
	pthread_join(conectarHilo,(void **)&idSo);
	return *idSo;		
}

void * conectarPeer( void* peerH){
	
	const char* codigoOperacion = ((struct datos*)peerH)->codigoOperacion;
	struct sockaddr_in peer;
	memset(&peer,sizeof(peer),0);
	peer.sin_family = AF_INET;
	peer.sin_port = htons(((struct datos*)peerH)->puerto);
	peer.sin_addr.s_addr = INADDR_ANY;
	int *idSocketPeer = (int*)malloc(sizeof(int));
	*idSocketPeer = socket(AF_INET,SOCK_STREAM,0);
	char * baf;
	const char* puertoPad = ((struct datos*)peerH)->puertoPadre;
	if(*idSocketPeer == -1){
		
		printf("Error al crear el socket\n");
		pthread_exit(NULL);	
	}
	
	if(connect(*idSocketPeer,(struct sockaddr *)&peer,sizeof(peer))==-1){
		
		printf("%sConexion fallida peer: Ip: LOCAL_IP Puerto: %d %s\n",KRED,ntohs(peer.sin_port),RESET);
	}else{
		
		printf("%sConexion exitosa peer: Ip: LOCAL_IP Puerto: %d %s\n",KGRN,ntohs(peer.sin_port),RESET);
		send(*idSocketPeer,codigoOperacion,strlen(codigoOperacion),0);
        recv(*idSocketPeer,baf,SIZE_BUFFER,0);
		if(send(*idSocketPeer,puertoPad,strlen(puertoPad),0)==-1)
            cout << "Error send" << endl;
		int codigo = atoi(codigoOperacion);
		
		if(codigo == 1){
            
            int input_file = open("lista.txt", O_RDONLY );
			while (1) {
				char buffer[SIZE_BUFFER];
    			int bytes_read = read(input_file, buffer, sizeof(buffer));
    			if (bytes_read == 0) 
					break;
    			if (bytes_read < 0) {
        
    			}

	    		void *p = buffer;
    			while (bytes_read > 0) {
        			int bytes_written = write(*idSocketPeer, p, bytes_read);
        			if (bytes_written <= 0) {
            		    cout << "que pedo" << endl;
        			}
        		    bytes_read -= bytes_written;
        	        p += bytes_written;
    			}
                if(SIZE_BUFFER>bytes_read){
                    break;   
                }
			}
            close(input_file);
            
			
		}else if(codigo == 2){
			
		}
	}
	
	pthread_exit(idSocketPeer);
}

void * acceptCiclo(void* ){
	
	struct sockaddr_in cliente;
	socklen_t tamanoCliente = (socklen_t) sizeof(cliente);
	while(true){
		int *idCanal = (int*) malloc(sizeof(int));
		*idCanal = accept(idSocket,(struct sockaddr *)&cliente,&tamanoCliente);
		if(*idCanal == -1){
			
			perror("Error al realizar el accept");
			exit(-1);
			
		}else{
			
			pthread_t hilo;
			if(pthread_create(&hilo, NULL, peticionCliente,idCanal)==-1){
				perror("Phtread_Create");
				exit(EXIT_FAILURE);
			}
		}
	}
	
	pthread_exit(NULL);
}

void * peticionCliente( void* datosC){
	
	char buffer[SIZE_BUFFER];
	int *idCanal = (int *) datosC;
	int caracteresRecibidos,enviado;
	int codigo=0,port=0;
	
	char mensaje[SIZE_BUFFER];
		
	caracteresRecibidos = recv(*idCanal,buffer,SIZE_BUFFER,0);
	buffer[caracteresRecibidos] = '\0';
	codigo = atoi(buffer);
	
    send(*idCanal,"Que pedo",SIZE_BUFFER,0);
    
    caracteresRecibidos = recv(*idCanal,mensaje,SIZE_BUFFER,0);
	mensaje[caracteresRecibidos] = '\0';
	port = atoi(mensaje);
    
	if(codigo==1){
            
            string an = to_string(port);
            an.append(".txt");
            int input_file = open(an.c_str(), O_WRONLY |O_CREAT | O_TRUNC,0666);
			while (1) {
				char bufferr[SIZE_BUFFER];
    			int bytes_read = read(*idCanal, bufferr, sizeof(bufferr));
    			if (bytes_read == 0) 
					break;
    			if (bytes_read < 0) {
                    perror("Read ");
                    exit(-1);
    			}

	    		void *p = bufferr;
    			while (bytes_read > 0) {
        			int bytes_written = write(input_file, p, bytes_read);
        			if (bytes_written <= 0) {
            		
        			}
        		    bytes_read -= bytes_written;
        		    p += bytes_written;
    			}
                if(SIZE_BUFFER>bytes_read){
                    break;   
                }
                //
			}
            close(input_file);
			close(*idCanal);
		
	}else if(codigo==2){ 
			
	}else if(codigo == 0){
		
	}
	
	pthread_exit(NULL);
}

