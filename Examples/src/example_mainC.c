#include <inttypes.h>
uint32_t msg[10] = {10,123456,67,69,83,65,82,32,12,0};
int matriz=0;
extern int f_asm(void);

struct time_str {     
    
    unsigned long long h;//64 - 8 bytes
    unsigned char a;     //16 - 2 bytes
    long long g;         //64 - 8 bytes 
    signed char b;       //16 - 2 bytes
    unsigned short c;    //16 - 2 bytes
    signed short d;     //16 - 2 bytes
    int e;              //32 - 4 bytes
    unsigned f;         //32 - 4 bytes
};
struct time_str t1;

struct time_good {
    unsigned long long h;
    long long g;
    int e;
    unsigned f;
    unsigned short c;
    signed short d;
    signed char b;
    unsigned char a;
};
struct time_good t2;

#pragma pack(1)
struct time_str_packed {
    unsigned long long h;
    unsigned char a;
    long long g;
    signed char b;
    unsigned short c;
    signed short d;
    int e;
    unsigned f;
};
struct time_str_packed t3;

#pragma pack(1)
typedef struct SE_Protocolo{ 
  uint8_t tamanho;
  uint32_t id;
  uint16_t codigo;//[20];
  uint32_t temperatura;
  uint32_t acelX;
}TProtocolo;

const int endereco=1234;
volatile int med;

int main()
{
  int tamanhoT1=sizeof(t1);
  int tamanhoT2=sizeof(t2);
  int tamanhoT3=sizeof(t3);
  t1.a=12;
  t2.a=12;
  t3.a=12;
  int z=tamanhoT1+tamanhoT2+tamanhoT3;
  
  uint32_t *p=msg;
  
  TProtocolo *tp;  
  tp=(TProtocolo*)p;
  
  z=tp->acelX;
  
  med = f_asm( );
  
  
  return 0;
}

