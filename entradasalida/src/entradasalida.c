#include <../include/entradasalida.h>

int main(int argc, char* argv[]) {

    // IMPORTANTE: ANTES DE LAS PRUEBAS: NO OLVIDAR ()
    //      ingresar en el launch.json los posibles argumentos que puede 
    //      tener el main cada vez que se inicia una interfaz, basicamente,
    //      agregar a "args" los diferentes PATHS de archivos de configuracion
    //      para que puedan realizarse las PRUEBAS. cualquier duda: arg en main en doc utn

    // FIN DEL COMUNICADO CAPO SEGUI CON LO TUYO

    if(argc !=3){ // verifica que los parametros sean 3, el modulo (el .c), el nombre de interfaz y su archivo de configuracion
        printf("Ingreso incorrecto de parametros");
        return 1;
    }
    
    nombre_de_interfaz = argv[1]; //argv[0] es el comando en sí, por eso no se usa
    path_configuracion = argv[2];
    decir_hola(nombre_de_interfaz);

    logger_obligatorio = log_create("../../utils/logs_obligatorios.log",nombre_de_interfaz,1,LOG_LEVEL_INFO); // todos los modulos modifican este archivo de logger, por eso en utils
    logger_io= log_create("entradasalida_logs.log",nombre_de_interfaz,1,LOG_LEVEL_INFO); // crea el puntero al log
    config_io = config_create(path_configuracion); // crea el puntero al archivo de config
    
    leer_configuraciones();//eso, guarda toda la info necesaria del archivo de las configuraciones
    
    if(iniciar_conexiones()){
        log_info(logger_io,"Error al iniciar conexión al Kernel");
        terminar_programa();
        exit (2);
    }

    //Ponerse a esperar solicitudes de Kernel

    atender_kernel(); //hilo principal
    //la validacion de la instruccion la hace el kernel, una io recibe la operacion solo si el kernel verificó que la conoce => no hace falta verificarla
    //en principio, GEN, STDIN, STDOUT siempre escuchan a kernel constantemente, para devolver respuesta, iniciativa de operacion nunca de interfaz
    // GEN: NO SE COMUNICA CON MEMORIA -- no me preocupo por eso, solo espera un tiempillo
    // STDIN: se recibe de kernel la indicacion de ponerse a leer algo por teclado
    //        eso ingresado tendra un tamaño (que tambien se recibe por parametro)
    //        debe enviarle a memoria eso que se leyó para guardar en la direccion logica indicada
    //         ==> envia algo a memoria pero no necesita una respuesta, solo dice: guardalo en tal lado
    // STDOUT: se conecta a memoria para obtener el contenido de una direccion fisica, memoria debe devolverle ese valor
    //         y se imprime ese valor por pantalla, en esta interfaz
    //         ==> envia algo a memoria y espera su respuesta
    // IMPORTANTE: que se le DEVUELVE al kernel como respuesta? un flag diciendole que ya se hizo la operacion.
    
    terminar_programa();

    return 0;
}

void leer_configuraciones(){

   tipo_de_interfaz = config_get_string_value(config_io,"TIPO_INTERFAZ");
   if(!strcmp(tipo_de_interfaz,"Generica")){
        type_interfaz = GENERICA;
    }else if(!strcmp(tipo_de_interfaz,"STDIN")){
        type_interfaz = STDIN;
    }else if(!strcmp(tipo_de_interfaz,"STDOUT")){
        type_interfaz = STDOUT;
    }else if(!strcmp(tipo_de_interfaz,"DialFS")){
        type_interfaz = DIALFS;
    }else{
        log_info(logger_io,"Tipo de interfaz desconocido");
    }    
    
    switch(type_interfaz){
        case GENERICA:
            tiempo_unidad_trabajo = atoi(config_get_string_value(config_io,"TIEMPO_UNIDAD_TRABAJO"));
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        case STDIN:
            ip_memoria = config_get_string_value(config_io,"IP_MEMORIA");
            puerto_memoria = config_get_string_value(config_io,"PUERTO_MEMORIA");
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        case STDOUT:
            tiempo_unidad_trabajo = atoi(config_get_string_value(config_io,"TIEMPO_UNIDAD_TRABAJO"));
            ip_memoria = config_get_string_value(config_io,"IP_MEMORIA");
            puerto_memoria = config_get_string_value(config_io,"PUERTO_MEMORIA");
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        case DIALFS:
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        default:
            log_info(logger_io,"Tipo de interfaz desconocido");
    }
    
}

bool iniciar_conexiones(){
    
   switch(type_interfaz){
        case GENERICA:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            return fd_conexion_kernel != (-1);
            break;
        case STDIN:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
            return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
            break;
        case STDOUT:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
            return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
            break;
        case DIALFS:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
            return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
            break;
        default:
            log_info(logger_io,"Tipo de interfaz desconocido");
            return false;
    }
    return false; 
}

