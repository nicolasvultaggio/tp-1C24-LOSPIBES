#include <../../cpu/include/cpu.h>

int main(int argc, char* argv[]){
    
	decir_hola("CPU");
    
    logger_cpu = log_create("cpu_logs.log","cpu",1,LOG_LEVEL_INFO);
    config_cpu = config_create("./configs/cpu.config");

	log_info(logger_cpu, "INICIA CPUTA");

    leer__configuraciones();
	inicializar_semaforos();

	/* CONECTO CPU CON MEMORIA */
	fd_conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_cpu, "CPU");
	enviar_mensaje("Hola, soy CPUta!", fd_conexion_memoria);
	snd_handshake(fd_conexion_memoria);
	log_info(logger_cpu, "Handshake a MEMORIA realizado");
	
	inicializar_tlb();

	/* COMENZAMOS CON LA LOCURA DE HILOS */
	pthread_t *hilo_dispatch = malloc(sizeof(pthread_t));
	pthread_t *hilo_interrupt = malloc(sizeof(pthread_t));
	pthread_t *hilo_cilo_instruccion = malloc(sizeof(pthread_t));

	pthread_create(hilo_dispatch, NULL, (void*)dispatch, NULL);
	pthread_create(hilo_interrupt, NULL, (void*)interrupcion, NULL);
	pthread_create(hilo_cilo_instruccion, NULL, (void*)fetch, NULL);
	
	pthread_join(*hilo_dispatch, NULL);
	pthread_join(*hilo_interrupt, NULL);
	pthread_join(*hilo_cilo_instruccion, NULL);

    terminar_programa();

    return EXIT_SUCCESS;    
}

