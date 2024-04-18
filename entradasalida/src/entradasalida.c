#include <../include/entradasalida.h>

int main(int argc, char* argv[]) {
    decir_hola("la interfaz de I/Os");

    logger_io= log_create("entradasalida_logs.log","entradasalida",1,LOG_LEVEL_INFO); // crea el puntero al log
    config_io = config_create("./entradasalida.config"); // crea el puntero al archivo de config
    
    leer_configuraciones();//eso, guarda toda la info necesaria del archivo de las configuraciones

    if(!iniciar_conexiones()){
        log_error(logger_io,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }

    //IO - MEMORIA
    enviar_operacion(socket_cliente_a_memoria, &handshake_de_memoria);
    char mensaje_ok_memoria[1024] = {0};
    recv(socket_cliente_a_memoria, mensaje_ok_memoria, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje_ok_memoria);

    //IO-KERNEL
    enviar_operacion(socket_cliente_a_kernel,&handshake_de_kernel);
    char mensaje_ok_kernel[1024] = {0};
    recv(socket_cliente_a_kernel, mensaje_ok_kernel, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje_ok_kernel);


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
    socket_cliente_a_kernel = crear_conexion(ip_kernel,puerto_kernel);
    socket_cliente_a_memoria = crear_conexion(ip_memoria,puerto_memoria);

    return socket_cliente_a_kernel != -1 && socket_cliente_a_memoria != -1; 
}

void terminar_programa()
{
	if(logger_io != NULL){
		log_destroy(logger_io);
	}
	if(config_io != NULL){
		config_destroy(config_io);
	}
    liberar_conexion(socket_cliente_a_kernel);
    liberar_conexion(socket_cliente_a_memoria);
}