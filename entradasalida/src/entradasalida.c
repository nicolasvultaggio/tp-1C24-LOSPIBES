#include <../include/entradasalida.h>

int main(int argc, char* argv[]) {
    decir_hola("la interfaz de I/Os");

    logger_io= log_create("entradasalida_logs.log","entradasalida",1,LOG_LEVEL_INFO); // crea el puntero al log
    config_io = config_create("./entradasalida.config"); // crea el puntero al archivo de config
    
    leer_configuraciones();//eso, guarda toda la info necesaria del archivo de las configuraciones
    
    
    fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
    
    fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
    

    //atender a Kernel-Dispatch
    args_atendedor * args_kernel = crear_args(fd_conexion_kernel,logger_io,"Kernel");
    pthread_t hilo_kernel;
    pthread_create(&hilo_kernel,NULL,(void*)atender_conexion,(void *)args_kernel);
    pthread_detach(hilo_kernel);

    //atender a Kernel-Interrupt
    args_atendedor * args_memoria = crear_args(fd_conexion_memoria,logger_io,"Memoria");
    pthread_t hilo_memoria;
    pthread_create(&hilo_memoria,NULL,(void*)atender_conexion,(void *)args_memoria);
    pthread_join(hilo_memoria,NULL);

    /*
     if(!iniciar_conexiones()){
        log_error(logger_io,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }
    enviar_operacion(fd_conexion_client_memoria, handshakeIO);
    char mensaje_ok_memoria[1024] = {0};
    recv(fd_conexion_client_memoria, mensaje_ok_memoria, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje_ok_memoria);

    //IO-KERNEL
    enviar_operacion(fd_conexion_client_kernel,handshakeIO);
    char mensaje_ok_kernel[1024] = {0};
    recv(fd_conexion_client_kernel, mensaje_ok_kernel, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje_ok_kernel);
    */
   

    //IO - MEMORIA
    

    terminar_programa();

    return 0;
}

void leer_configuraciones(){
    ip_memoria = config_get_string_value(config_io,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_io,"PUERTO_MEMORIA");
    ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
    puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
}

bool iniciar_conexiones(){
    fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
    fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");

    return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
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