/* OBTENER PARAMETROS DE .CONFIG */
void leer__configuraciones(){
    ip_memoria = config_get_string_value(config_cpu,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_cpu,"PUERTO_MEMORIA");
    puerto_cpu_dispatch = config_get_string_value(config_cpu,"PUERTO_ESCUCHA_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config_cpu,"PUERTO_ESCUCHA_INTERRUPT");
	cantidad_entradas_tlb = config_get_string_value(config_cpu,"CANTIDAD_ENTRADAS_TLB");
	algoritmo_tlb = config_get_string_value(config_cpu,"ALGORITMO_TLB");
}

/* Inicializo SEMAFOROS */
void inicializar_semaforos(){

	pthread_mutex_init(&mutex_tlb, NULL);

	sem_init(&sem_recibir_pcb, 0, 1);
	sem_init(&sem_execute, 0, 0);
	sem_init(&sem_interrupcion, 0, 0);

	
}

void dispatch(void *arg){

	fd_cpu_dispatch = iniciar_servidor(NULL, puerto_cpu_dispatch,logger_cpu,"CPU");
	log_info(logger_cpu, "Levantado el puerto DISPATCH");	
	fd_escucha_dispatch  = esperar_cliente(fd_cpu_dispatch, logger_cpu, "Kernell (dispatch)");

	while (1) {
		sem_wait(&sem_recibir_pcb);
		hay_interrupcion = false;// ponerle mutex a hay_interrupción, ya que dos procesos que se ejecutan paralelamente lo modifican o leen --> puede haber condicion de carrera
		int codigo_operacion = recibir_operacion(fd_escucha_dispatch, logger_cpu, "Kernell (dispatch)");
		switch (codigo_operacion) {
		case PCBBITO:
			PCB = recibir_pcb(fd_escucha_dispatch);
			sem_post(&sem_execute);
			break;
		case -1:
			log_error(logger_cpu, "El cliente se desconecto.");
			fd_escucha_dispatch = esperar_cliente(fd_cpu_dispatch, logger_cpu, "Kernel (dispatch)");
			break;
		default:
			log_warning(logger_cpu,"Operacion desconocida.");
			break;
		}
	}
}

/* FETCH busco las proximas intrucciones a ejecutar */
void fetch (void *arg){

	while(1){

		sem_wait(&sem_execute);
		t_linea_instruccion* proxima_instruccion = prox_instruccion(PCB->PID, PCB->PC);
		log_info(logger_cpu, "PID: %d - FETCH - Program Counter: %d", PCB->PID, PCB->PC);
		PCB->PC++;
		decode(proxima_instruccion, PCB);

	}
}

t_linea_instruccion* prox_instruccion(int pid, int program_counter){
	
	t_linea_instruccion* instruccion_recibida = malloc(sizeof(t_linea_instruccion*));
	
	//enviar_solicitud_de_instruccion(fd_conexion_memoria, pid, program_counter);
	
	int codigo_operacion = recibir_operacion(fd_conexion_memoria, logger_cpu, "Memoria");
		switch (codigo_operacion) {
			case PROXIMA_INSTRUCCION:
				instruccion_recibida = recibir_proxima_instruccion(fd_conexion_memoria);
				break;
		}
		return instruccion_recibida;
}

/* DECODE interpreto las intrucciones y las mando a ejecutar */
void decode (t_linea_instruccion* instruccion, pcb* PCB){
	switch(instruccion->instruccion){
		case SET:
			ejecutar_set(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case MOV_IN:
			ejecutar_mov_in(PCB, instruccion->parametro1, instruccion->parametro2);
			break;
		case MOV_OUT:
			ejecutar_mov_out(PCB, instruccion->parametro1, instruccion->parametro2);
			break;
		case SUM:
			ejecutar_sum(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case SUB:
			ejecutar_sub(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case JNZ:
			ejecutar_jnz(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case RESIZE:
			//ejecutar_resize();
			break;
		case COPY_STRING:
			//ejecutar_copy_string();
			break;
		case WAIT:
			ejecutar_wait(PCB, instruccion->parametro1);
			break;
		case SIGNAL:
			ejecutar_signal(PCB, instruccion->parametro1);
			break;
		case IO_GEN_SLEEP:
			ejecutar_io_gen_sleep(PCB, "IO_GEN_SLEEP", instruccion->parametro1, instruccion->parametro2);
			break;
		case IO_STDIN_READ:
			//ejecutar_io_stdin_read();
			break;
		case IO_STDOUT_WRITE:
			//ejecutar_io_stdout_write();
			break;
		case IO_FS_CREATE:
			//ejecutar_io_fs_create();
			break;
		case IO_FS_DELETE:
			//ejecutar_io_fs_delete();
			break;
		case IO_FS_TRUNCATE:
			//ejecutar_io_fs_truncate();
			break;
		case IO_FS_WRITE:
			//ejecutar_io_fs_write();
			break;
		case IO_FS_READ:
			//ejecutar_io_fs_read();
			break;
		case EXIT:
			ejecutar_exit(PCB);
			break;
		default:
			ejecutar_error(PCB);
			break;
	}
}

/* EXECUTE se ejecua lo correspodiente a cada instruccion */

void ejecutar_set(pcb* PCB, char* registro, char* valor){
	if(medir_registro(registro)){
		uint8_t valor8 = strtoul(valor, NULL, 10);
		setear_registro8(PCB, registro, valor8);
	}else{
		uint32_t valor32 = strtoul(valor, NULL, 10);
		setear_registro32(PCB, registro, valor32);
	}
	return;
}

void ejecutar_mov_in(pcb* PCB, char* DATOS, char* DIRECCION){

log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "MOV IN", param1, param2);
	
	
	
	int direccion_logica = atoi(DIRECCION);
	int direccion_fisica = solicitar_direccion_fisica(pcb, direccion_logica);
	if(direccion_fisica == -1){
		//pcb->program_counter -= 1;
		return;

}
}

void ejecutar_mov_out(pcb* PCB, char* DIRECCION, char* DATOS){
	log_info(logger, "PID: %d - Ejecutando: %s - [%s, %s]", pcb->pid, "MOV OUT", param1, param2);
		
	int direccion_fisica = MMU(pcb, DIRECCION);
	
	if(direccion_fisica == -1){
		//pcb->program_counter -= 1;
		return;


}}


void ejecutar_sum(pcb* PCB, char* destinoregistro, char* origenregistro){
	uint8_t destino8;
	uint8_t origen8;
	uint32_t destino32;
	uint32_t origen32;
	if(medir_registro(destinoregistro)){
		destino8 = capturar_registro8(PCB, destinoregistro);
	}else{
		destino32 = capturar_registro32(PCB, destinoregistro);
	}

	if(medir_registro(origenregistro)){
		origen8 = capturar_registro8(PCB, origenregistro);
	}else{
		origen32 = capturar_registro32(PCB, origenregistro);
	}

	if(medir_registro(destinoregistro)){
		setear_registro8(PCB, destinoregistro, destino8 + (uint8_t) origen8); 
	}else{//																	//ojo, aca no hay mas referencia a origen ni destino, solo 
		setear_registro32(PCB, destinoregistro, destino32 + (uint32_t) origen32);
	}
		
	return;
}

void ejecutar_sub(pcb* PCB, char* destinoregistro, char* origenregistro){

	uint8_t origen;
	uint32_t origen32;

	if(medir_registro(origenregistro)){
		origen = capturar_registro8(PCB, origenregistro);
	}else{
		origen = capturar_registro32(PCB, origenregistro);
	}

	if(medir_registro(destinoregistro)){
		uint8_t destino = capturar_registro8(PCB, destinoregistro);
		uint8_t origen8 = (uint8_t) origen;
		setear_registro8(PCB, destinoregistro, destino + origen8);
	}else{
		uint32_t destino = capturar_registro32(PCB, destinoregistro);
		uint32_t origen8 = (uint32_t) origen32;
		setear_registro32(PCB, destinoregistro, destino + origen32);
	}
		
	return;
}

void ejecutar_jnz(pcb* PCB, char* registro, char* valor){
	uint32_t program_counter_actualizado = (uint32_t)strtoul(valor,NULL,10);
	if(medir_registro(registro)){
		if(capturar_registro8(PCB, registro) != 0)
			PCB->PC = program_counter_actualizado;
	}else{
		if(capturar_registro32(PCB, registro) != 0)
			PCB->PC = program_counter_actualizado;
	}
	return;
}

void ejecutar_wait(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "WAIT", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch, RECURSO, SOLICITAR_WAIT,NULL,NULL,NULL,NULL,NULL);
	free(recurso);
	sem_post(&sem_recibir_pcb);
}

void ejecutar_signal(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "SIGNAL", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch, RECURSO, SOLICITAR_SIGNAL,NULL,NULL,NULL,NULL,NULL);
	free(recurso);
	sem_post(&sem_execute);
}

void ejecutar_io_gen_sleep(pcb* PCB, char* instruccion, char* interfaz, char* unidad_de_tiempo){
	enviar_pcb(PCB, fd_escucha_dispatch, INTERFAZ, SOLICITAR_INTERFAZ_GENERICA, instruccion, interfaz, unidad_de_tiempo,NULL,NULL);
	sem_post(&sem_execute);
	return;
}

void ejecutar_exit(pcb* PCB){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, EXITO,NULL,NULL,NULL,NULL,NULL);
	sem_post(&sem_execute);
}

void ejecutar_error(pcb* PCB){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, EXIT_CONSOLA,NULL,NULL,NULL,NULL,NULL);
	sem_post(&sem_execute);
}


