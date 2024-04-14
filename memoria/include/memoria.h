#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>


int socket_server_MEMORIA;
int socket_cliente_MODULO;

char* PUERTO_ESCUCHA;
char* TAM_MEMORIA;
char* TAM_PAGINA;
char* PATH_INSTRUCCIONES;
char* RETARDO_RESPUESTA;

t_log* logger;
t_config* config;

bool iniciar_conexiones();
void terminar_programa();
void leer_configuraciones();