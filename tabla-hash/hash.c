#define _POSIX_C_SOURCE 200809L
#include "hash.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#define MITAD 2
#define FACTOR_AGRANDAR 1
#define FACTOR_DISMINUIR 2
enum { VACIO, LLENO, BORRADO} ESTADO_T;
enum {AGRANDAR= true, ACHICAR= false};
const int capacidad_tabla[] = {57, 107, 211, 409, 809, 1511, 3011, 6029,
                            12511, 25013, 50021, 100019, 200009, 400009,
                            800011, 1500007, 3000017};


typedef struct campo {
    int estado;
    char* clave;
    void* dato;
} campo_t;

typedef struct hash_iter{
    campo_t* campo;
    int cantidad;
    int cont;
    int pos;
} hash_iter_t;

typedef struct  hash {
    campo_t* campo;  
    int cantidad; 
    int capacidad; 
    int borrados;
    hash_destruir_dato_t destruir_dato;
} hash_t;


//https://stackoverflow.com/questions/7666509/hash-function-for-string
int funcion_hash(const char *clave){
	int p=0;
	int i;
	for(i=0; i<strlen(clave); i++){
		p=p*31+clave[i];
	}
	p = abs(p);
	return p;
}

int hashing(const char *clave, int capacidad){
    return funcion_hash(clave) % capacidad;
}

void inicializar_campos(campo_t* campo, int capacidad){
    for(int i=0; i<capacidad; i++) campo[i].estado = VACIO;
}

int buscar_redimension(campo_t* campo, const char *clave, int capacidad) {
    int indice = hashing(clave, capacidad);
    int i = 1;
    while (campo[indice].estado == LLENO && i<capacidad) {
        indice += i;
        i += 2;        
        if (indice >= capacidad) indice -= capacidad;
    }
    return indice;
}

void guardar(campo_t* campo, char* clave, void* dato, int capacidad){
    int indice = buscar_redimension(campo, clave, capacidad);
    campo[indice].clave = clave;
    campo[indice].dato = dato;
    campo[indice].estado = LLENO;
}

void reubicarDatos(hash_t *hash, campo_t* campo, int capacidad){
    int a_visitar = hash->cantidad;
    for(int i=0; a_visitar; i++){
        if ( hash->campo[i].estado == LLENO ) {
            guardar(campo,hash->campo[i].clave, hash->campo[i].dato, capacidad_tabla[capacidad]);
            a_visitar--;
        }
    }
    free(hash->campo);
    hash->campo = campo;
    hash->capacidad = capacidad;
    hash->borrados = 0;
}

int buscar(campo_t* campo, const char *clave, int capacidad) {
    int indice = hashing(clave, capacidad);
    int i = 1;
    while ( campo[indice].estado != VACIO && i<capacidad) {
        if ( (campo[indice].estado ==LLENO) && !(strcmp(campo[indice].clave, clave)) ) {
            break;
        }
        indice += i;
        i += 2;
        if (indice >= capacidad) indice -= capacidad;
    }
    return indice;
}

bool redimensionar (hash_t *hash, bool tam) {
    int capacidad = tam ? hash->capacidad+FACTOR_AGRANDAR : hash->capacidad-FACTOR_DISMINUIR;
    if (capacidad<0) return false;
    campo_t* campo = malloc(sizeof(campo_t)*capacidad_tabla[capacidad]);
    
    if(!campo) return false;
    
    inicializar_campos(campo, capacidad_tabla[capacidad]);   
    reubicarDatos(hash, campo, capacidad);

    return true;
}

hash_t *hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* hash = malloc (sizeof(hash_t));
    if(!hash) return NULL;
    campo_t* campo = malloc(sizeof(campo_t)*capacidad_tabla[0]);
    if(!campo){
        free(hash);
        return NULL;
    }
    inicializar_campos(campo, capacidad_tabla[0]);
    hash->campo = campo;
    hash->cantidad = 0;
    hash->borrados = 0;
    hash->capacidad = 0;
    hash->destruir_dato = destruir_dato;
    
    return hash;
}

bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
    int indice = buscar(hash->campo, clave, capacidad_tabla[hash->capacidad]);
    if (hash->campo[indice].estado == LLENO) {
        if (hash->destruir_dato) {
            hash->destruir_dato(hash->campo[indice].dato);
        }
        hash->campo[indice].dato = dato;
        return true;
    }
    hash->campo[indice].clave = strdup(clave);
    hash->campo[indice].dato = dato;
    hash->campo[indice].estado = LLENO;
    hash->cantidad++;
    
    if ( (hash->cantidad  + hash->borrados ) >= (capacidad_tabla[hash->capacidad]/MITAD) ) redimensionar(hash, AGRANDAR);
    
    return true;
}

void *hash_borrar(hash_t *hash, const char *clave) {
    int indice = buscar(hash->campo, clave, capacidad_tabla[hash->capacidad]);

    if ( hash->campo[indice].estado != LLENO ) return NULL;
    
    free(hash->campo[indice].clave);

    void* dato = hash->campo[indice].dato;
    hash->campo[indice].dato = NULL;
    hash->campo[indice].estado = BORRADO;
    hash->cantidad--;
    hash->borrados++;
    return dato;

    if ( (hash->cantidad  + hash->borrados )*8 <= (capacidad_tabla[hash->capacidad]) ) redimensionar(hash, ACHICAR);
}

bool hash_pertenece(const hash_t *hash, const char *clave) {
    return hash->campo[buscar(hash->campo, clave, capacidad_tabla[hash->capacidad])].estado == LLENO;
}

size_t hash_cantidad(const hash_t *hash){
    return hash->cantidad;
}

void hash_destruir(hash_t *hash){
    int a_visitar = hash->cantidad;
    for(int i=0; a_visitar; i++){
        if(hash->campo[i].estado == LLENO){
            if( hash->destruir_dato) hash->destruir_dato(hash->campo[i].dato);
            free(hash->campo[i].clave);
            a_visitar--;
        }
    }
    free(hash->campo);
    free(hash);
}

void *hash_obtener(const hash_t *hash, const char *clave) {
    int indice = buscar(hash->campo, clave, capacidad_tabla[hash->capacidad]);
    return hash->campo[indice].estado == LLENO ? hash->campo[indice].dato : NULL;
}

hash_iter_t *hash_iter_crear(const hash_t *hash) {
    hash_iter_t* it = malloc(sizeof(hash_iter_t));
    if(!it || !hash ) return NULL;

    it->campo = hash->campo;
    it->cantidad = hash->cantidad;
    it->pos = -1;
    it->cont = 1;

    if (hash->cantidad ) while(it->campo[++it->pos].estado != LLENO );
    return it; 
}

bool hash_iter_avanzar(hash_iter_t *iter) {
    if(iter->cont + 1 <= iter->cantidad ) {
        while(iter->campo[++iter->pos].estado != LLENO );
        iter->cont++;
        return true;
    }
    iter->cont ++;
    return false;
}

bool hash_iter_al_final(const hash_iter_t *iter){
    return iter->cont > iter->cantidad; 
}

const char *hash_iter_ver_actual(const hash_iter_t *iter){
    return iter->cont > iter->cantidad ? NULL : iter->campo[iter->pos].clave;
}

void hash_iter_destruir(hash_iter_t* iter){
    free(iter);
}