/* FUNCIONES PARA EL CALCULO ARITMETICOLOGICO */

bool medir_registro(char* registro){
	if(strcmp(registro, "AX") == 0){
		return true;
	} else if(strcmp(registro, "BX") == 0){
		return true;
	} else if(strcmp(registro, "CX") == 0){
		return true;
	} else if(strcmp(registro, "DX") == 0){
		return true;
	}else{
		return false;
	}
}

void setear_registro8(pcb* PCB, char* registro, uint8_t valor){
	if(strcmp(registro, "AX") == 0){
		PCB->registros.AX = valor;
	} else if(strcmp(registro, "BX") == 0){
		PCB->registros.BX = valor;
	} else if(strcmp(registro, "CX") == 0){
		PCB->registros.CX = valor;
	} else if(strcmp(registro, "DX") == 0){
		PCB->registros.DX = valor;
	}
	return;
}

void setear_registro32(pcb* PCB, char* registro, uint32_t valor){
   if(strcmp(registro, "EAX") == 0){
		PCB->registros.EAX = valor;
	} else if(strcmp(registro, "EBX") == 0){
		PCB->registros.EBX = valor;
	} else if(strcmp(registro, "ECX") == 0){
		PCB->registros.ECX = valor;
	} else if(strcmp(registro, "EDX") == 0){
		PCB->registros.EDX = valor;
	} else if(strcmp(registro, "SI") == 0){
		PCB->registros.SI = valor;
	} else if(strcmp(registro, "DI") == 0){
		PCB->registros.DI = valor;
	}
	return;
}

