#include <../../memoria/include/memoria.h>
int main() {
    
    decir_hola("Memoria, verdad y justicia");

    logger_memoria = log_create("memoria_logs.log","memoria",1,LOG_LEVEL_INFO);
    config_memoria = config_create("./configs/memoria.config");
    leer_configuraciones();
	 //CHECKPOINT 3 lo que realice aca es crear la memoria de usuario 

	//iniciar_memoria_contigua();
	int mem = iniciarMemoria();

	if(!mem){
		return 0;
	}
    //iniciar servidor de memoria
    //CHECKPOINT 1 *************/
    fd_escucha_memoria = iniciar_servidor(NULL, puerto_propio,logger_memoria,"Memoria");
    
    log_info(logger_memoria, "Puerto de memoria habilitado para sus clientes");

    //***************************CHECKPOINT 2*************************+
    
    // Iniciar conexiones y esperar y creo lista de procesos
	
	while(server_escuchar(fd_escucha_memoria));

	// Termino programa
	terminar_programa(logger_memoria, config_memoria);
    
    //CHECKPOINT 3
	
}

/*
void iniciar_memoria_contigua(){
	user_space = malloc(tam_memoria); 
	cant_marcos = tam_memoria / tam_pagina;//siempre devuelve un numero entero, porque tam_memoria = tam_pagina * cant_marcos, por hipotesis del tp
	bitmap=malloc(dividir_y_redondear_hacia_arriba(cant_marcos,8)); //el tamaño del bitmap es la cantidad de bits que ocupará expresada en bytes
	frames_array = bitarray_create_with_mode(bitmap,dividir_y_redondear_hacia_arriba( cant_marcos , 8), LSB_FIRST); // simplemente apunto al bitmap como si fuera un array de bits
	//solo utilizar cant_marcos, user_space, frames_array
} //IMPORTANTE, la cantidad de elementos del bitarray a veces no termina siendo igual a la cantidad de marcos, redondeando para arriba nos aseguramos de que esta no sea menor
*/

int nro_de_marco_libre(){
	for(int i=0; i<cant_marcos; i++){
		bool valor;
		pthread_mutex_lock(&mutex_frames_array);
		valor=bitarray_test_bit(frames_array, (off_t) i);
		pthread_mutex_unlock(&mutex_frames_array);
		if(!valor)
			return i;
		
	}
	return -1; //simbolizamos el -1 como que no hay marcos disponibles ----> acá devolver OUT OF MEMORY
}

void inicializar_semaforos(){
	//pthread_mutex_init(&mutex_lista_new, NULL);
	pthread_mutex_init(&mutex_lista_procesos, NULL);
	pthread_mutex_init(&mutex_frames_array, NULL);
}

//Esta pensado para que en un futuro lea mas de una configuracion. 
void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_memoria,"PUERTO_PROPIO");
    tam_memoria= config_get_int_value(config_memoria, "TAM_MEMORIA");
    tam_pagina= config_get_int_value(config_memoria,"TAM_PAGINA");
    path_instrucciones=config_get_string_value(config_memoria,"PATH_INSTRUCCIONES");
    retardo_respuesta= config_get_int_value(config_memoria,"RETARDO_RESPUESTA");
}

