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

void * remove_con_mutex(t_list* lista, pthread_mutex_t* mutex, int posicion){
	pthread_mutex_lock(mutex);
	void* elemento = list_remove(lista, posicion); 
	pthread_mutex_unlock(mutex);
    return elemento;
}

int buscar_posicion_proceso(t_list* lista, int pid){

	for(int i = 0; i<list_size(lista); i++){
		pcb* pcb_buscado = list_get(lista, i);
		if(pcb_buscado->PID == pid){
			return i;
		}
	}
	return -1;
}


void empaquetar_pcb(t_paquete* paquete, pcb* un_PCB, motivo_desalojo MOTIVO){

	un_PCB->motivo = MOTIVO ; //faltaba esto no? si no el motivo se pasa al pedo por parametro

	//Sergio: no seria mas facil hacer un agregar_a_paquete(paquete,un_PCB,sizeof(pcb)); y listo? creo que como esta ahora seria un uso de memoria inecesario
	//Nico:no, no podes, por serializacion, repasate la guia, no podes enviar todo un struct de una, tenes que agregar todos los datos uno x uno
	agregar_a_paquete(paquete, &(un_PCB->PID), sizeof(int));
	agregar_a_paquete(paquete, &(un_PCB->PC), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(un_PCB->QUANTUM), sizeof(int));
	agregar_a_paquete(paquete, &(un_PCB->motivo), sizeof(int));
	agregar_a_paquete(paquete, &(un_PCB->estado), sizeof(int));
	agregar_a_paquete(paquete, &(un_PCB->registros.AX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.BX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.CX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.DX), sizeof(uint8_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.EAX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.EBX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.ECX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.EDX), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.SI), sizeof(uint32_t));
	agregar_a_paquete(paquete, &(un_PCB->registros.DI), sizeof(uint32_t));


}
/* VER ESTO ANTES DE ENTENDER LOS PROXIMOS PROGRAMAS
Ejemplo de como quedaria un PCB empaquetado

pcb{
	pid
	pc
	.
	.
	.
	t_list * recursos_asignados = [recurso1, recurso2 recurso3]
}

recursos_asignados{
	nombre
	instancias_asignadas
}

1.PID
2.PC
.
.
.
n.cantidad de recursos (ejemplo = 3)
n+1.nombre (recurso 1)
n+2.instancias_asignadas(recurso1)
n+3 nombre (recurso2)
n+4 instancias_asignadas(recurso2)
.
.
.
*/

int fin_pcb(pcb* pcb){
	int cantidad_de_recursos = list_size(pcb->recursos_asignados);
	printf("Cantidad de recursos: %d", cantidad_de_recursos);
    int elementosDeLaListaQueSonRecursos = cantidad_de_recursos * 2;
	
    //return cantidad_de_recursos + cantidad_de_atributos_recursos; 
	return 15 + elementosDeLaListaQueSonRecursos; //TOMI FIJATE SI esta corrección está mejor
}
/*
PID
.
.
.
.
ULTIMO REGISTRO; 14
CANTIDAD RECURSOS =2; 15
NOMBRE 16
INSTANCIA 17
NOMBRE 18
INSTANCIA 19
*/

void empaquetar_recursos(t_paquete* paquete, t_list* lista_de_recursos){
	int cantidad_recursos = list_size(lista_de_recursos);
	printf("La cantidad de recursos es: %d \n", cantidad_recursos);
	agregar_a_paquete(paquete, &(cantidad_recursos), sizeof(int));//Agregamos la CANTIDAD de RECURSOS
	for(int i = 0; i<cantidad_recursos; i++){
		recurso_asignado* recurso_asignado = list_get(lista_de_recursos, i);
		agregar_a_paquete(paquete, recurso_asignado->nombreRecurso, strlen(recurso_asignado->nombreRecurso) + 1);
		agregar_a_paquete(paquete, &(recurso_asignado->instancias), sizeof(int));
	}
}
void empaquetar_traducciones(t_paquete* paquete, t_list* lista_de_traducciones){
	int cantidad_de_traducciones =  list_size(lista_de_traducciones);
	agregar_a_paquete(paquete, &(cantidad_de_traducciones), sizeof(int));//Agregamos la cantidad de traducciones
	for(int i = 0; i<cantidad_de_traducciones; i++){
		nodo_lectura_escritura* traduccion = list_get(lista_de_traducciones, i);
		agregar_a_paquete(paquete, &(traduccion->direccion_fisica), sizeof(uint32_t));
		agregar_a_paquete(paquete, &(traduccion->bytes), sizeof(uint32_t));
	}
}