uint8_t capturar_registro8(pcb* PCB, char* registro){
	if(strcmp(registro, "AX") == 0){
		return PCB->registros.AX;
	} else if(strcmp(registro, "BX") == 0){
		return PCB->registros.BX;
	} else if(strcmp(registro, "CX") == 0){
		return PCB->registros.CX;
	} else if(strcmp(registro, "DX") == 0){
		return PCB->registros.DX;
	}
}

uint32_t capturar_registro32(pcb* PCB, char* registro){
   if(strcmp(registro, "EAX") == 0){
		return PCB->registros.EAX;
	} else if(strcmp(registro, "EBX") == 0){
		return PCB->registros.EBX;
	} else if(strcmp(registro, "ECX") == 0){
		return PCB->registros.ECX;
	} else if(strcmp(registro, "EDX") == 0){
		return PCB->registros.EDX;
	} else if(strcmp(registro, "SI") == 0){
		return PCB->registros.SI;
	} else if(strcmp(registro, "DI") == 0){
		return PCB->registros.DI;
	}
}

/*****************************************************/

/* TRADUCCION LOGICA A FISICA */

int MMU(pcb* PCB, char* DIRECCION){
	int direccion_fisica;
	int marco;
	int direccion_logica = atoi(DIRECCION);
	int numero_pagina = floor(direccion_logica / TAM_PAGINA);
	int desplazamiento =  direccion_logica - numero_pagina * TAM_PAGINA;

	switch (MAX_TLB_ENTRY)
	{
	case 0:
		enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
		marco = recibir_marco();
		break;
	default:
		marco = consultar_tlb(PCB->PID, numero_pagina);
		break;
	}


	
	send_solicitud_marco(fd_memoria, pcb->pid, numero_pagina);
	int marco = recibir_marco();
	if(marco == -1){
		//log_info(logger, "Page Fault PID: %d - Pagina: %d", pcb->pid, numero_pagina); //log ob
		//iniciar acciones page fault
		//send_pcb(pcb, dispatch_cliente_fd);
		//send_pcb_pf(numero_pagina, desplazamiento, dispatch_cliente_fd);
		//sem_post(&sem_nuevo_proceso);
		//return marco;
	}
	log_info(logger,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", pcb->pid, numero_pagina, marco); //log ob
	//int direccion_fisica = marco*tam_pagina + desplazamiento;
	return direccion_fisica;
}



/* CHECK INTERRUPT */
void* interrupcion(void *arg) {

	fd_cpu_interrupt = iniciar_servidor(NULL, puerto_cpu_interrupt, logger_cpu, "CPU");
	log_info(logger_cpu, "Leavantado el puerto INTERRUPT");
	fd_escucha_interrupt = esperar_cliente(fd_cpu_interrupt,logger_cpu, "Kernel (interrupt)");
	
	while (1) {
		int codigo_operacion = recibir_operacion(fd_escucha_interrupt, logger_cpu, "Kernel (interrupt)");
		switch (codigo_operacion) {
		case INTERR:
			detectar_motivo_desalojo();
			sem_post(&sem_recibir_pcb);
			break;
		case -1:
			log_error(logger_cpu, "El cliente se desconecto.");
			fd_escucha_interrupt = esperar_cliente(fd_cpu_interrupt,logger_cpu, "Kernel (interrupt)");
			break;
		default:
			log_warning(logger_cpu,"Operacion desconocida.");
			break;
		}
	}
}

void detectar_motivo_desalojo(){
	motivo_desalojo motivo = recibir_motiv_desalojo(fd_escucha_interrupt);
	hay_interrupcion = true;//OJO, variable que leen y escriben muchos hilos, posible mutex
	sem_wait(&sem_interrupcion);
	switch (motivo)	{
		case INTERRUPCION:
			log_info(logger_cpu, "Interrupcion: Finalizar proceso.");
			PCB->motivo = INTERRUPCION;
			enviar_pcb(PCB, fd_escucha_dispatch, INTERR, INTERRUPCION,NULL,NULL,NULL,NULL,NULL); 
			break;

		case FIN_QUANTUM:
			log_info(logger_cpu, "Interrupcion: Fin de Quantum.");
			PCB->motivo = FIN_QUANTUM;
			enviar_pcb(PCB, fd_escucha_dispatch, INTERR, FIN_QUANTUM,NULL,NULL,NULL,NULL,NULL);
			break;
	}
	return;
}

void check_interrupt (){
	if(hay_interrupcion) {
		sem_post(&sem_interrupcion);
	} else {
		sem_post(&sem_execute);
	}
}


/* TLB */

void inicializar_tlb(){

	translation_lookaside_buffer = list_create();
	MAX_TLB_ENTRY = atoi (cantidad_entradas_tlb);

}

int solicitar_info_memory(int PID){

	enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
	return recibir_marco();
}

void agregar_entrada_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria, pthread_mutex_t* mutex, int PID, int numero_pagina, int marco){

	info_proceso_memoria->valor->pid = PID;
	info_proceso_memoria->valor->numero_pagina = numero_pagina;
	info_proceso_memoria->valor->marco = marco;

	push_con_mutex(TLB, info_proceso_memoria, &mutex_tlb);

}