void terminar_programa(){
    config_destroy(config_memoria);
    log_destroy(logger_memoria);
}
t_list * leer_pseudocodigo(char* ruta){ //tenía que devolver un puntero a lista
    
    t_list* instrucciones = list_create();
    FILE* f;
    char buffer[256]; 
    char* palabra;
    char* instruccion_leida= NULL;
    char* parametros[5];
    int contadordeparametros;

    f=fopen(ruta,"r");
	while (fgets(buffer, 256, f) != NULL) {
		t_linea_instruccion* instruccion = malloc(sizeof(cod_instruccion)); 
		instruccion->parametro1 = malloc(256);
		instruccion->parametro2 = malloc(256);
		instruccion->parametro3 = malloc(256);
		instruccion->parametro4 = malloc(256);
        instruccion->parametro5 = malloc(256);


		// Eliminar el carácter de salto de línea del final de la línea
		buffer[strcspn(buffer, "\n")] = '\0';

        instruccion_leida = strtok(buffer, " ");                                                             
        contadordeparametros = 0;
        while ((palabra = strtok(NULL, " ")) != NULL && contadordeparametros < 5) {
            parametros[contadordeparametros++] = palabra;
        }

        if(instruccion_leida != NULL){
            cod_instruccion cod_inst = instruccion_to_enum(instruccion_leida);

            instruccion->instruccion = cod_inst;
            if(contadordeparametros > 0){
            	strcpy(instruccion->parametro1, parametros[0]);
            }else{
            	strcpy(instruccion->parametro1, "");
            }
            if(contadordeparametros > 1){
            	strcpy(instruccion->parametro2, parametros[1]);
            }else{
            	strcpy(instruccion->parametro2, "");
            }
            if(contadordeparametros > 2){
            	strcpy(instruccion->parametro3, parametros[2]);
            }else{
            	strcpy(instruccion->parametro3, "");
            }
			if(contadordeparametros>3){
                strcpy(instruccion->parametro1,parametros[3]);
            }
            else{
                strcpy(instruccion->parametro1," ");
            }
          if(contadordeparametros>4){
                strcpy(instruccion->parametro1,parametros[4]);
            }
         else{
              strcpy(instruccion->parametro1," ");
            }
            
		list_add(instrucciones, instruccion);
        }else{
        	instruccion_destroyer(instruccion);
        }
	}

	fclose(f);

	return instrucciones;
}
void instruccion_destroyer(t_linea_instruccion* instruccion){
	free(instruccion);
	free(instruccion->parametro1);
	free(instruccion->parametro2);
	free(instruccion->parametro3);
	free(instruccion->parametro4);
	free(instruccion->parametro5);
}

//aca se le conectan primero cpu, despues kernel, y despues pueden conectarse MUULTIPLES interfaces
int server_escuchar() { 
	 
	int cliente_socket = esperar_cliente(fd_escucha_memoria, logger_memoria, "SOY UN CLIENTE");

	if (cliente_socket != -1) {
		pthread_t hilo;
		int *args = malloc(sizeof(int)); 
		args = &cliente_socket;
		pthread_create(&hilo, NULL, (void*) procesar_clientes, (void*) args);
		pthread_detach(hilo);
		return 1;
	}

	return 0;
}

