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

}

/* Inicializo SEMAFOROS */
void inicializar_semaforos(){
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
		hay_interrupcion = false;
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
	enviar_solicitud_de_instruccion(fd_conexion_memoria, pid, program_counter);
	
	while (1) {
		int codigo_operacion = recibir_operacion(fd_conexion_memoria, logger_cpu, "Memoria");
		switch (codigo_operacion) {
			case PROXIMA_INSTRUCCION:
				instruccion_recibida = recibir_proxima_instruccion(fd_conexion_memoria);
				break;
		}
		return instruccion_recibida;
	}
}

/* DECODE interpreto las intrucciones y las mando a ejecutar */
void decode (t_linea_instruccion* instruccion, pcb* PCB){
	switch(instruccion->instruccion){
		case SET:
			ejecutar_set(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case MOV_IN:
			ejecutar_mov_in();
			break;
		case MOV_OUT:
			ejecutar_mov_out();
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
			ejecutar_resize();
			break;
		case COPY_STRING:
			ejecutar_copy_string();
			break;
		case WAIT:
			ejecutar_wait(PCB, instruccion->parametro1);
			break;
		case SIGNAL:
			ejecutar_signal(PCB, instruccion->parametro1);
			break;
		case IO_GEN_SLEEP:
			ejecutar_io_gen_sleep(PCB, instruccion->parametro1, instruccion->parametro2);
			break;
		case IO_STDIN_READ:
			ejecutar_io_stdin_read();
			break;
		case IO_STDOUT_WRITE:
			ejecutar_io_stdout_write();
			break;
		case IO_FS_CREATE:
			ejecutar_io_fs_create();
			break;
		case IO_FS_DELETE:
			ejecutar_io_fs_delete();
			break;
		case IO_FS_TRUNCATE:
			ejecutar_io_fs_truncate();
			break;
		case IO_FS_WRITE:
			ejecutar_io_fs_write();
			break;
		case IO_FS_READ:
			ejecutar_io_fs_read();
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

void ejecutar_sum(pcb* PCB, char* destinoregistro, char* origenregistro){
	if(medir_registro(origenregistro)){
		uint8_t destino = capturar_registro8(PCB, destinoregistro);
		uint8_t origen = capturar_registro8(PCB, origenregistro);
		setear_registro8(PCB, destinoregistro, destino + origen);
	}else{
		uint32_t destino = capturar_registro32(PCB, destinoregistro);
		uint32_t origen = capturar_registro32(PCB, destinoregistro);
		setear_registro32(PCB, destinoregistro, destino + origen);
	}
	return;
}

void ejecutar_sub(pcb* PCB, char* destinoregistro, char* origenregistro){
	if(medir_registro(origenregistro)){
		uint8_t destino = capturar_registro8(PCB, destinoregistro);
		uint8_t origen = capturar_registro8(PCB, origenregistro);
		setear_registro8(PCB, destinoregistro, destino - origen);
	}else{
		uint32_t destino = capturar_registro32(PCB, destinoregistro);
		uint32_t origen = capturar_registro32(PCB, destinoregistro);
		setear_registro32(PCB, destinoregistro, destino - origen);
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

void ejecutar_wait(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "WAIT", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch);
	enviar_recurso_por_wait(recurso, fd_escucha_dispatch, ESPERA);
	free(recurso);
	sem_post(&sem_recibir_pcb);
}

void ejecutar_signal(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "SIGNAL", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch);
	enviar_recurso_por_signal(recurso, fd_escucha_dispatch, SIGNAL);
	free(recurso);
	sem_post(&sem_execute);
}

void ejecutar_io_gen_sleep(pcb* PCB, char* interfaz, char* unidad_de_tiempo){
	enviar_solicitud_de_dormir_interfaz_generica(fd_cpu_dispatch, interfaz, unidad_de_tiempo);
	enviar_pcb(pcb* PCB, int fd_escucha_dispatch);
	sem_post(&sem_execute);
	return;
}

void ejecutar_exit(pcb* PCB){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch);
	enviar_cambio_de_estado(EXITO, fd_escucha_dispatch);
	sem_post(&sem_execute);
}

void ejecutar_error(pcb* PCB){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch);
	enviar_cambio_de_estado(EXIT_CONSOLA, fd_escucha_dispatch);
	sem_post(&sem_execute);
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
	hay_interrupcion = true;
	sem_wait(&sem_interrupcion);
	switch (motivo)	{
		case INTERRUPCION:
			log_info(logger_cpu, "Interrupcion: Finalizar proceso.");
			PCB->motivo = INTERRUPCION;
			enviar_pcb(PCB, fd_escucha_dispatch);
			//enviar_solicitud_de_cambio_de_estado(EXITT, fd_escucha_interrupt);
			break;
		case FIN_QUANTUM:
			log_info(logger_cpu, "Interrupcion: Fin de Quantum.");
			PCB->motivo = FIN_QUANTUM;
			enviar_pcb(PCB, fd_escucha_interrupt);
			//enviar_solicitud_de_cambio_de_estado(READY, fd_escucha_interrupt);
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
