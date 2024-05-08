//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************
//                       
//  Clase que atiende una petición FTP.
// 
//****************************************************************************
//                          COMANDOS IMPLEMENTADOS
//
//      Línea 136          USER        nombre de usuario
//      "   " 153          PASS        contraseña
//      "   " 170          SYST        información del sistema
//      "   " 182          PWD         ubicación actual
//      "   " 198          CWD         cambiar de directorio
//      "   " 223          RNFR        renombrar archivo
//      "   " 237          RNTO        renombrar archivo
//      "   " 254          DELE        eliminar archivo
//      "   " 271          MKD         crear directorio
//      "   " 288          RMD         eliminar directorio
//      "   " 305          LIST        mostrar archivos
//      "   " 348          TYPE        tipo de conexión
//      "   " 372          PORT        modo activo
//      "   " 395          PASV        modo pasivo
//      "   " 438          RETR        descargar archivo del servidor
//      "   " 480          STORE       subir archivo al servidor
//      "   " 523          QUIT        salir
//****************************************************************************


#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"
#include "FTPServer.h"





ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");
    if (fd == NULL){
	std::cout << "Connection closed" << std::endl;

	fclose(fd);
	close(control_socket);
	ok = false;
	return ;
    }
    
    ok = true;
    data_socket = -1;
    parar = false;
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP(uint32_t address, uint16_t port){
    struct sockaddr_in sin;
    struct hostent *hent;
    int s;
    memset(&sin, 0, sizeof(sin)); sin.sin_family = AF_INET; sin.sin_port = htons (port);
    memcpy(&sin.sin_addr, &address, sizeof(address));
    s = socket (AF_INET, SOCK_STREAM, 0);
    if(s < 0)
    errexit("No se puede crear el socket: %s\n", strerror(errno));
    if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("No se puede conectar con %d: %s\b", address, strerror(errno));
    return s; // You must return the socket descriptor.

}

void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
} 

#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {

      fscanf(fd, "%s", command);
    
        if (COMMAND("USER")) {
            printf("Nueva conexión.\n");
            fscanf(fd, "%s", arg);
            if(strcmp(arg, "hugo") == 0){
                fprintf(fd, "331 User name ok, need password\n"); fflush(fd);
                printf("User: %s logged in.\n", arg);
            }else{
                fprintf(fd, "530 Not logged in\n");
                fflush(fd);
                printf("User: %s is not registered.", arg);
                parar=!parar;
            }
        }else if (COMMAND("PASS")) {
            fscanf(fd, "%s", arg);
            if(strcmp(arg, "12345") == 0) {
                fprintf(fd, "230 User logged in, proceed\n"); fflush(fd);
                printf("Usuario autenticado.\n");
            }
            else{
                fprintf(fd, "530 Not logged in\n");
                fflush(fd);
                printf("Intento fallido de autenticación.\n");
            }
        }else if (COMMAND("PASS")) {
            fscanf(fd, "%s", arg);
            if(strcmp(arg,"1234") == 0){
                fprintf(fd, "230 User logged in\n");
            }
            else{
                fprintf(fd, "530 Not logged in.\n");
                parar = true;
            }
        } else if (COMMAND("PORT")) {
            parar = false;
                
            unsigned int ip[4];
            unsigned int port[2];

            fscanf(fd, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &port[0], &port[1]);

            uint32_t ip_addr = ip[3]<<24 | ip[2]<<16 | ip[1]<<8 | ip[0];
            uint16_t port_v = port[0] << 8 | port[1];
            
            data_socket = connect_TCP(ip_addr,port_v);

        fprintf(fd, "200 Okey\n");
        } else if (COMMAND("PASV")) { 
            int s = define_socket_TCP(0);

            struct sockaddr_in addr;
            socklen_t len = sizeof (addr);

            int result = getsockname(s, (struct sockaddr*)&addr, &len);

            uint16_t pp = addr.sin_port;
            int p1 = pp >> 8;
            int p2 = pp & 0xFF;

            if (result < 0 ){
                fprintf(fd, "421 Service not available, closing control connection.\n"); 
                fflush(fd);
                return;
            }
            
            fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d)\n", p2, p1);
            len = sizeof(addr);
            fflush(fd);
            result = accept(s, (struct sockaddr*)&addr, &len);

            if(result< 0 ) {
                fprintf(fd, "421 Service not available, closing control connection.\n"); fflush(fd);
                return;
            }
            data_socket = result;
        } else if (COMMAND("STOR") ) {
            fscanf(fd, "%s", arg);
            fprintf(fd, "150 File status okay; about to open data connection.\n"); 
            fflush(fd);
            FILE *f= fopen(arg, "wb+");
            if(f == NULL){
                fprintf(fd, "425 Can't open data connection.\n");
                fflush(fd);
                close(data_socket);
                break;
            }
            fprintf(fd, "125 Data connection already open; transfer starting.\n"); 
            char *buffer[MAX_BUFF];
            fflush(fd);
            while(1) {
            
                int b = recv(data_socket, buffer, MAX_BUFF, 0); fwrite(buffer, 1, b, f);
                if (b = MAX_BUFF) {
                    break;
                }
            }
            fprintf(fd, "226 Closing data connection.\n");
            fflush(fd);
            fclose(f);
            close(data_socket);
            fflush(fd);
        } else if (COMMAND("LIST")) {
            fprintf(fd, "150 Here comes the directory listing.\n");
            fflush(fd);

            if(data_socket < 0) {
                fprintf(fd, "425 Can't open data connection.\n");
                fflush(fd);
                return;
            }

            DIR *d;
            struct dirent *dir;
            d = opendir("."); // Abre el directorio actual
            if (d) {
                while ((dir = readdir(d)) != NULL) {
                    // Ignora los directorios '.' y '..'
                    if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                        // Envía el nombre del archivo por el socket de datos
                        dprintf(data_socket, "%s\r\n", dir->d_name);
                    }
                }
                closedir(d); // Cierra el directorio
                fprintf(fd, "226 Directory send OK.\n");
            } else {
                fprintf(fd, "550 Failed to list directory.\n");
            }
            fflush(fd);
            close(data_socket); // Cierra el socket de datos
        }  else if (COMMAND("RETR")) {
            fscanf(fd, "%s", arg);
            FILE *f= fopen(arg, "r");
            if(f!= NULL)
            {
                fprintf(fd, "125 Data connection already open; transfer starting.\n");
                fflush(fd);
                char *buffer[MAX_BUFF];
                while(1) {
                    int b = fread(buffer, 1, MAX_BUFF, f);
                    send(data_socket, buffer, b, 0);
                    if (b = MAX_BUFF) {
                        break;
                    }
                }
                fprintf(fd, "226 Closing data connection.\n"); 
                fflush(fd);

                fclose(f);
                close(data_socket);
            } else{
                fprintf(fd, "425 Can't open data connection.\n");
                fflush(fd);
                close(data_socket);
            }
        } else if (COMMAND("SYST")) {
            fprintf(fd, "215 UNIX Type: L8.\n");   
        } else if (COMMAND("TYPE")) {
            fscanf(fd, "%s", arg);
            fprintf(fd, "200 OK\n");   
        } else if (COMMAND("QUIT")) {
            fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
            close(data_socket);	
            parar=true;
            break;
        } else  {
            fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
            printf("Comando : %s %s\n", command, arg);
            printf("Error interno del servidor\n");
        }
    }
    
    fclose(fd);

    
    return;
  
};
