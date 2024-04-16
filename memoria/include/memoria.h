#ifndef MEMORIA_H_
#define MEMORIA_H_
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

int socket_server_MEMORIA;
int socket_cliente_MODULO;
int handshake;
//No va a quedar para siempre
int handshakeDeCPU = 0;
int handshakeDeKERNEL = 1;
int handshakeDeIO = 2;
//int32_t  * conexionExitosaCPU;
//int32_t  * noCoincideHandshakeCPU;
//(* conexionExitosaCPU) = 0;
//(* noCoincideHandshakeCPU) = -1;

char* PUERTO_ESCUCHA;
char* TAM_MEMORIA;
char* TAM_PAGINA;
char* PATH_INSTRUCCIONES;
char* RETARDO_RESPUESTA;

t_log* logger_memoria;
t_config* config_memoria;

int iniciar_conexiones();
void iterator(char* value);
void terminar_programa();
void leer_configuraciones();

#endif