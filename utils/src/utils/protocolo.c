#include <../../utils/include/protocolo.h>

/* ATENDDER (NO SE USAN MAS)*/
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
	
	pthread_exit(NULL);
}

args_atendedor* crear_args(int un_fd, t_log * un_logger, char * un_cliente){
	args_atendedor* args = (args_atendedor*) malloc(sizeof(args_atendedor));
	args -> fd_conexion = un_fd;
	args -> logger_atendedor = un_logger;
	args -> cliente = un_cliente;
	return args;
}

/*--------------------------*/

void * pop_con_mutex(t_list* lista, pthread_mutex_t* mutex){
	pthread_mutex_lock(mutex);
	void* elemento = list_remove(lista, 0); // obtiene el PRIMER ELEMENTO de esta cola
	pthread_mutex_unlock(mutex);
	return elemento;
}

void push_con_mutex(t_list* lista, void * elemento ,pthread_mutex_t* mutex){
  	pthread_mutex_lock(mutex);
    list_add(lista, elemento);
	pthread_mutex_unlock(mutex);
    return;
}

void empaquetar_pcb(t_paquete paquete, pcb* PCB){

	agregar_a_paquete(paquete, &(PCB->PID), sizeof(int));
	agregar_a_paquete(paquete, &(PCB->PC), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(PCB->QUANTUM), sizeof(int));
	agregar_a_paquete(paquete, &(PCB->estado), sizeof(estadosDeLosProcesos));
	agregar_a_paquete(paquete, &(PCB->registros.AX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(PCB->registros.BX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(PCB->registros.CX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(PCB->registros.DX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(PCB->registros.EAX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(PCB->registros.EBX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(PCB->registros.ECX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(PCB->registros.EDX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(PCB->registros.SI), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(PCB->registros.DI), sizeof(uint32_t));

}



/* FUNCIONES DE PAQUETE */
t_paquete* crear_paquete(op_code OPERACION)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = OPERACION;
	crear_buffer(paquete);
	return paquete;
}

void crear_buffer(t_paquete* paquete){
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
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

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

/* FUNCIONES DE ENVIO (SENDS) */
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

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void enviar_mensaje_de_exito(int socket, char* mensaje) {
    send(socket, mensaje, strlen(mensaje), 0);
}

void enviar_operacion(int socket_conexion, op_code numero)
{
	int buffer = numero;
	send(socket_conexion, &buffer, sizeof(int) ,0);
	
}

void snd_handshake(int fd_socket_cliente){
	t_paquete* paquete = crear_paquete(PAQUETE);
	int ok = 1;
	agregar_a_paquete(paquete, &ok, sizeof(int));
	enviar_paquete(paquete, fd_socket_cliente);
	eliminar_paquete(paquete);
}

void enviar_solicitar_instruccion(int fd, int pid, int program_counter){
	
	t_paquete* paquete = crear_paquete(SOLICITAR_INSTRUCCION);
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete, &program_counter, sizeof(int));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}

void enviar_datos_proceso(char* path, int pid, int fd_conexion){ 
	t_paquete* paquete_de_datos_proceso = crear_paquete(DATOS_PROCESO);

	agregar_a_paquete(paquete_de_datos_proceso, &pid, sizeof(int));
	agregar_a_paquete(paquete_de_datos_proceso, path, strlen(path) + 1);

	enviar_paquete(paquete_de_datos_proceso, fd_conexion);
	eliminar_paquete(paquete_de_datos_proceso);
}

void enviar_solicitud_de_dormir_interfaz_generica(int fd_cpu_dispatch, char* interfaz, char* unidad_de_tiempo){
	t_paquete* paquete = crear_paquete(SOLICITAR_IO_GEN_SLEEP);
	agregar_a_paquete(paquete, &interfaz, sizeof(char));
	agregar_a_paquete(paquete, &unidad_de_tiempo, sizeof(char));
	enviar_paquete(paquete,fd_cpu_dispatch);
	eliminar_paquete(paquete);
}

void enviar_pcb(pcb* PCB, int fd_escucha_dispatch, op_code OPERACION, char* parametro1, char* parametro2, char* parametro3, char* parametro4, char* parametro5){

	t_paquete* paquete = crear_paquete(OPERACION);

	empaquetar_pcb(paquete, PCB);

	switch (OPERACION){
	
	case ESPERA:
		agregar_a_paquete(paquete, parametro1, strlen(parametro1) + 1);
		break;

	case WAIT:
		agregar_a_paquete(paquete, parametro1, strlen(parametro1) + 1);
		break;

	case SOLICITAR_IO_GEN_SLEEP:
		agregar_a_paquete(paquete, &parametro1, sizeof(char));
		agregar_a_paquete(paquete, &parametro2, sizeof(char));
		agregar_a_paquete(paquete, &parametro3, sizeof(char));
		break;

	default:
		break;
	}

	enviar_paquete(paquete, fd_escucha_dispatch);
	eliminar_paquete(paquete);

}

void enviar_cambio_de_estado(motivo_desalojo motivo, int fd_escucha_dispatch){
	t_paquete* paquete = crear_paquete(motivo);
	agregar_a_paquete(paquete, &motivo, sizeof(motivo_desalojo));
	enviar_paquete(paquete, fd_escucha_dispatch);
	eliminar_paquete(paquete);
}

void enviar_recurso(char * recurso, int fd_escucha_dispatch, op_code OPERACION){
	t_paquete* paquete = crear_paquete(OPERACION);
	agregar_a_paquete(paquete, recurso, strlen(recurso) + 1);
	enviar_paquete(paquete, fd_escucha_dispatch);
	eliminar_paquete(paquete);
}


/* FUNNCIONES DE RECIBO (RECV) */
void recibir_mensaje(t_log* loggerServidor ,int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(loggerServidor, "Me llego el mensaje %s", buffer);
	free(buffer);
}

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

pcb* recibir_pcb(int socket){

	t_list* paquete = recibir_paquete(socket);
	pcb* PCB = malloc(sizeof(PCB));

	uint32_t* pid = list_get(paquete, 0);
	PCB->PID = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	PCB->PC = *program_counter;
	free(program_counter);

	int* quantum = list_get(paquete, 2);
	PCB->QUANTUM = *quantum;
	free(quantum);

	estadosDeLosProcesos* estado = list_get(paquete, 3);
	PCB->estado = *estado;
	free(estado);

	uint8_t* ax = list_get(paquete, 4);
	PCB->registros.AX = *ax;
	free(ax);

	uint8_t* bx = list_get(paquete, 5);
	PCB->registros.BX = *bx; 
	free(bx);

	uint8_t* cx = list_get(paquete, 6);
	PCB->registros.CX = *cx;
	free(cx);

	uint8_t* dx = list_get(paquete, 7);
	PCB->registros.DX = *dx;
	free(dx);

	uint32_t* eax = list_get(paquete, 8);
	PCB->registros.EAX = *eax;
	free(eax);

	uint32_t* ebx = list_get(paquete, 9);
	PCB->registros.EBX = *ebx;
	free(ebx);

	uint32_t* ecx = list_get(paquete, 10);
	PCB->registros.ECX = *ecx;
	free(ecx);

	uint32_t* edx = list_get(paquete, 11);
	PCB->registros.EDX = *edx;
	free(edx);

	uint32_t* si = list_get(paquete, 12);
	PCB->registros.SI = *si;
	free(si);

	uint32_t* di = list_get(paquete, 13);
	PCB->registros.DI = *di;
	free(di);

	list_destroy(paquete);

	return PCB;
}

t_linea_instruccion* recibir_proxima_instruccion(int fd_conexion){
	
	t_list* paquete = recibir_paquete(fd_conexion);
	t_linea_instruccion* instruccion_recibida = malloc(sizeof(t_linea_instruccion));

	cod_instruccion* instruccion = list_get(paquete, 0);
	instruccion_recibida->instruccion = *instruccion;
	free(instruccion);

	char* parametro1 = list_get(paquete, 1);
	instruccion_recibida->parametro1 = malloc(strlen(parametro1));
	strcpy(instruccion_recibida->parametro1, parametro1);
	free(parametro1);

	char* parametro2 = list_get(paquete, 2);
	instruccion_recibida->parametro2 = malloc(strlen(parametro2));
	strcpy(instruccion_recibida->parametro2, parametro2);
	free(parametro2);

	char* parametro3 = list_get(paquete, 3);
	instruccion_recibida->parametro3 = malloc(strlen(parametro3));
	strcpy(instruccion_recibida->parametro3, parametro3);
	free(parametro3);

	char* parametro4 = list_get(paquete, 4);
	instruccion_recibida->parametro4 = malloc(strlen(parametro4));
	strcpy(instruccion_recibida->parametro4, parametro4);
	free(parametro4);

	char* parametro5 = list_get(paquete, 5);
	instruccion_recibida->parametro5 = malloc(strlen(parametro5));
	strcpy(instruccion_recibida->parametro5, parametro5);
	free(parametro5);

	list_destroy(paquete);
	return instruccion_recibida;
}

t_datos_proceso* recibir_datos_del_proceso(int fd_kernel){
	t_list* paquete = recibir_paquete(fd_kernel);
	t_datos_proceso* datos_proceso = malloc(sizeof(t_datos_proceso)); // Cuando implementen la funcion hagan FREE
	
	char *path = list_get(paquete, 0);
	datos_proceso->path = malloc(strlen(path)); 					//		""""""""""""""""""""""""""""
	strcpy(datos_proceso->path, path);
	free(path);

	int* pid = list_get(paquete,1);
	datos_proceso->pid = *pid;
	free(pid);

	list_destroy(paquete);
	return datos_proceso;

}

motivo_desalojo recibir_motiv_desalojo(int fd_escucha_interrupt){
	t_list* paquete = recibir_paquete(fd_escucha_interrupt);
	motivo_desalojo* motivo = list_get(paquete, 0);
	motivo_desalojo ret = *motivo;
	free(motivo);
	list_destroy(paquete);
	return ret;
}