t_list* desempaquetar_recursos(t_list* paquete, int cantidad){
	t_list* recursos = list_create();
	int* cantidad_recursos = list_get(paquete, cantidad);
	int i = cantidad + 1; //ej: cantidad = 3 -> arranca desde 4 porq el 3 seria la cantidad de recursos

//PRIMERA TIRADA
//	4-3-1 < 6 -> 0 < 6 -> 6, osea, hay 3 recursos (3 nombres + 3 instancias) 0<
//SEGUNDA
// 6-4 < 6 -> 2 < 6 -> faltan 2 recursos (2 nombres + 2 instancias)

	while(i - cantidad - 1 < ((*cantidad_recursos)* 2)){
		recurso_asignado* recurso_asignado = malloc(sizeof(recurso_asignado));
		char* nombre = list_get(paquete, i); //nombe del primero
		recurso_asignado->nombreRecurso = malloc(strlen(nombre) + 1);
		strcpy(recurso_asignado->nombreRecurso, nombre);
		//free(nombre);           esto creo que tiene que estar porque ya se copio el valor a otro puntero
		i++; // para pasar a las instancias

		int* instancia = list_get(paquete, i);
		recurso_asignado->instancias = *instancia;
		free(instancia);
		i++; // para pasar al nombre del siguiente recurso

		list_add(recursos, recurso_asignado);
//		free(recurso_asignado->nombre_recurso);
	}

	free(cantidad_recursos); //¿hay que hacerle free?
	return recursos;
}
t_list * desempaquetar_traducciones(t_list* paquete, int index_cantidad){
	t_list* traducciones = list_create();
	int* cantidad_traducciones = list_get(paquete, index_cantidad);
	int i = index_cantidad + 1; 

	while(i - index_cantidad - 1 < ((*cantidad_traducciones)* 2)){
		nodo_lectura_escritura* traduccion = malloc(sizeof(nodo_lectura_escritura));
		uint32_t* una_direccion_fisica = (uint32_t*) list_get(paquete, i); //nombe del primero
		traduccion->direccion_fisica = *una_direccion_fisica;
		free(una_direccion_fisica);
		i++;

		uint32_t* _bytes = (uint32_t*) list_get(paquete, i);
		traduccion->bytes = *_bytes;
		free(_bytes);
		i++; // para pasar al nombre del siguiente recurso

		list_add(traducciones, traduccion);
	}

	free(cantidad_traducciones);
	return traducciones;
}

void traduccion_destroyer(void * traduccion){
	nodo_lectura_escritura * traduccion_c = (nodo_lectura_escritura *) traduccion;
	free(traduccion_c);
}

void pcb_destroy(pcb* pcb){
    int tamanioListaRecursos = list_size(pcb->recursos_asignados);
    for (int i = 0; i < tamanioListaRecursos; i++)
    {
        recurso_asignado* recurso_asignado = list_get(pcb->recursos_asignados, i);
        //recurso_destroy(recurso_asignado);
    }
    free(pcb);
}
//void recurso_destroy_pcb(recurso_asignado* recurso_pcb)

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