//por que necesita static? no la invoca ningun otro archivo
static void procesar_clientes(void* void_args){ 
	int *args = (int*) void_args;
	int cliente_socket = *args;

	op_code cop;
	while (cliente_socket != -1) {
		if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
			log_info(logger_memoria, "El cliente se desconecto de memoria server"); 
			free(args);
			return;
		}
		switch (cop) {
		case MENSAJE:
			recibir_mensaje(logger_memoria, cliente_socket);
			//posiblemente haya que enviar una respuesta tambien para indicar que se recibio el mensaje
			break;
		case PAQUETE:
			t_list *paquete_recibido = recibir_paquete(cliente_socket);
			log_info(logger_memoria, "Recibí un paquete con los siguientes valores: ");
			list_iterate(paquete_recibido, (void*) iterator);
			break;	
		case DATOS_PROCESO: // CREAR PROCESO: este codigo SOLO LO ENVIA EL KERNEL 
			t_datos_proceso* datos_proceso = recibir_datos_del_proceso(cliente_socket);// por que esta en protocolo.h? si es una funcion que conoce solo la memoria, puede estar en memoria.h
			iniciar_proceso_a_pedido_de_Kernel(datos_proceso->path, datos_proceso->pid, cliente_socket);
			free(datos_proceso->path);
			free(datos_proceso);
			break;
		case SOLICITAR_INSTRUCCION:// ESTE CODIGO SOLO LO ENVÍA CPU
			log_info(logger_memoria, "Solicitud de instruccion recibida");
			procesar_pedido_instruccion(cliente_socket);
			break;
		case SOLICITUD_MARCO:
			log_info(logger_memoria, "Solicitud de marco"); //cuando sergio haga send_pedido_de_marco o como se iame
			procesar_solicitud_nromarco(cliente_socket);
			//recibir el paquete (si o si un pid y un numero de pagina)
			//invocar una funcion que busque un proceso por su pid, acceda a su tabla de paginas y devuelva el numero de marco asociado a un numero de pagina enviado por parametro
			break;
		case LECTURA_MEMORIA: //PARA STDOUT
			procesar_lectura_en_memoria(cliente_socket);
			break;
		case ESCRITURA_MEMORIA: //PARA STDIN
			procesar_escritura_en_memoria(cliente_socket);
			break;
		case REAJUSTAR_TAMANIO_PROCESO:
			procesar_reajuste_de_memoria(cliente_socket);
			break;
		case FINALIZAR_PROCESO:
			finalizar_proceso_a_pedido_de_kernel(cliente_socket); //si o si recibe el pid ahi adentro
			break;
		default:
			log_error(logger_memoria, "Codigo de operacion no reconocido en memoria");
			return;
			}

		}
	log_warning(logger_memoria, "El cliente se desconecto de %s server", "memoria");
	return;
}
cod_instruccion instruccion_to_enum(char* instruccion){
	if(strcmp(instruccion, "SET") == 0){
		return SET;
	} else if(strcmp(instruccion, "SUM") == 0){
		return SUM;
	} else if(strcmp(instruccion, "SUB") == 0){
		return SUB;
	} else if(strcmp(instruccion, "JNZ") == 0){
		return JNZ;
	} else if(strcmp(instruccion, "EXIT") == 0){
		return EXIT;
	} else if(strcmp(instruccion, "WAIT") == 0){
		return WAIT;
	} else if(strcmp(instruccion, "IO_STDIN_READ") == 0){
		return IO_STDIN_READ;
	} else if(strcmp(instruccion, "SIGNAL") == 0){
		return SIGNAL;

	} else if(strcmp(instruccion, "MOV_OUT") == 0){
		return MOV_OUT;
	} else if(strcmp(instruccion, "MOV_IN") == 0){
		return MOV_IN;
	} else if (strcmp(instruccion, "RESIZE") == 0){
		return RESIZE;
	} else if (strcmp(instruccion, "COPY_STRING") == 0){
		return COPY_STRING;
	}
     else if (strcmp(instruccion, "IO_STDOUT_WRITE") == 0){
		return IO_STDOUT_WRITE;
	}
     else if (strcmp(instruccion, "IO_FS_CREATE") == 0){
		return IO_FS_CREATE;
	}
     else if (strcmp(instruccion, "IO_FS_DELETE") == 0){
		return IO_FS_DELETE;
	}
     else if (strcmp(instruccion, "IO_FS_TRUNCATE") == 0){
		return IO_FS_TRUNCATE;
	}
     else if (strcmp(instruccion, "IO_FS_WRITE") == 0){
		return IO_FS_WRITE;
	}
     else if (strcmp(instruccion, "IO_FS_READ") == 0){
		return IO_FS_READ;
	}
	return EXIT_FAILURE;
}
void iniciar_proceso_a_pedido_de_Kernel(char* path, int pid, int socket_kernel) {
    // Construir la ruta completa del archivo  
    char* rutaCompleta = string_from_format("%s%s.txt",path_instrucciones ,path);

    // Generar instrucciones y cargarlas a la variable global PROCESO_INSTRUCCIONES
    t_list* instrucciones = leer_pseudocodigo(rutaCompleta);
    free(rutaCompleta);

    // Crear el objeto de proceso_instrucciones
    t_proceso* proceso_nuevo = malloc(sizeof(t_proceso));
    proceso_nuevo->pid = pid;
    proceso_nuevo->instrucciones = instrucciones;
	proceso_nuevo->tabla_de_paginas=list_create(); //solo crea la lista, pero arranca sin elementos ya que no tiene marcos asignados, se le agregan elementos del tipo fila_tabla_de_paginas

	push_con_mutex(lista_de_procesos, proceso_nuevo, &mutex_lista_procesos);
}


