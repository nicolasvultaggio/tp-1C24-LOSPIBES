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
	enviar_mensaje("Hola, soy CPU!", fd_conexion_memoria);
	//posiblemente haya que esperar una respuesta del mensaje antes de enviar otra cosa, para que sepamos que el cliente ya esta escuchando de nuevo
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
	pthread_mutex_init(&mutex_motivo_x_consola, NULL);

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

t_linea_instruccion* prox_instruccion(int pid, uint32_t program_counter){
	
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
			log_info(logger_cpu, "PID: %d - Ejecutando SET %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_set(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case MOV_IN:
			log_info(logger_cpu, "PID: %d - Ejecutando MOV_IN %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_mov_in(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case MOV_OUT:
			log_info(logger_cpu, "PID: %d - Ejecutando MOV_OUT %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_mov_out(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case SUM:
			log_info(logger_cpu, "PID: %d - Ejecutando SUM %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_sum(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case SUB:
			log_info(logger_cpu, "PID: %d - Ejecutando SUB %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_sub(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case JNZ:
			log_info(logger_cpu, "PID: %d - Ejecutando JNZ %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_jnz(PCB, instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case RESIZE:
			log_info(logger_cpu, "PID: %d - Ejecutando RESIZE %d %d", PCB->PID, instruccion->parametro1);
			ejecutar_resize(instruccion->parametro1);
			break;
		case COPY_STRING:
			log_info(logger_cpu, "PID: %d - Ejecutando COPY_STRING %d %d", PCB->PID, instruccion->parametro1);
			ejecutar_copy_string(PCB,instruccion->parametro1);
			break;
		case WAIT:
			log_info(logger_cpu, "PID: %d - Ejecutando WAIT %d %d", PCB->PID, instruccion->parametro1);
			ejecutar_wait(PCB, instruccion->parametro1);
			check_interrupt();
			break;
		case SIGNAL:
			log_info(logger_cpu, "PID: %d - Ejecutando SIGNAL %d %d", PCB->PID, instruccion->parametro1);
			ejecutar_signal(PCB, instruccion->parametro1);
			check_interrupt();
			break;
		case IO_GEN_SLEEP:
			log_info(logger_cpu, "PID: %d - Ejecutando IO_GEN_SLEEP %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2);
			ejecutar_io_gen_sleep(PCB, "IO_GEN_SLEEP", instruccion->parametro1, instruccion->parametro2);
			check_interrupt();
			break;
		case IO_STDIN_READ:
			log_info(logger_cpu, "PID: %d - Ejecutando IO_STDIN_READ %d %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3);
			ejecutar_io_stdin_read(instruccion->parametro1,instruccion->parametro2,instruccion->parametro3);
			check_interrupt();
			break;
		case IO_STDOUT_WRITE:
			log_info(logger_cpu, "PID: %d - Ejecutando IO_STDIN_READ %d %d %d", PCB->PID, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3);
			ejecutar_io_stdout_write(instruccion->parametro1,instruccion->parametro2,instruccion->parametro3);
			check_interrupt();
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
			check_interrupt();
			break;
		default:
			ejecutar_error(PCB);
			break;
	}
}

/* EXECUTE se ejecua lo correspodiente a cada instruccion */

void ejecutar_set(pcb* PCB, char* registro, char* valor){
	
	uint8_t valor8 = strtoul(valor, NULL, 10);
	uint32_t valor32 = strtoul(valor, NULL, 10);
	
	setear_registro(PCB, registro, valor8, valor32);

	es_exit =false; //poner en true 
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_mov_in(pcb* PCB, char* DATOS, char* DIRECCION){

	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s, %s]", PCB->pid, "MOV IN", DATOS, DIRECCION);
	/*
	SET EAX 30
	MOV_IN EBX EAX

    En EAX tengo guardada la dirección lógica 30
    La longitud a leer va a ser el tamaño de EBX, o sea 4
    Entonces tengo que leer desde el byte 30 hasta el 33
	*/
	uint32_t * direccion_logica = capturar_registro(DIRECCION);
	size_t size_reg = size_registro(DATOS);
	t_list * traducciones = obtener_traducciones(direccion_logica, size_reg);
	t_paquete * paquete = crear_paquete(LECTURA_MEMORIA); //no uso enviar_pcb porque no me sirve, necesito enviar una lista de cosas
	agregar_a_paquete(paquete,&size_reg,(int));
	empaquetar_traducciones(paquete,traducciones);
	enviar_paquete(paquete,fd_conexion_memoria);
	eliminar_paquete(paquete);
	
	if(size_reg == 4){
		setear_registro(PCB, DATOS, 0, recibir_lectura_memoria());
	}else if(size_reg == 1){
		setear_registro(PCB, DATOS, (uint8_t) recibir_lectura_memoria(), 0);
	}

	//log_info(logger_cpu, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %d", pcb->pid, direccion_fisica, valor);
	
	return;
}

void ejecutar_mov_out(pcb* PCB, char* DIRECCION, char* DATOS){
	
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s, %s]", PCB->pid, "MOV OUT", DIRECCION, DATOS);

	uint32_t * direccion_logica = capturar_registro(DIRECCION);
	uint32_t * datos = capturar_registro(DATOS);
	size_t size_reg = size_registro(DATOS);
	t_list * traducciones = obtener_traducciones(direccion_logica, size_reg);
	t_paquete * paquete = crear_paquete(ESCRITURA_MEMORIA);
	agregar_a_paquete(paquete,&size_reg,(int));
	agregar_a_paquete(paquete,datos,size_reg);
	empaquetar_traducciones(paquete,traducciones);
	enviar_paquete(paquete,fd_conexion_memoria);
	eliminar_paquete(paquete);
	//log_info(logger_cpu, "PID: %d - Accion: ESCRIBIR - Direccion Fisica: %d - Valor: %d", pcb->pid, direccion_fisica, valor);
	check_interrupt();

}

void ejecutar_sum(pcb* PCB, char* destinoregistro, char* origenregistro){
	
	size_t size_origen = 0, size_destino = 0;

	uint8_t destino8, origen8;
		
	uint32_t destino32, origen32;

	size_origen = size_registro(origenregistro);
	size_destino = size_registro(destinoregistro);

	if((size_origen == 1) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 + origen8;
	}else if((size_origen == 1) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 + (uint32_t) origen8;
	}else if((size_origen == 4) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 + (uint8_t) origen32;
	}else if((size_origen == 4) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 + origen32;
	}

	es_exit = false;  
	es_bloqueante = false; 
	return;
}

void ejecutar_sub(pcb* PCB, char* destinoregistro, char* origenregistro){

	size_t size_origen = 0, size_destino = 0;

	uint8_t destino8, origen8;
		
	uint32_t destino32, origen32;

	size_origen = size_registro(origenregistro);
	size_destino = size_registro(destinoregistro);

	if((size_origen == 1) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 - origen8;
	}else if((size_origen == 1) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen8 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 - (uint32_t) origen8;
	}else if((size_origen == 4) && (size_destino == 1)){
		* destino8 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino8 = destino8 - (uint8_t) origen32;
	}else if((size_origen == 4) && (size_destino == 4)){
		* destino32 = capturar_registro(PCB, destinoregistro);
		* origen32 = capturar_registro(PCB, origenregistro);
		destino32 = destino32 - origen32;
	}

	es_exit =false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_jnz(pcb* PCB, char* registro, char* valor){
		
	int * reg_value = capturar_registro (registro); //si es cero cualquier formaton numerico devuelve cero
	uint32_t pc_actualizado = 0;

	if (reg_value != 0){
		pc_actualizado = strtoul(valor, NULL, 10);
		PCB->PC = pc_actualizado;
	}
	
	es_exit=false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
	return;
}

void ejecutar_resize(pcb* PCB, char* tamanio){
	
	int ajuste_tamanio = atoi(tamanio);
	t_paquete * paquete = crear_paquete(REAJUSTAR_TAMANIO_PROCESO);
	agregar_a_paquete(paquete,&ajuste_tamanio,sizeof(int));
	enviar_paquete(paquete,fd_cpu_dispatch);
	eliminar_paquete(paquete);

	int codigo_operacion = recibir_operacion(fd_conexion_memoria);
	switch (codigo_operacion){
		case OUTOFMEMORY:
			enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, SIN_MEMORIA,NULL,NULL,NULL,NULL,NULL);
			es_exit = true;  //siempre modificar
			es_bloqueante = false; //modificar siempre que es_exit = false
			break;
		case OK:
			es_exit = false;  //siempre modificar
			es_bloqueante = false; //modificar siempre que es_exit = false
			break;
	}
	
	return;
}

void ejecutar_copy_string(pcb* PCB, char* tamanio){

	uint32_t reg_SI = PCB->registros.SI;
	uint32_t reg_DI = PCB->registros.DI;
	int copy_tamanio = atoi (tamanio);

	t_paquete * paquete = crear_paquete(COPY);
	agregar_a_paquete(paquete,&reg_SI,sizeof(uint32_t));
	agregar_a_paquete(paquete,&reg_DI,sizeof(uint32_t));
	agregar_a_paquete(paquete,&copy_tamanio,sizeof(int));
	enviar_paquete(paquete,fd_conexion_memoria);
	eliminar_paquete(paquete);

	return;
}

void ejecutar_wait(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "WAIT", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch, RECURSO, SOLICITAR_WAIT,NULL,NULL,NULL,NULL,NULL);
	free(recurso);
	sem_post(&sem_recibir_pcb);
	es_exit=false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
}

void ejecutar_signal(pcb* PCB, char* registro){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s - [%s]", PCB->PID, "SIGNAL", registro);
	char* recurso = malloc(strlen(registro) + 1);
	strcpy(recurso, registro);
	enviar_pcb(PCB, fd_escucha_dispatch, RECURSO, SOLICITAR_SIGNAL,NULL,NULL,NULL,NULL,NULL);
	free(recurso);
	//aca te falta esperar la respuesta del kernel
	es_exit = false;  //siempre modificar
	es_bloqueante=false; //modificar siempre que es_exit = false
}

void ejecutar_io_gen_sleep(pcb* PCB, char* instruccion, char* interfaz, char* unidad_de_tiempo){
	enviar_pcb(PCB, fd_escucha_dispatch, INTERFAZ, SOLICITAR_INTERFAZ_GENERICA, instruccion, interfaz, unidad_de_tiempo,NULL,NULL);
	sem_post(&sem_recibir_pcb);
	es_exit=false;  //siempre modificar
	es_bloqueante=true; //modificar siempre que es_exit = false
	es_wait = false;  //modificar si se pone a bloqueante = true
	es_resize = false; //modificar si se pone bloqueante = true
	return;
}

void ejecutar_io_stdin_read(char * nombre_interfaz, char * registro_direccion, char * registro_tamanio){
	
	/*int direccion_logica_i = atoi(registro_direccion);
	int tamanio_a_leer = atoi(registro_tamanio);
	*/ // son char pero palbras (osea EAX) igual registro tamaño

	uint32_t * direccion_logica_i = capturar_registro(registro_direccion);
	size_t tamanio_a_leer = size_registro(registro_tamanio);

	t_list * traducciones = obtener_traducciones(direccion_logica_i,tamanio_a_leer);
	
	t_paquete * paquete = crear_paquete(INTERFAZ); //no uso enviar_pcb porque no me sirve, necesito enviar una lista de cosas
	empaquetar_pcb(paquete, PCB, SOLICITAR_STDIN);
	empaquetar_recursos(paquete, PCB->recursos_asignados);

	agregar_a_paquete(paquete,"IO_STDIN_READ",strlen("IO_STDIN_READ")+1);
	agregar_a_paquete(paquete,nombre_interfaz,strlen(nombre_interfaz)+1);
	agregar_a_paquete(paquete,&tamanio_a_leer,sizeof(int));
	empaquetar_traducciones(paquete,traducciones);
	enviar_paquete(paquete,fd_cpu_dispatch);
	sem_post(&sem_recibir_pcb);
	eliminar_paquete(paquete);
	es_exit=false;  //siempre modificar
	es_bloqueante=true; //modificar siempre que es_exit = false
	es_wait = false;  //modificar si se pone a bloqueante = true
	es_resize = false; //modificar si se pone bloqueante = true

}

void ejecutar_io_stdout_write(char * nombre_interfaz, char * registro_direccion, char * registro_tamanio){
	/*int direccion_logica_i = atoi(registro_direccion);
	int tamanio_a_leer = atoi(registro_tamanio);
	*/ // son char pero palbras (osea EAX) igual registro tamaño

	uint32_t * direccion_logica_i = capturar_registro(registro_direccion);
	size_t tamanio_a_escribir = size_registro(registro_tamanio);

	t_list * traducciones = obtener_traducciones(direccion_logica_i,tamanio_a_escribir);
	
	t_paquete * paquete = crear_paquete(INTERFAZ); //no uso enviar_pcb porque no me sirve, necesito enviar una lista de cosas
	empaquetar_pcb(paquete, PCB, SOLICITAR_STDIN);
	empaquetar_recursos(paquete, PCB->recursos_asignados);
	agregar_a_paquete(paquete,"IO_STDOUT_WRITE",strlen("IO_STDOUT_WRITE")+1);
	agregar_a_paquete(paquete,nombre_interfaz,strlen(nombre_interfaz)+1);
	agregar_a_paquete(paquete,&tamanio_a_escribir,sizeof(int));
	empaquetar_traducciones(paquete,traducciones);
	enviar_paquete(paquete,fd_cpu_dispatch);
	sem_post(&sem_recibir_pcb);
	eliminar_paquete(paquete);
	es_exit=false;  //siempre modificar
	es_bloqueante=true; //modificar siempre que es_exit = false
	es_wait = false;  //modificar si se pone a bloqueante = true
	es_resize = false; //modificar si se pone bloqueante = true

}

void ejecutar_exit(pcb* PCB){
	log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, EXITO,NULL,NULL,NULL,NULL,NULL);
	sem_post(&sem_recibir_pcb);
	es_exit=true;  //siempre modificar
}

void ejecutar_error(pcb* PCB){ //se usa esta en algun momento? creo que no, 
	//log_info(logger_cpu, "PID: %d - Ejecutando: %s", PCB->PID, "EXIT");
	enviar_pcb(PCB, fd_escucha_dispatch, PCB_ACTUALIZADO, EXIT_CONSOLA,NULL,NULL,NULL,NULL,NULL);
	sem_post(&sem_recibir_pcb);
}


/* FUNCIONES PARA EL CALCULO ARITMETICOLOGICO */

size_t size_registro(char * registro){

	size_t SIZEEE;

	if(strcmp(registro, "AX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else if(strcmp(registro, "BX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else if(strcmp(registro, "CX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else if(strcmp(registro, "DX") == 0){
		SIZEEE = sizeof(uint8_t);
	} else  if(strcmp(registro, "EAX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "EBX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "ECX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "EDX") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "SI") == 0){
		SIZEEE = sizeof(uint32_t);
	} else if(strcmp(registro, "DI") == 0){
		SIZEEE = sizeof(uint32_t);
	}

	return SIZEEE;
}

void setear_registro(pcb * PCB, char * registro, uint8_t valor8, uint32_t valor32){
	if(strcmp(registro, "AX") == 0){
		PCB->registros.AX = valor8;
	} else if(strcmp(registro, "BX") == 0){
		PCB->registros.BX = valor8;
	} else if(strcmp(registro, "CX") == 0){
		PCB->registros.CX = valor8;
	} else if(strcmp(registro, "DX") == 0){
		PCB->registros.DX = valor8;
	}else if(strcmp(registro, "EAX") == 0){
		PCB->registros.EAX = valor32;
	} else if(strcmp(registro, "EBX") == 0){
		PCB->registros.EBX = valor32;
	} else if(strcmp(registro, "ECX") == 0){
		PCB->registros.ECX = valor32;
	} else if(strcmp(registro, "EDX") == 0){
		PCB->registros.EDX = valor32;
	} else if(strcmp(registro, "SI") == 0){
		PCB->registros.SI = valor32;
	} else if(strcmp(registro, "DI") == 0){
		PCB->registros.DI = valor32;
	}
	return;
}

void * capturar_registro(pcb * PCB, char * registro){
	
	if(strcmp(registro, "AX") == 0){
		return * PCB->registros.AX;
	} else if(strcmp(registro, "BX") == 0){
		return * PCB->registros.BX;
	} else if(strcmp(registro, "CX") == 0){
		return * PCB->registros.CX;
	} else if(strcmp(registro, "DX") == 0){
		return * PCB->registros.DX;
	}else if(strcmp(registro, "EAX") == 0){
		return * PCB->registros.EAX;
	} else if(strcmp(registro, "EBX") == 0){
		return * PCB->registros.EBX;
	} else if(strcmp(registro, "ECX") == 0){
		return * PCB->registros.ECX;
	} else if(strcmp(registro, "EDX") == 0){
		return * PCB->registros.EDX;
	} else if(strcmp(registro, "SI") == 0){
		return * PCB->registros.SI;
	} else if(strcmp(registro, "DI") == 0){
		return * PCB->registros.DI;
	}

}

/* FUNCIONES stdin_read y stout_write */

t_list * obtener_traducciones(uint32_t direccion_logica_i, int tamanio_a_leer ){ //cambio los tipos de datos de DL y tamanio por el siguiente ejemplo
	/*
	SET EAX 30 
	MOV_IN EBX EAX

    En EAX tengo guardada la dirección lógica 30
    La longitud a leer va a ser el tamaño de EBX, o sea 4
    Entonces tengo que leer desde el byte 30 hasta el 33
	*/
	/* EAX es uint32_t y el tamanio a leer es sizeof(EBX) y EBX es sizeof*/
	int direccion_logica_f = direccion_logica_i + tamanio_a_leer -1;

	int pagina_inicial = direccion_logica_i / tam_pagina;
	int pagina_final = direccion_logica_f / tam_pagina;

	// int numero_de_paginas_a_traducir = pagina_final - pagina_inicial + 1;
    t_list * lista_traducciones = list_create();
	//MMU (direccion logica) -> devuelve direccion fisica
	if (pagina_final != pagina_inicial){
		for (int i = pagina_inicial; i<=pagina_final;i++){
			if(i == pagina_inicial){
				nodo_lectura_escritura * traduccion_inicial = malloc(sizeof(nodo_lectura_escritura));
				int offset = direccion_logica_i % TAM_PAGINA; 
				traduccion_inicial->direccion_fisica = MMU(direccion_logica_i);
				traduccion_inicial->bytes = TAM_PAGINA - offset;
				list_add(lista_traducciones,traduccion_inicial);
			}else{
				if(i == pagina_final){
					nodo_lectura_escritura * traduccion_final = malloc(sizeof(nodo_lectura_escritura));
					int offset = direccion_logica_f % TAM_PAGINA; //ultimo byte que se escribe en ese marco
					traduccion_final->direccion_fisica = MMU(pagina_final*TAM_PAGINA); //se empieza a escribir desde la direccion logica que da inicio a esa pagina
					traduccion_final->bytes = offset+1; //por cuanto se escribe?
					list_add(lista_traducciones,traduccion_final);
				}else{
					nodo_lectura_escritura * traduccion_intermedia = malloc(sizeof(nodo_lectura_escritura));
					traduccion_intermedia->direccion_fisica = MMU(i*TAM_PAGINA);
					traduccion_intermedia->bytes = TAM_PAGINA;
					list_add(lista_traducciones,traduccion_intermedia);
				}
			}
		}
	}else{
		nodo_lectura_escritura * traduccion_inicial = malloc(sizeof(nodo_lectura_escritura));
		int offset = direccion_logica_i % TAM_PAGINA; 
		traduccion_inicial->direccion_fisica = MMU(direccion_logica_i);
		traduccion_inicial->bytes = TAM_PAGINA - offset;
		list_add(lista_traducciones,traduccion_inicial);
	}
	return lista_traducciones;
	//importante, esta funcion no libera la lista
}

uint32_t recibir_lectura_memoria(){
	t_list* paquete = recibir_paquete(fd_conexion_memoria);
	uint32_t lecturita = 0;
	uint32_t* lectura = list_get(paquete, 0);
	lecturita = *lectura;
	free(lectura);
	list_destroy(paquete);
	return lecturita;
}


/* TLB */

/* TRADUCCION LOGICA A FISICA */

int MMU(uint32_t direccion_logica){
	int marco;
	int numero_pagina = floor(direccion_logica / TAM_PAGINA); //ya de por si redondea para abajo, posiblemente floor sea innecesario
	int desplazamiento =  direccion_logica % TAM_PAGINA;

	switch (MAX_TLB_ENTRY)
	{
	case 0:
		marco = solicitar_info_memory(numero_pagina);
		break;
	default:
		marco = consultar_tlb(PCB->PID, numero_pagina);
		break;
	} //ya tenes el numero de marco

	log_info(logger_cpu,  "PID: %d - OBTENER MARCO - Página: %d - Marco: %d", PCB->pid, numero_pagina, marco); //log ob

	return marco *TAM_PAGINA + desplazamiento;
}

void inicializar_tlb(){

	translation_lookaside_buffer = list_create();
	MAX_TLB_ENTRY = atoi (cantidad_entradas_tlb);
	TAM_PAGINA = solicitar_tamanio_pagina();

}

int solicitar_tamanio_pagina(){ //vos le pedis tambien? si poque no tengo manera de saber el tamaño de pagina
	int tamanio;
	int codigo_operacion = recibir_operacion(fd_conexion_memoria);
	switch (codigo_operacion){
		case SIZE_PAGE:
			TAM_PAGINA = recibir_tamanio_pagina(fd_conexion_memoria);
			break;
	}		
	
	return tamanio;
}

int solicitar_info_memory(int numero_pagina){ 

	enviar_solicitud_marco(fd_conexion_memoria, PCB->PID, numero_pagina);
	return recibir_marco(fd_conexion_memoria);

}

void agregar_entrada_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria, pthread_mutex_t* mutex, int PID, int numero_pagina, int marco){

	info_proceso_memoria->pid = PID;
	info_proceso_memoria->num_pag = numero_pagina;
	info_proceso_memoria->marco = marco;

	push_con_mutex(TLB, info_proceso_memoria, &mutex_tlb);

}

void verificar_tamanio_tlb(t_list* TLB, pthread_mutex_t* mutex){

	if((list_size(TLB)/MAX_TLB_ENTRY) =! 1){
		return;
	}
	nodo_tlb* puntero = pop_con_mutex(TLB, mutex);
	free(puntero); 
	return;
}

bool es_entrada_TLB_de_PID(void * un_nodo_tlb ){
	nodo_tlb * nodo_tlb_c = (nodo_tlb *) un_nodo_tlb;
	return nodo_tlb_c->pid == PCB->PID;
}

int consultar_tlb(int PID, int numero_pagina){
	
	int marco;
	int posicion_elemento_buscado;
	nodo_tlb * info_proceso_memoria;
	
	if(list_is_empty(translation_lookaside_buffer)){ //TLB MISS PERO PORQUE LA TLB ESTA VACIA
		log_info(logger_cpu, "PID: %d - TLB MISS - Pagina: %d", PCB->PID, numero_pagina);
		marco = solicitar_info_memory(numero_pagina);
		agregar_entrada_tlb(translation_lookaside_buffer, info_proceso_memoria, &mutex_tlb, PID, numero_pagina, marco);
		return marco;
	}
	
	info_proceso_memoria = list_find(translation_lookaside_buffer,(void*)es_entrada_TLB_de_PID);

	if(info_proceso_memoria == NULL){ //TLB MISS
		log_info(logger_cpu, "PID: %d - TLB MISS - Pagina: %d", PCB->PID, numero_pagina);
		marco = solicitar_info_memory(PID, numero_pagina);
		info_proceso_memoria = administrar_tlb(PID, numero_pagina, marco); //devuelve si o si la nueva entrada
	}else{ //ESTO ES TLB HIT
		if(algoritmo_tlb == "LRU"){ 
			log_info(logger_cpu, "PID: %d - TLB HIT - Pagina:  %d", PCB->PID, numero_pagina);
			list_remove_element(translation_lookaside_buffer,info_proceso_memoria);
			list_add(translation_lookaside_buffer,info_proceso_memoria);
		}
	}

	log_info(logger_cpu, "PID: %d - OBTENER MARCO - Pagina: %d - Marco: %d", PCB->PID, numero_pagina, info_proceso_memoria->marco);

	return info_proceso_memoria->marco;
}

nodo_tlb * administrar_tlb( int PID, int numero_pagina, int marco){ //a revisar

	verificar_tamanio_tlb(translation_lookaside_buffer, &mutex_tlb); // si queda tamanio disponible, no hace nada, si no tiene mas espacio, quita el primero
	//si o si, tenes espacio para agregar otra entrada

	nodo_tlb * nueva_entrada = malloc(sizeof(nodo_tlb));

	agregar_entrada_tlb(translation_lookaside_buffer,nueva_entrada,&mutex_tlb,PID,numero_pagina,marco);
	
	return nueva_entrada;
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
			motivo_desalojo motivo = recibir_motiv_desalojo(fd_escucha_interrupt);
			interrupcion_actual = malloc(sizeof(motivo_desalojo));
			pthread_mutex_lock(&mutex_motivo_x_consola);
			if(!hay_interrupcion_x_consola){
				if(motivo == EXIT_CONSOLA){
					hay_interrupcion_x_consola = true;
					(*interrupcion_actual) = motivo; // motivo = EXIT_CONSOLA
				}else{
					(*interrupcion_actual) = motivo; // motivo = EXIT_CONSOLA
				}
			}
			pthread_mutex_unlock(&mutex_motivo_x_consola);
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

// yo se que es un quilombo de if y else, pero posta que si no mostras que hacer en todos los casos es un quilombo entenderlo
void check_interrupt (){
	if(interrupcion_actual!=NULL){
		if (!es_exit){
			if(*interrupcion_actual == FIN_QUANTUM){ //si la interrupcion es por desalojo de quantum
				if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
					if(error_memoria){ //
						log_info(logger_cpu,"Como hubo un error de escritura del proceso %d, ignoramos interrupcion de fin de quantum",PCB->PID);
						liberar_interrupcion_actual(); // no atendemos interrupcion ni ponemos ningun semaforo porque mov in y mov out ya desalojan y ponen a escuchar otro pcb
					}else{ // se sabe que no se desalojo al proceso previamente
						log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
						enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
						sem_post(&sem_recibir_pcb);
						liberar_interrupcion_actual();
					}
					
				}else{ //pudo haberse desalojado al proceso
					if(es_wait){
						if(cambio_proceso_wait){
							log_info(logger_cpu,"Cambio el PCB POR WAIT, ahora es del proceso %d, ignoramos interrupcion por FIN DE QUANTUM",PCB->PID);
							sem_post(&sem_execute); //el pcb cambiado ya lo recibimos, tenemos que simplemente ponernos a ejecutar otro ciclo de instruccion
							liberar_interrupcion_actual();
						}else{
							log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
							enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
							sem_post(&sem_recibir_pcb);
							liberar_interrupcion_actual();
						}
					}else{//no es wait, puede ser una syscall bloqueante o resize
						if(es_resize){ 
							if(resize_desalojo_outofmemory){
								log_info(logger_cpu,"Resize ya había desalojado al proceso, ignoramos interrupcion por FIN DE QUANTUM");
								liberar_interrupcion_actual(); // syscall bloqueante ya se encargo de poner a escuchar por otro pcb
							}else{
								log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
								enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
								sem_post(&sem_recibir_pcb);
								liberar_interrupcion_actual();
							}
						}else{ //este caso es las syscalls bloqueantes, ni resize ni wait
							log_info(logger_cpu,"Syscall bloqueante ya había desalojado al proceso %d, ignoramos interrupcion por FIN DE QUANTUM",PCB->PID);
							liberar_interrupcion_actual();// syscall bloqueante ya se encargo de poner a escuchar por otro pcb
						}	
					}
				}
			}else{//fin de proceso
				if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
					if(error_memoria){ //
						log_info(logger_cpu,"Como hubo un error de escritura del proceso %d, ignoramos el pedido de finalizacion porque ya va a finalizar por este error de escritura",PCB->PID);
						liberar_interrupcion_actual(); // no atendemos interrupcion ni ponemos ningun semaforo porque mov in y mov out ya desalojan y ponen a escuchar otro pcb
					}else{ // se sabe que no se desalojo al proceso previamente
						log_info(logger_cpu,"Interrupcion: Fin de Quantum para el proceso: %d",PCB->PID);
						enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,FIN_QUANTUM,NULL,NULL, NULL, NULL, NULL);
						sem_post(&sem_recibir_pcb);
						liberar_interrupcion_actual();
					}
				}else{ //pudo haberse desalojado al proceso
					if(es_wait){
						if(cambio_proceso_wait){ //se desalojo el proceso
							//avisarle a kernel del desalojo?
							liberar_interrupcion_actual();
							sem_post(&sem_execute);
						}else{ //no se desalojo el proceso
							log_info(logger_cpu,"Interrupcion: Finalizacion del proceso: %d",PCB->PID);
							enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,EXITO,NULL,NULL, NULL, NULL, NULL);
							sem_post(&sem_recibir_pcb);
							liberar_interrupcion_actual();
							pthread_mutex_lock(&mutex_motivo_x_consola);
							hay_interrupcion_x_consola = false;
							pthread_mutex_unlock(&mutex_motivo_x_consola);
						}
					}else{//no es wait, puede ser una syscall bloqueante o resize
						if(es_resize){ 
							if(resize_desalojo_outofmemory){
								//avisarle a kernel del desalojo?na, ya lo va a finalizar solito
								liberar_interrupcion_actual();
							}else{//resize no desalojo al proceso, atender interrupcion
								log_info(logger_cpu,"Interrupcion: Finalizacion del proceso: %d",PCB->PID);
								enviar_pcb(PCB,fd_escucha_dispatch,PCB_ACTUALIZADO,EXITO,NULL,NULL, NULL, NULL, NULL);
								sem_post(&sem_recibir_pcb);
								liberar_interrupcion_actual();
								pthread_mutex_lock(&mutex_motivo_x_consola);
								hay_interrupcion_x_consola = false;
								pthread_mutex_unlock(&mutex_motivo_x_consola);
							}
						}else{ //este caso es las syscalls bloqueantes, ni resize ni wait
							//avisarle a kernel del desalojo?
							liberar_interrupcion_actual();
						}	
					}
				}
			}
		}else{//es exit
			log_info(logger_cpu,"Instruccion ejecutada: EXIT. Proceso  ya había sido desalojado Entonces ignoramos la interrupcion");
			liberar_interrupcion_actual();
		}
	}else{//no hubo una interrupcion
		if(!es_exit){
			if(!es_bloqueante){ // se sabe que no se desalojo al proceso previamente
				if(error_memoria){
					//no hacer nada porque se supone que mov in o mov out ya se pusieron a escuchar otro pcb
				}else{// se sabe que no se desalojo al proceso previamente
					sem_post(&sem_execute);
				}
			}else{ //pudo haberse desalojado al proceso
				if(es_wait){
					if(cambio_proceso_wait){
						sem_post(&sem_execute);
					}else{
						sem_post(&sem_execute);	
					}
				}else{//no es wait, puede ser una syscall bloqueante o resize
					if(es_resize){ 
						if(resize_desalojo_outofmemory){
						//no hacer nada porque la resize ya pone a escuchar otro pcb despues de desalojar ya se había encargado de poner a escuchar otro pcb
						}else{
						sem_post(&sem_execute);
						}
					}else{ //este caso es las syscalls bloqueantes, ni resize ni wait
					//no hacer nada porque la syscall bloqueante ya se había encargado de poner a escuchar otro pcb
					}	
				}
			}
		}else{
			//no hacer nada porque exit ya directamente pone a escuchar otro pcb
		}
		
	}
}

void liberar_interrupcion_actual(){
	free(interrupcion_actual);
	interrupcion_actual = NULL;
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