void iterator(char* value, t_log *logger) {
	log_info(logger,"%s", value);
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

void enviar_pcb(pcb* PCB, int un_fd, op_code OPERACION, motivo_desalojo MOTIVO, char* parametro1, char* parametro2, char* parametro3, char* parametro4, char* parametro5){

	t_paquete* paquete = crear_paquete(OPERACION);
	empaquetar_pcb(paquete, PCB, MOTIVO);
	empaquetar_recursos(paquete, PCB->recursos_asignados);
	
	switch (MOTIVO){ 
		
		case SOLICITAR_INTERFAZ_GENERICA:
			agregar_a_paquete(paquete, parametro1, strlen(parametro1) + 1);
			agregar_a_paquete(paquete, parametro2, strlen(parametro2) + 1);
			agregar_a_paquete(paquete, parametro3, strlen(parametro3) + 1);
			break;

		case SOLICITAR_WAIT:
			agregar_a_paquete(paquete, parametro1, strlen(parametro1) + 1);
			break;

		case SOLICITAR_SIGNAL:
			agregar_a_paquete(paquete, &parametro1, strlen(parametro1) + 1);
			break;
		
		/*
			-Sergio:ES NECESARIO COLOCAR LOS MOTIVOS EN LOS UE NO SE AGREGA INFORMACION AL PCB? 
			-Nico: tiene razon Sergio, creo que no haría falta poner el resto porque total no agregan mas informacion, podría salir del switch ahora
			(cuando el resto lo vea pongan que opinan)
		*/
		case EXITO:
			break;
		case EXIT_CONSOLA:
			break;
		case INTERRUPCION:
			break;
		case FIN_QUANTUM:
			break;	
		case PROCESO_ACTIVO:
			break;
		case FINALIZAR_PROCESO:
			break;
		case SIN_MEMORIA:
			break;
		default:
			break;
	}
	enviar_paquete(paquete, un_fd);
	eliminar_paquete(paquete);
	return;

}

void enviar_liberar_proceso(pcb* un_pcb, int fd){
	/*
	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);

	agregar_a_paquete(paquete, pcb,sizeof(pcb));

	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
	*/

	//oojo, esto de aca arriba esta mal, no hace falta enviar el pcb entero, solo el pid
	//ademas esta mal enviado el pcb, no podes enviar un struct asi de una en un paquete nenedaun
	t_paquete* paquete = crear_paquete(FINALIZAR_PROCESO);
	agregar_a_paquete(paquete,&(un_pcb->PID),sizeof(int));
	enviar_paquete(paquete,fd);
	eliminar_paquete(paquete);

	
}

void enviar_tamanio_pagina(int fd_cpu_dispatch, int tam_pag){
	t_paquete* paquete = crear_paquete(SIZE_PAGE);
	agregar_a_paquete(paquete, &tam_pag, sizeof(int));
	enviar_paquete(paquete, fd_cpu_dispatch);
	eliminar_paquete(paquete);
}

void enviar_solicitud_marco( int fd_conexion_memoria,int pid, int numero_pagina){
	t_paquete* paquete = crear_paquete(SOLICITUD_MARCO);
	//ES NECESARIO HACER ESTAS VARIABLES COMO PUNTEROS? na, no hacia falta xd
	agregar_a_paquete(paquete, &pid, sizeof(int));
	agregar_a_paquete(paquete,&numero_pagina, sizeof(int));
	enviar_paquete(paquete, fd_conexion_memoria);
	eliminar_paquete(paquete);
	return;
}

void enviar_marco (int fd_conexion_memoria, int marco){ //EN EL CASO DE NO ENCONTRAR EL MARCO CORRESPONDIENTE A LOS VALORES PEDIDOS ENVIAR UN -1
	t_paquete* paquete = crear_paquete(MARCO);
	agregar_a_paquete(paquete, &marco, sizeof(int));
	enviar_paquete(paquete, fd_conexion_memoria);
	eliminar_paquete(paquete);
}


/* FUNNCIONES DE RECIBO (RECV) */
void recibir_mensaje(t_log* loggerServidor ,int socket_cliente){
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

	//Sergio: y apartir de lo de enviar pcb aca seria mas facil PCB = list_get(paquete,0)  y listo

	uint32_t* pid = list_get(paquete, 0);
	PCB->PID = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	PCB->PC = *program_counter;
	free(program_counter);

	int* quantum = list_get(paquete, 2);
	PCB->QUANTUM = *quantum;
	free(quantum);

	motivo_desalojo* motivo_recibido = list_get(paquete,3);
	PCB->motivo = *motivo_recibido;
	free(motivo_recibido);

	estadosDeLosProcesos* estado_recibido = list_get(paquete, 4);
	PCB->estado = *estado_recibido;
	free(estado_recibido);

	uint8_t* ax = list_get(paquete, 5);
	PCB->registros.AX = *ax;
	free(ax);

	uint8_t* bx = list_get(paquete, 6);
	PCB->registros.BX = *bx; 
	free(bx);

	uint8_t* cx = list_get(paquete, 7);
	PCB->registros.CX = *cx;
	free(cx);

	uint8_t* dx = list_get(paquete, 8);
	PCB->registros.DX = *dx;
	free(dx);

	uint32_t* eax = list_get(paquete, 9);
	PCB->registros.EAX = *eax;
	free(eax);

	uint32_t* ebx = list_get(paquete, 10);
	PCB->registros.EBX = *ebx;
	free(ebx);

	uint32_t* ecx = list_get(paquete, 11);
	PCB->registros.ECX = *ecx;
	free(ecx);

	uint32_t* edx = list_get(paquete, 12);
	PCB->registros.EDX = *edx;
	free(edx);

	uint32_t* si = list_get(paquete, 13);
	PCB->registros.SI = *si;
	free(si);

	uint32_t* di = list_get(paquete, 14);
	PCB->registros.DI = *di;
	free(di);

	t_list* recursos = desempaquetar_recursos(paquete, 15);
	PCB->recursos_asignados = recursos;

	list_destroy(paquete);

	return PCB;
}

t_linea_instruccion* recibir_proxima_instruccion(int fd_conexion){
	
	t_list* paquete = recibir_paquete(fd_conexion);
	t_linea_instruccion* instruccion_recibida = malloc(sizeof(t_linea_instruccion));

	instruccion_recibida->parametros = list_create();
	
	cod_instruccion* instruccion = list_get(paquete, 0);
	instruccion_recibida->instruccion = *instruccion;
	free(instruccion);

	int cantidad_de_parametros=cantidad_de_parametros_segun_instruccion(instruccion_recibida->instruccion);
	
	for(int i=0;i<cantidad_de_parametros;i++){
		void * aux = list_get(paquete,i+1);
		list_add(instruccion_recibida->parametros,aux);
	}

	list_destroy(paquete);
	return instruccion_recibida;
}

t_datos_proceso* recibir_datos_del_proceso(int fd_kernel){
	t_list* paquete = recibir_paquete(fd_kernel);
	t_datos_proceso* datos_proceso = malloc(sizeof(t_datos_proceso)); // Cuando implementen la funcion hagan FREE
	
	datos_proceso->path = (char *)list_get(paquete, 1);	//apuntamos al area de memoria ya reservada al recibir el paquete			

	int* pid = (int *)list_get(paquete,0);
	datos_proceso->pid = *pid;
	free(pid);

	list_destroy(paquete);
	return datos_proceso;

}


pcb* guardar_datos_del_pcb(t_list* paquete){ 

	pcb* PCB = malloc(sizeof(pcb));

	uint32_t* pid = list_get(paquete, 0);
	PCB->PID = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	PCB->PC = *program_counter;
	free(program_counter);

	int* quantum = list_get(paquete, 2);
	PCB->QUANTUM = *quantum;
	free(quantum);

	motivo_desalojo* motivo_recibido = list_get(paquete,3);
	PCB->motivo = *motivo_recibido;
	free(motivo_recibido);

	estadosDeLosProcesos* estado_recibido = list_get(paquete, 4);
	PCB->estado = *estado_recibido;
	free(estado_recibido);

	uint8_t* ax = list_get(paquete, 5);
	PCB->registros.AX = *ax;
	free(ax);

	uint8_t* bx = list_get(paquete, 6);
	PCB->registros.BX = *bx; 
	free(bx);

	uint8_t* cx = list_get(paquete, 7);
	PCB->registros.CX = *cx;
	free(cx);

	uint8_t* dx = list_get(paquete, 8);
	PCB->registros.DX = *dx;
	free(dx);

	uint32_t* eax = list_get(paquete, 9);
	PCB->registros.EAX = *eax;
	free(eax);

	uint32_t* ebx = list_get(paquete, 10);
	PCB->registros.EBX = *ebx;
	free(ebx);

	uint32_t* ecx = list_get(paquete, 11);
	PCB->registros.ECX = *ecx;
	free(ecx);

	uint32_t* edx = list_get(paquete, 12);
	PCB->registros.EDX = *edx;
	free(edx);

	uint32_t* si = list_get(paquete, 13);
	PCB->registros.SI = *si;
	free(si);

	uint32_t* di = list_get(paquete, 14);
	PCB->registros.DI = *di;
	free(di);

	t_list* recursos = desempaquetar_recursos(paquete, 15);
	PCB->recursos_asignados = recursos;

	return PCB;
}


pcb* recibir_liberar_proceso(int fd){
	t_list* paqute = recibir_paquete(fd);
	pcb* pcb = list_get(paqute, 0);

	list_destroy(paqute);
	return pcb;
}


int recibir_marco (int fd_conexion_memoria){ //para recibir send_marco
	int a;
	recv(fd_conexion_memoria,&a,sizeof(int),MSG_WAITALL);//faltaba recibir primero el codigo de operacion MARCO
	t_list* paquete = recibir_paquete(fd_conexion_memoria);
	int marquitos = 0;
	int* marco = (int*) list_get(paquete, 0);
	marquitos = *marco;
	free(marco);
	list_destroy(paquete);
	return marquitos;
}

uint32_t recibir_valor_leido_memoria(int fd_memoria){
	t_list* paquete = recibir_paquete(fd_memoria);
	uint32_t* valor = list_get(paquete, 0);
	list_destroy(paquete);
	return *valor;
}

int cantidad_de_parametros_segun_instruccion(cod_instruccion una_instruccion){
	switch(una_instruccion){
		case SET:
		return 2 ;
		case MOV_IN :
		return 2 ;
		case MOV_OUT:
		return 2;
		case SUM:
		return 2;
		case SUB :
		return 2;
		case JNZ:
		return 2;
		case RESIZE:
		return 1;
		case COPY_STRING:
		return 1;
		case WAIT:
		return 1 ;
		case SIGNAL:
		return 1;
		case IO_GEN_SLEEP:
		return 2 ;
		case IO_STDIN_READ:
		return 3;
		case IO_STDOUT_WRITE:
		return 3 ;
		case IO_FS_CREATE:
		return 2;
		case IO_FS_DELETE:
		return 2;
		case IO_FS_TRUNCATE:
		return 3;
		case IO_FS_WRITE:
		return 5 ;
		case IO_FS_READ:
		return 5;
		case EXIT:
		return 0 ;
	}
}