void finalizar_proceso_a_pedido_de_kernel(int un_fd){

	t_list * lista = recibir_paquete(un_fd);

	int * ppid = (int*) list_get(lista,0);
	int un_pid = *ppid;
	free(ppid);
	list_destroy(lista);

	
	t_proceso * proceso_a_finalizar = buscar_proceso_en_lista(un_pid);


	pthread_mutex_lock(&mutex_lista_procesos);
	bool b=list_remove_element(lista_de_procesos,(void*) proceso_a_finalizar); //lo removemos de la lista
	pthread_mutex_unlock(&mutex_lista_procesos);
	
	void liberar_fila_paginas(void * fila){
		t_pagina * fila_c = (t_pagina*) fila;
		int marco = fila_c->marco;
		pthread_mutex_lock(&mutex_frames_array);
		bitarray_clean_bit(frames_array, (off_t) marco); //marcamos cada marco como libre en el bitarray
		pthread_mutex_unlock(&mutex_frames_array);
		free(fila_c);
	};

	list_destroy_and_destroy_elements(proceso_a_finalizar->tabla_de_paginas, (void*) liberar_fila_paginas );
	list_destroy_and_destroy_elements(proceso_a_finalizar->instrucciones, (void*) instruccion_destroyer);
}

t_solicitud_instruccion* recv_solicitar_instruccion(int fd){
	t_list* paquete = recibir_paquete(fd);
	t_solicitud_instruccion* solicitud_instruccion_recibida = malloc(sizeof(t_solicitud_instruccion));

	int* pid = list_get(paquete, 0);
	solicitud_instruccion_recibida->pid = *pid;
	free(pid);

	int* program_counter = list_get(paquete, 1);
	solicitud_instruccion_recibida->program_counter = *program_counter;
	free(program_counter);

	list_destroy(paquete);
	return solicitud_instruccion_recibida;
}// RECIBE EL PEDIDO DE CPU, PID Y PROGRAM COUNTER

t_linea_instruccion* buscar_instruccion(int pid, int program_counter){ //no tiene sentido que lo pasees por parametro 
	
	int i = 0;
	
	pthread_mutex_lock(&mutex_lista_procesos);
	t_proceso* proceso_instr = list_get(lista_de_procesos, i);
	pthread_mutex_unlock(&mutex_lista_procesos);

	while(pid != proceso_instr->pid){
		i++;
		pthread_mutex_lock(&mutex_lista_procesos);
		proceso_instr = list_get(lista_de_procesos, i);
		pthread_mutex_unlock(&mutex_lista_procesos);
	}
	//tenias una funcion de la commons que te ahorraba todo este trabajo igual, se llama list_find, fijate en la funcion finalizar_proceso_a_pedido_de_kernel

	return list_get(proceso_instr->instrucciones, program_counter);
}//BUSCA LA INSTRUCCION EN LA LISTA QUE CREAMOS CUANDO KERNEL PIDIO INCIAR PROCESO, YA ESTA CREADA EN UNA VARIABLE GLOBAL



void send_proxima_instruccion(int filedescriptor, t_linea_instruccion* instruccion){
	t_paquete* paquete = crear_paquete(PROXIMA_INSTRUCCION);

	agregar_a_paquete(paquete, &(instruccion->instruccion), sizeof(cod_instruccion));
	agregar_a_paquete(paquete, instruccion->parametro1, strlen(instruccion->parametro1) + 1);
	agregar_a_paquete(paquete, instruccion->parametro2, strlen(instruccion->parametro2) + 1);
	agregar_a_paquete(paquete, instruccion->parametro3, strlen(instruccion->parametro3) + 1);
	agregar_a_paquete(paquete, instruccion->parametro4, strlen(instruccion->parametro4) + 1);
	agregar_a_paquete(paquete, instruccion->parametro5, strlen(instruccion->parametro5) + 1);

	enviar_paquete(paquete, filedescriptor);
	eliminar_paquete(paquete);
}//envia la instruccion pedida a CPU serializado

