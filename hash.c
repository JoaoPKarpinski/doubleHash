#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define SEED    0x12345678
#define SEED2   0x87654321
#define BUCKETS 5570

typedef struct {
    char codigoIBGE[50];
    char nome[50];
    char codigoUf[50];
    char capital[50];
    char latitude[50];
    char longitude[50];
    char siafi[50];
    char ddd[50];
    char fusoHorario[50];
}tMunicipio;

char* get_key(void* reg){
    return (*((tMunicipio*)reg)).codigoIBGE;
}

void* constroiCidade(char* codigoIBGE, char* nome, char* codigoUf, char* capital, char* latitude, char* longitude, char* siafi, char* ddd, char* fusoHorario){
    tMunicipio * cidade = malloc(sizeof(tMunicipio));
    strcpy(cidade->codigoIBGE, codigoIBGE);
    strcpy(cidade->nome, nome);
    strcpy(cidade->codigoUf, codigoUf);
    strcpy(cidade->capital, capital);
    strcpy(cidade->latitude, latitude);
    strcpy(cidade->longitude, longitude);
    strcpy(cidade->siafi, siafi);
    strcpy(cidade->ddd, ddd);
    strcpy(cidade->fusoHorario, fusoHorario);
    return cidade;
}

typedef struct {
     uintptr_t* table;
     int size;
     int max;
     uintptr_t deleted;
     char * (*get_key)(void *);
}thash;

