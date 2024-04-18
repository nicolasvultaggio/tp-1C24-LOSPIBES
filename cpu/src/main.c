#include <../../cpu/include/main.h>

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    //CREO CONEXION CPU A MEMORIA Y RECIBO MENSAJE//

    logger = log_create("cpu_logs.log","cpu",1,LOG_LEVEL_INFO);
    config1 = config_create("./configs/cpu.config");

    leer__configuraciones();
    int fdcpu;//filedescriptorCPU
    fdcpu= crear_conexion(ip__memoria,puerto__memoria);

    if(!fdcpu){
        log_error(logger,"Alguna conexion esta tirando error");
        terminar_programa();
        exit(2);
    }

    enviar_operacion(fdcpu, &handshakeDeCPU);

    char mensaje1[1024] = {0};
    recv(fdcpu, mensaje1, 1024, 0);
    printf("Respuesta del servidor: %s\n", mensaje1);    

    close(fdcpu);

    

    //ABRIR CPU COMO SERVIDOR DE KERNEL(unico cliente)//
    logger__memoria = log_create("cpu_logs.log","LOG",1,LOG_LEVEL_INFO);
    config__memoria = config_create("./configs/cpu.config");
    leer__configuraciones();
    
    int socket_server_CPU = iniciar_servidor(NULL,puerto__propio);
    
    log_info(logger__memoria, "Puerto de CPU habilitado para su UNICO cliente");

    
    while ((socket_cliente_delKERNEL = esperar_cliente(socket_server_CPU)) != (-1))
    {
        int cod_oop = recibir_operacion(socket_cliente_delKERNEL);
        if(cod_oop==5){
        log_info(logger__memoria,"Se conecto KERNEL(unico cliente)");
        enviar_mensaje_de_exito(socket_cliente_delKERNEL, "HOLA KERNEL!! SOY CPU");
        }
        
        
    }
    terminar_programa();
    return 0;
  

}

void leer__configuraciones(){
    ip__propio= config_get_string_value(config1,"IP__ESCUCHA");
    puerto__propio = config_get_string_value(config1,"PUERTO__ESCUCHA");
    ip__memoria = config_get_string_value(config1,"IP__MEMORIA");
    puerto__memoria = config_get_string_value(config1,"PUERTO__MEMORIA");

    return;

   }


void terminar_programa(){
    if(logger != NULL){
		log_destroy(logger);
	}
	if(config1 != NULL){
		config_destroy(config1);
	}
    return;
}