void procesar_pedido_instruccion(int socket_cpu){ //la lista de procesos es una variable global, no hace falta que la pases por parametro

	int retardo_respuesta = config_get_long_value(config_memoria, "RETARDO_RESPUESTA");
	t_solicitud_instruccion* solicitud_instruccion = recv_solicitar_instruccion(socket_cpu);

	t_linea_instruccion* instruccion_a_enviar = buscar_instruccion(solicitud_instruccion->pid, solicitud_instruccion->program_counter - 1);//es -1 xq como se va a llamar varias veces una vez que 
	free(solicitud_instruccion);
	usleep(retardo_respuesta*1000);//preguntar 
	send_proxima_instruccion(socket_cpu, instruccion_a_enviar);
}
/*************************************/
int iniciarMemoria(){
	
	idGlobal = 0;
	log_info(logger_memoria, "RAM: %d",tam_memoria);
    int control = iniciarPaginacion();
    
    return control; //DEVUELVE 0 SI FALLA LA ELEGIDA
}

int iniciarPaginacion(){ 
    
    //MEMORIA PRINCIPAL
    memoriaPrincipal = malloc(tam_memoria);

    if(memoriaPrincipal == NULL){
        //NO SE RESERVO LA MEMORIA
    	perror("MALLOC FAIL!\n");
        return 0;
    };
    
    cant_marcos = tam_memoria/ tam_pagina;
    
    log_info(logger_memoria,"Tengo %d marcos de %d bytes en memoria principal",cant_marcos,tam_pagina);
    
    data = asignarMemoriaBits(cant_marcos);//entero a bit
    
    if(data == NULL){
        
        perror("MALLOC FAIL!\n");
        return 0;
    }

    memset(data,0,cant_marcos/8);//pongo en cero
    frames_array = bitarray_create_with_mode(data, cant_marcos/8, MSB_FIRST);//creo el puntero al vector de bit que ya estan en cero

    return 1;
}
	
char* asignarMemoriaBits(int bits)//recibe bits asigna bytes
{
	char* aux;
	int bytes;
	bytes = bitsToBytes(bits);
	//printf("BYTES: %d\n", bytes);
	aux = malloc(bytes);
	memset(aux,0,bytes);
	return aux; 
}

int bitsToBytes(int bits){
	int bytes;
	if(bits < 8)
		bytes = 1; 
	else
	{
		double c = (double) bits;
		bytes = ceil(c/8.0);
	}
	
	return bytes;
}
void procesar_solicitud_nromarco(int fd1){
	pid_y_pag_de_cpu* valoresenbruto= recv_solicitud_marco(fd1);

	//buscar pagina en tdp
	t_pagina* pagina = buscar_pagina(valoresenbruto->pid, valoresenbruto->numero_pagina);//Obtengo puntero a la tabla de pagina en la lista de procesos.Uso el pid para encontrar el proceso y nro pagina para encontrar el nro marco
    // t_pagina es tabla que tiene un id del proceso, pagina y el marco correspondiente a esa pagina
	send_marco(fd1, pagina->marco);//devuelvo el NRO DE MARCO, HAY QUE HACER VALIDACION ? SI El proceso todavia no tiene marcos asignados ya que no hizo resize
	free(valoresenbruto); //excelente
}