void eliminar_entrada_tlb(t_list* TLB, pthread_mutex_t* mutex, int posicion){

	remove_con_mutex(TLB, mutex, posicion);
	
} 

void verificar_tamanio_tlb(t_list* TLB, pthread_mutex_t* mutex){

	if((list_size(TLB)/MAX_TLB_ENTRY) =! 1){
		return;
	}
	pop_con_mutex(TLB, mutex);
}

int consultar_tlb(int PID, int numero_pagina){
	
	int marco;

	nodo_tlb * info_proceso_memoria;

	if(list_is_empty(translation_lookaside_buffer)){
		marco = solicitar_info_memory(PID, numero_pagina);
		agregar_entrada_tlb(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
		return info_proceso_memoria->valor->marco;
	}
	
	info_proceso_memoria = list_find(translation_lookaside_buffer,(void*)PCB->PID);

	if(info_proceso_memoria = NULL){
		marco = solicitar_info_memory(PID, numero_pagina);
		agregar_entrada_tlb(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
	}

	marco = info_proceso_memoria->valor->marco;

	administrar_tlb(translation_lookaside_buffer, info_proceso_memoria);

	return marco;
}

int posicion_nodo_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria){
	for(int i = 0; i<list_size(TLB); i++){
		nodo_tlb * NODO = list_get(TLB, i);
		if(NODO->valor->pid == info_proceso_memoria->valor->pid){
			return i;
		}
}
}

void administrar_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria){

	if(algoritmo_tlb == "FIFO"){
		return;
	}

	verificar_tamanio_tlb(TLB &mutex_tlb);

	int posicion_nodo_buscado = posicion_nodo_tlb(TLB, info_proceso_memoria);

	agregar_entrada_tlb(TLB, info_proceso_memoria, &mutex_tlb, info_proceso_memoria->valor->pid, info_proceso_memoria->valor->numero_pagina, info_proceso_memoria->valor->marco);
	
	eliminar_entrada_tlb(TLB, &mutex_tlb, posicion_nodo_buscado);

	return;
}


/* LIBERAR MEMORIA USADA POR CPU */
void terminar_programa(){
    if(logger_cpu != NULL){
		log_destroy(logger_cpu);
	}
	if(config_cpu != NULL){
		config_destroy(config_cpu);
	}
    liberar_conexion(fd_conexion_memoria);
}