uint32_t hashf(const char* str, uint32_t h){
    /* One-byte-at-a-time Murmur hash 
    Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32_t hashDouble(const char* str){
    uint32_t h = SEED2;
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}


int hash_insere(thash * h, void* bucket){
    uint32_t hash = hashf(h->get_key(bucket), SEED);
    
    char key[50]; 
    strcpy(key, h->get_key(bucket));
    
    int pos = hash % (h->max);
    if (h->max == (h->size+1)){
        free(bucket);
        return EXIT_FAILURE;
    }else{
        int i = 1;
        while(h->table[pos] != 0){
            if (h->table[pos] == h->deleted)
                break;
            pos = (hashf(key, SEED) + i * hashDouble(key)) % h->max;
            i++;
        }
        h->table[pos] = (uintptr_t) bucket;
        h->size +=1;
    }
    return EXIT_SUCCESS;
}



int hash_constroi(thash* h, int nbuckets, char* (*get_key)(void*) ){
    h->table = calloc(sizeof(uintptr_t), nbuckets +1);
    if (h->table == NULL){
        return EXIT_FAILURE;
    }
    h->max = nbuckets +1;
    h->size = 0;
    h->deleted = (uintptr_t) & (h->size);
    h->get_key = get_key;
    return EXIT_SUCCESS;
}


void* hash_busca(thash h, const char * key){
    int pos = hashf(key,SEED) % (h.max);
    int i = 1;
    while(h.table[pos] != 0){
        if (strcmp (h.get_key((void*)h.table[pos]),key) == 0){
            return (void *)h.table[pos];
        } else {
            pos = (hashf(key, SEED) + i * hashDouble(key)) % h.max;
        }
        i++;
    }
    return NULL;
}

int hash_remove(thash * h, const char * key){
    int pos = hashf(key,SEED) % (h->max);
    int i = 1;
    while(h->table[pos]!=0){
        if (strcmp (h->get_key((void*)h->table[pos]),key) ==0){
            free((void *) h->table[pos]);
            h->table[pos] = h->deleted;
            h->size -=1;
            return EXIT_SUCCESS;
        }else{
            pos = (hashf(key, SEED) + i * hashDouble(key)) % h->max;
        }
        i++;
    }
    return EXIT_FAILURE;
}

void hash_apaga(thash *h){
    int pos;
    for(pos =0;pos< h->max;pos++){
        if (h->table[pos] != 0){
            if (h->table[pos]!=h->deleted){
                free((void *)h->table[pos]);
            }
        }
    }
    free(h->table);
}

int lerCidades(thash* hash){
    FILE* municipios = fopen("municipios.txt", "r");

    if(municipios == NULL){
        return EXIT_FAILURE;
    }

    char codigoIBGE[50];
    char nome[50];
    char codigoUf[50];
    char capital[50];
    char latitude[50];
    char longitude[50];
    char siafi[50];
    char ddd[50];
    char fusoHorario[50];


    for(int i = 0; i<5570; i++){
        fscanf(municipios, " %s %s %s %s %s %s %s %s %s\n", codigoIBGE, nome, codigoUf, capital, latitude, longitude, siafi, ddd, fusoHorario);
        hash_insere(hash, constroiCidade(codigoIBGE, nome, codigoUf, capital, latitude, longitude, siafi, ddd, fusoHorario));
    }

    return EXIT_SUCCESS;
}


void test_hash(){
    thash h;
    
    hash_constroi(&h, BUCKETS, get_key);
    assert(lerCidades(&h)==EXIT_SUCCESS);
    hash_apaga(&h);
}


void test_search(){
    thash h;

    hash_constroi(&h, BUCKETS, get_key);
    lerCidades(&h);
    
    tMunicipio* city = (tMunicipio*)hash_busca(h, "4219507");
    assert(city->nome[0] == 'X');

    city = (tMunicipio*)hash_busca(h, "3557105");
    assert(city->nome[0] == 'V');

    city = hash_busca(h, "1508357");
    assert(city->nome[0]=='V');

    hash_apaga(&h);
}

void test_remove(){
    thash h;

    hash_constroi(&h, BUCKETS, get_key);
    lerCidades(&h);

    assert(h.size == 5570);

    assert(hash_remove(&h,"3557105")==EXIT_SUCCESS);

    tMunicipio* cidade = hash_busca(h, "3557105");
    assert(cidade == NULL);

    assert(h.size == 5569);

    assert(hash_remove(&h, "3557105")==EXIT_FAILURE);

    cidade = hash_busca(h, "4219507");
    assert(cidade->nome[0]=='X');


    hash_apaga(&h);

}

void op1(thash* t){
    char key[50];
    tMunicipio* m;
    printf("Insira o código IBGE: ");
    scanf(" %s", key);
    printf("\n");

    m = (tMunicipio*)hash_busca(*t, key);
    if(m == NULL){
        printf("Cidade não presente no banco de dados.\n");
    } else {
        printf("NOME: %s \nCÓDIGO UF: %s \nCAPITAL: %s \nLATITUDE: %s \nLONGITUDE: %s \nSIAFI: %s \nDDD: %s \nFUSOHORÁRIO: %s \n", m->nome, m->codigoUf, m->capital, m->latitude, m->longitude, m->siafi, m->ddd, m->fusoHorario);
    } 
    
    printf("\n\n\n");
}

void op2(thash* t){
    char key[50];
    tMunicipio* m;
    printf("Insira o código IBGE: ");
    scanf(" %s", key);
    printf("\n");

    int exit = hash_remove(t, key);
    if(exit == 0){
        printf("Sucesso!\n");
    } else {
        printf("Falha na operação.\n");
    }

    printf("\n\n\n");
}

void app(thash* hash){
    int op;
    printf("BUSCADOR DE CIDADES\n");
    do{
        printf("-------------------------------------------\n");
        printf("Escolha uma operação a ser realizada: \n1 - Buscar cidade\n2 - Excluir cidade\n3 - Finalizar\n");
        printf("-------------------------------------------\n");
        scanf("%d", &op);
        printf("\n");
        if(op == 1){
            op1(hash);
        }
        else if(op == 2){
            op2(hash);
        }
        else if(op != 3){
            printf("Operação inválida.\n");
        }
    }while(op != 3);

    printf("FINALIZADO\n");
}

int main(int argc, char* argv[]){
    test_hash();
    test_search();
    test_remove();

    thash h;

    hash_constroi(&h, BUCKETS, get_key);
    lerCidades(&h);
    
    tMunicipio* city = (tMunicipio*)hash_busca(h, "4219507");
    assert(city->nome[0] == 'X');

    city = (tMunicipio*)hash_busca(h, "3557105");
    assert(city->nome[0] == 'V');

    city = hash_busca(h, "1508357");
    assert(city->nome[0]=='V');

    thash novaHash;

    hash_constroi(&novaHash, BUCKETS, get_key);
    lerCidades(&novaHash);

    app(&novaHash);

    return 0;
}