t_pagina* buscar_pagina(int npid, int numero_pagina){
	//buscar proceso en tdps
	bool encontrar_pid(void* t) {
		t_proceso* un_proceso = (t_proceso*) t ;
        return un_proceso->pid == npid;
    };//lo hizo nico, y segun el compilador lo entiende aunque este en blanco, -SOy nico, lo usaste bien 
	t_proceso* proceso = list_find(lista_de_procesos,(void*) encontrar_pid);

	//buscar pagina en tdp
	t_pagina* pagina = list_get(proceso->tabla_de_paginas, numero_pagina);//aca obtengo puntero q apunta a la tabla de pagina alli obtengo marco
	// IMPORTANTE, para encontrar el t_pagina SE PUEDE usar list_get en vez de list_find ¿POR QUÉ? porque justo COINCIDE el numero de pagina con el numero de elemento en la lista o sea: PAGINA O -> PAGINA1 -> PAGINA 2 -> PAGINA 3 | OJO esto no es así con los marcos
	log_info(logger_memoria, "PID: %d - Pagina: %d - Marco: %d", npid, numero_pagina, pagina->marco);
	return pagina;
}
pid_y_pag_de_cpu* recv_solicitud_marco(int fd){
	t_list* paquete = recibir_paquete(fd);
	/*
	pid_y_pag_de_cpu* valores = list_get(paquete, 0); 
	list_destroy(paquete);
	return valores;

	esto SOLO no es valido, asi no lo envía el cpu, hay que recibir bien el paquete
	*/
	int * pid = (int *) list_get(paquete,0);
	int * n_pagina = (int *) list_get(paquete,1);
	list_destroy(paquete);

	pid_y_pag_de_cpu * valores = malloc(sizeof(pid_y_pag_de_cpu));
	valores->numero_pagina = *n_pagina;
	valores->pid = *pid;
	free(pid); //como ya guardamos los valores, borramos el puntero
	free(n_pagina); //como ya guardamos los valores, borramos el puntero
	
	return valores;
}
void send_marco (int fd, int marco){
	t_paquete* paquete = crear_paquete(MARCO);//preguntarel opcode que utiliza quienme pidio el nro de marco
	agregar_a_paquete(paquete, &marco, sizeof(int));
	enviar_paquete(paquete, fd);
	eliminar_paquete(paquete);
}   

void procesar_escritura_en_memoria(int cliente_socket){ //esto es para las interfaces
	
	//recibimos la data
	t_list * lista = recibir_paquete(cliente_socket);
	uint32_t * p_direccion_fisica = (uint32_t *) list_get(lista,0);
	uint32_t direccion_fisica =  *p_direccion_fisica;
	free(p_direccion_fisica);

	uint32_t * p_bytes = (uint32_t *) list_get(lista,1);
	uint32_t bytes = *p_bytes;
	free(p_bytes);

	void *a_escribir = list_get(lista,2);

	list_destroy(lista);

	memcpy(memoriaPrincipal+direccion_fisica,a_escribir,bytes); //OJO, nosotros no escribimos en memoria con caracter nulo, por eso no es strlen(string_a_escribir)+1

	free(a_escribir);

	char * rta = "Ok";
	t_paquete * paquete = crear_paquete(ESCRITURA_MEMORIA); //podría ser cualquier codigo de operacion, no me importa
	agregar_a_paquete(paquete,rta,sizeof(rta));
	enviar_paquete(paquete,cliente_socket);
	eliminar_paquete(paquete);
}

void procesar_lectura_en_memoria(int cliente_socket){
	//terminar durante ingles
	t_list * lista = recibir_paquete(cliente_socket);

	uint32_t * p_bytes = (uint32_t *) list_get(lista,0);
	uint32_t bytes = *p_bytes;
	free(p_bytes);
	uint32_t * p_direccion_fisica = (uint32_t *) list_get(lista,1);
	uint32_t direccion_fisica = *p_direccion_fisica;
	free(p_direccion_fisica);
	list_destroy(lista);

	char * buffer= malloc(bytes);
	memcpy(buffer,memoriaPrincipal+direccion_fisica,bytes);  //ojo, buffer sin '\0' al final

	t_paquete * paquete = crear_paquete(LECTURA_MEMORIA); //podría ser cualquier codigo de operacion, no me importa
	agregar_a_paquete(paquete,buffer,bytes);//no lo enviamos con el caracter nulo porque en memoria no se guarda el caracter nulo, STDOUT se encarga de agregar el '\0'
	enviar_paquete(paquete,cliente_socket);
	eliminar_paquete(paquete);
	free(buffer);
}