void atender_kernel(){
    int una_operacion = 0;
    do{
        una_operacion = recibir_operacion(fd_conexion_kernel,logger_io,"Kernel"); // se supone que solo recibe la operacion
        switch(una_operacion){
            case (INFORMAR_NOMBRE):
                informar_nombre(); //luego el kernel tiene que recibir, codigo nombre informado, el tamaño del nombre, para guardarlo, y el puntero al nombre
                break;
            case (INFORMAR_TIPO):
                send(fd_conexion_kernel,&type_interfaz,sizeof(int),0);
                break;
            case (INSTRUCCION):
                atender_instruccion(); // luego el kernel solo debe recibir la notificacion de la operacion realizada
                break;
            default:
                log_info(logger_io,"Operacion de Kernel NO reconocida"); 
                break;
        }
    }while(una_operacion != (-1));
    log_info(logger_io,"Finalizo la atencion al Kernel, desconectando...DONE.");
}

void informar_nombre(){ // similar a enviar_mensaje tp0, hay que informar el nombre de la interfaz, el cual es un string de tamaño variable, por eso hay que informar tambien su tamaño
    //send(fd_conexion_kernel,nombre_de_interfaz,strlen(nombre_de_interfaz),0); es una solucion muy simple, hay que serializarlo un poco para que kernel reciba además el tamaño, si lo dejo asi, el kernel no sabe con cuantos bytes guardar el mensaje
    t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = NOMBRE_INFORMADO;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(nombre_de_interfaz) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, nombre_de_interfaz, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(fd_conexion_kernel, a_enviar , bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	
} //podría hacerse en modo paquete? creo que podría simplificarse a igual que informar_tipo

void atender_instruccion(){
    switch(type_interfaz){
        case GENERICA:
            atender_GENERICA();
            break;
        case STDIN:
            atender_STDIN();
            break;
        case STDOUT:
            atender_STDOUT();
            break;
        case DIALFS:
            atender_DIALFS();
            break;
    }
}
//no va a funcionar asi
char * recibir_unidades_de_tiempo(){ // similar a recibir mensaje del tp0
    int size;
    char * buffer = recibir_buffer(&size,fd_conexion_kernel); //cuando se envie una peticion desde kernel: primero codop, despues unidad de tiempo
    log_info(logger_io, "Kernel me dijo que tengo que esperar << %s >> unidades de tiempo", buffer);
    // OJO HAY QUE LIBERAR EL BUFFER, o sea, habra que liberar la instruccion luego, SI NO --> MEMORY LEAK, ------ listo, resuelto con el free(unidades_de_tiempo)
    return buffer;
}

void atender_GENERICA(){// IO_GEN_SLEEP (Interfaz, Unidades de trabajo), no hace falta validar instruccion, ya se supone que la valido el kernel, y ademas para esta interfaz 
    char * unidades_de_tiempo = recibir_unidades_de_tiempo(); // instruccion queda en memoria dinamica
    int tiempo_a_esperar = tiempo_unidad_trabajo * atoi(unidades_de_tiempo);
    usleep((__useconds_t) tiempo_a_esperar);
    avisar_operacion_realizada_kernel(); //esto va a servir para que el kernel procese vuelta a blocked y pase además, otro proceso pueda usar la interfaz
    free(unidades_de_tiempo); // hay que liberarla, ya que como se menciono anteriormente, la instruccion estaba en memoria dinamica
}

void atender_STDIN(){
   // char * instruccion = recibir_instruccion_de_kernel(); // instruccion queda en memoria dinamica
    // hace falta validar la instruccion? creo que eso es trabajo del kernel
    // se supone que la instruccion es valida, y para este tipo unica
   // free(instruccion);
}
void atender_STDOUT(){
    //char * instruccion = recibir_instruccion_de_kernel();
    // hace falta validar la instruccion? creo que eso es trabajo del kernel
    // se supone que la instruccion es valida, y para este tipo unica
    //free(instruccion);
}
void atender_DIALFS(){ // hoy, 2/5/2024, a las 20:07, esuchando JIMMY FALLON de Luchito, empiezo la funcion mas dificil de las interfaces, suerte loko
   // char * instruccion = recibir_instruccion_de_kernel();
    //free(instruccion);
}

void avisar_operacion_realizada_kernel(){
    int a=1;
    send(fd_conexion_kernel, &a,sizeof(int),0); //ponele que esa notificacion es simplemente recibir el numero 1, al menos para las genericas, por ahora
}

void terminar_programa()
{
	if(logger_io != NULL){
		log_destroy(logger_io);
	}
	if(config_io != NULL){
		config_destroy(config_io);
	}
    liberar_conexion(fd_conexion_kernel);
    liberar_conexion(fd_conexion_memoria);
}


