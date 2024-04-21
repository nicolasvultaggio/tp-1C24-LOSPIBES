#include <../../utils/include/protocolo.h>

//Cliente
void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar , bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	
}

void enviar_mensaje_de_exito(int socket, char* mensaje) {
    send(socket, mensaje, strlen(mensaje), 0);
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

//Servidor

t_list* recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}


void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

int recibir_operacion(int socket_cliente, t_log * unLogger , char * elQueManda)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		log_info(unLogger,"No se pudo recibir operación de : %s",elQueManda);
		return -1;
	}
}


void enviar_operacion(int socket_conexion, op_code numero)
{
	int buffer = numero;
	send(socket_conexion, &buffer, sizeof(int) ,0);
	
}


void recibir_mensaje(t_log* loggerServidor ,int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(loggerServidor, "Me llego el mensaje %s", buffer);
	free(buffer);
}

void atender_conexion(void * args){
	int clave = 1 ;
	args_atendedor *atender_args = (args_atendedor *)args;
    int fd_conexion = atender_args->fd_conexion;
    t_log *logger_atendedor = atender_args->logger_atendedor;
    char *cliente = atender_args->cliente;

	while (clave) {
		int cod_op = recibir_operacion(fd_conexion,logger_atendedor,cliente);
		switch (cod_op) {
		case MENSAJE:
			//
			break;
		case PAQUETE:
			//
			break;
		case -1:
			log_error(logger_atendedor, "Desconexión de %s",cliente);
			clave = 0;
            break;
		default:
			log_info(logger_atendedor,"Operacion desconocida de %s",cliente);
			break;
	}
    }
	free(args);
	free(atender_args);
	pthread_exit(NULL);
}

args_atendedor* crear_args(int un_fd, t_log * un_logger, char * un_cliente){
	args_atendedor* args = (args_atendedor*) malloc(sizeof(args_atendedor));
	args -> fd_conexion = un_fd;
	args -> logger_atendedor = un_logger;
	args -> cliente = un_cliente;
	return args;
}