void procesar_reajuste_de_memoria(int un_fd){
	
	t_list * lista = recibir_paquete(un_fd);
	uint32_t * p_uint32_t = (uint32_t*) list_get(lista,0); 
	uint32_t bytes_finales = *p_uint32_t;
	free(p_uint32_t);
	int * p_int = (int*) list_get(lista,2); 
	int un_pid = *p_int;
	free(p_int);
	list_destroy(lista);

	t_proceso * proceso_reajustado = buscar_proceso_en_lista(un_pid);

	int cantidad_de_paginas_finales = divide_and_ceil(((int) bytes_finales),tam_pagina);
	int cantidad_de_paginas_actuales = list_size(proceso_reajustado->tabla_de_paginas);
	
	int diferencia_de_paginas = cantidad_de_paginas_finales - cantidad_de_paginas_actuales;
	
	if(diferencia_de_paginas<0){ //recortar proceso
		acortar_tamanio_proceso(un_fd,proceso_reajustado,diferencia_de_paginas);
	}else{
		switch(diferencia_de_paginas){
			case 0:
				enviar_operacion(un_fd,OK);
				break;
			default:
				aumentar_tamanio_proceso(un_fd,proceso_reajustado,diferencia_de_paginas);
				break;
		}
	}
}

t_proceso *buscar_proceso_en_lista(int un_pid){
	
	bool es_proceso_con_pid(void * un_proceso){
		t_proceso * un_proceso_c = (t_proceso *) un_proceso;
		return un_proceso_c->pid == un_pid;
	};//no importa que esta en blanco es un tema del editor de texto, el compilador lo va a poder compilar

	pthread_mutex_lock(&mutex_lista_procesos);
	t_proceso * proceso_encontrado = list_find(lista_de_procesos, (void*)es_proceso_con_pid);
	pthread_mutex_unlock(&mutex_lista_procesos);

	return proceso_encontrado;

}

int divide_and_ceil(int numerator, int denominator) { //divide y redondea para arriba
    // Convertir los enteros a double, dividir y aplicar ceil, luego convertir de vuelta a int
    return (int)ceil((double)numerator / (double)denominator);
}

void acortar_tamanio_proceso(int un_fd,t_proceso * proceso_reajustado,int diferencia_de_paginas){
	int paginas_a_quitar = diferencia_de_paginas * (-1); //diferencia de paginas es negativo bro
	t_list * tabla_de_paginas = proceso_reajustado->tabla_de_paginas;
	int ultima_pagina = list_size(tabla_de_paginas)-1; //te dice cual es la ultima pagina(ultimo index), no cantidad de paginas

	for (int i=0;i<paginas_a_quitar;i++){
		t_pagina * fila = list_remove(tabla_de_paginas,ultima_pagina-i);
		pthread_mutex_lock(&mutex_frames_array);
		bitarray_clean_bit(frames_array,(off_t)(fila->marco)); //libero marco
		pthread_mutex_unlock(&mutex_frames_array);
		free(fila);
	}

	enviar_operacion(un_fd,OK);
	return;
}

void aumentar_tamanio_proceso(int un_fd,t_proceso * proceso_reajustado,int paginas_a_agregar){

	if(cantidad_de_marcos_disponibles()<paginas_a_agregar){
		enviar_operacion(un_fd,OUTOFMEMORY);
	}else{
		int ultima_pagina=list_size(proceso_reajustado->tabla_de_paginas)-1;
		int bits_encontrados=0;
		pthread_mutex_lock(&mutex_frames_array);
		for (int index=0;index<paginas_a_agregar;index++){
			if(!bitarray_test_bit(frames_array,(off_t)index)){
				bits_encontrados++;
				bitarray_set_bit(frames_array,(off_t)index);
				t_pagina * nueva_pagina = malloc(sizeof(t_pagina));
				nueva_pagina->numpag = ultima_pagina+1+bits_encontrados;
				nueva_pagina->marco = index;
				list_add(proceso_reajustado->tabla_de_paginas,nueva_pagina);
			}			
		}
		pthread_mutex_unlock(&mutex_frames_array);
		enviar_operacion(un_fd,OK);
	}
	return;
}

int cantidad_de_marcos_disponibles(){
	int cantidad_de_marcos_disponibles=0;
	pthread_mutex_lock(&mutex_frames_array);
	for (int index=0;index<(cant_marcos-1);index++){
		if(!bitarray_test_bit(frames_array,(off_t)index))
			cantidad_de_marcos_disponibles++;
	}
	pthread_mutex_unlock(&mutex_frames_array);
	return cantidad_de_marcos_disponibles;
}



