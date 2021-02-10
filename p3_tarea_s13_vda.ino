// 0 false 1 true
int isPulsed = 0;
// Programación básica tarjetaPI

// Declaración de variables

// Barrido del display-7seg y scanner del teclado (PORTL)
// Display (4 bits inferiores)
volatile int D4=  B11111110;  //PL0 D4-C1
volatile int D3=  B11111101;  //PL1 D3-C2
volatile int D2=  B11111011;  //PL2 D2-C3
volatile int D1=  B11110111;  //PL3 D1-nn

// Pulsadores pup, pdown, pleft, pright, penter y speaker(PORTC)
int pright = 30;  //PC7
int pdown = 31;   //PC6
int pleft = 32;   //PC5
int penter = 33;  //PC4
int pup = 34;     //PC3
//PC2
//PC1
int speaker =37;  //PC0


/*
// Teclado (4 bits superiores)
volatile int fila_R4=   B11100000;  //PL4 fila_R4
volatile int fila_R3=   B11010000;  //PL5 fila_R3
volatile int fila_R2=   B10110000;  //PL6 fila_R2
volatile int fila_R1=   B01110000;  //PL7 fila_R1

// Display de 7 segmentos (PORTA)
volatile int dp=29;  //PA7
volatile int g= 28;  //PA6
volatile int f= 27;  //PA5
volatile int e= 26;  //PA4
volatile int d= 25;  //PA3
volatile int c= 24;  //PA2
volatile int b= 23;  //PA1
volatile int a= 22;  //PA0

*/



// otras variables
volatile int  tecla_anterior=-1;
volatile int  tecla_actual=-1;
volatile boolean modoTurnomatic=1;
volatile int estado=0;
volatile int contador=0;
volatile int incremento=1;
volatile int unidades;
volatile int decenas;
volatile int val;

unsigned int fclk = 200; // 100 HZ, frecuencia del reloj de interrupción
unsigned int sound_base = 100;
unsigned int sound;
unsigned long duration = 200;


// código de 7-segmentos
byte tabla7seg[] = {63,06,91,79,102,109,125,39,127,103, 0,1, 2, 4, 8, 16, 32};
byte vsound[]={262,277,294,311,330,349,370,392,415,440};



void setup(){
  Serial.begin(9600);

  //PA7-PA0 (dpgfedcba) salidas --> display 7-seg
  DDRA = B11111111;

  // PC7-PC0 --> Pulsadores entrada PC7-PC3 (entrada)) PC4-PC5 (free, entrada), PC0 speaker (salida)
  DDRC = B00000001;
  // activación pull-up en las líneas de entrada
  PORTC = B11111110;

  // PL7-PL0 --> barrido display PL4-PL0 (salida) y scanner teclado (PL7-PL4)
  DDRL = B00001111;
  // activación líneas de pull-up en las entradas de PORTL
  PORTL = B11111111;


  // Pin para conectar el altavoz
  pinMode(speaker,OUTPUT);

  // Inicialización de variables
  estado = 0;
  unidades = 0;
  decenas = 0;
  sound = sound_base;


  // Habilitación del TIMER1 para interrumpir cada 10ms (100Hz)
  // Funcionamiento normal... todo a cero
  // Disable interrupts
  cli();
  // modo normal de funcionamiento
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0; // cuenta inicial a cero
  // mode CTC
  TCCR1B |= (1<<WGM12);
  // prescaler  N = 1024
  TCCR1B |= (1<<CS12)|(1<<CS10);
  // fintr = fclk/(N*(OCR1A+1)) --> OCR1A = [fclk/(N*fintr)] - 1
  // para fintr = 100Hz --> OCR1A = [16*10^6/(1024*100)] -1 = 155,25 --> 155

  OCR1A = 155; // para 200 Hz  programar OCR1A = 77 (se ve mejor a 200 Hz!)

  // enable timer1 compare interrupt
  TIMSK1 |=(1<<OCIE1A);
  // habilitamos interrupciones
  sei();
}


void loop(){

  // Programa principal de lectura de los Pulsadores

  if (digitalRead(pup) == 0){

    // Incrementar
    incrementar();
    do
    {
      val = digitalRead(pup);
      if (digitalRead(pdown)==0) reset();
    }while(val == 0);
  }

  // decrementar
  if (digitalRead(pdown) == 0){
    // Decrementar
    decrementar();
    do
    {
      val = digitalRead(pdown);
      if (digitalRead(pup)==0) reset();
    }while(val==0);
  }

}

void incrementar()
{
  contador=decenas*10+unidades;
  contador=contador+incremento;
  if (contador >99)contador = contador-100;
  decenas = int(contador/10);
  unidades = contador % 10;
  if(modoTurnomatic == 0) {
    sound = sound_base * 2;
    beep();
  } else {
    beep();
  }

}

void decrementar()
{
  contador=decenas*10+unidades;
  contador=contador-incremento;
  if (contador < 0)contador = contador+100;
  decenas = int(contador/10);
  unidades = contador % 10;
  if(modoTurnomatic == 0) {
    sound = sound_base * 2;
    beep();
  } else {
    beep();
  }
}

void reset()
{
  unidades=0;
  decenas=0;
  beep();
}

// función de refresco de los displays
//void DisplayRefresh()
ISR(TIMER1_COMPA_vect)
{
  // display-off
  PORTL=PORTL|B00001111;
  switch(estado){
    case 0:

    // UNIDADES
    // Actualizamos portA con unidades
    PORTA = tabla7seg[unidades];
    // Activamos unidades en PORTL (D1D2D3D4)
    PORTL = D4;
    teclado(estado);
    estado=1;

    break;

    case 1:
    // DECENAS
    // Actualizamos portA con decenas
    PORTA = tabla7seg[decenas];
    // Activamos decenas en PORTL (D1D2D3D4)
    PORTL=D3;
    teclado(estado);
    estado=2;
    break;

    case 2:
    // solo barrido de la tercera columna del teclado (D1D2D3D4)
    PORTA = tabla7seg[10];
    PORTL=D2;
    teclado(estado);
    estado=0;
    break;

    case 3:
    // No hace falta. Necesario solo si el teclado tuviese una cuarta columna o si queremos usar el D1
    PORTL=D1;
    teclado(estado);
    estado=0;
    break;

  }
}


void teclado(int estado){
  int teclau, tecla;

  // Leemos teclado
  teclau = (PINL & 0xF0)>>4; // leemos filas del teclado
  if (teclau != 15) // hay pulsación y esperamos a que se suelte tecla
  {
    while( ((PINL & 0xF0)>>4) !=15);
  }


  switch(estado){
    case 0:
    // Barrido de la primera columna del teclado

    switch (teclau) {
      case 7:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 1;
        modoTurnomatic = 0;
      }
      tecla = 1;
      break;
      case 11:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 4;
        modoTurnomatic = 0;
      }
      tecla = 4;
      break;
      case 13:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 7;
        modoTurnomatic = 0;
      }
      tecla = 7;
      break;
      case 14:
      // "*" 0x2A=42
      tecla=42;
      break;
    }
    break;
    case 1:
    // barrido de la segunda columna del teclado

    switch (teclau) {
      case 7:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 2;
        modoTurnomatic = 0;
      }
      tecla = 2;
      break;
      case 11:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 5;
        modoTurnomatic = 0;
      }
      tecla = 5;
      break;
      case 13:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 8;
        modoTurnomatic = 0;
      }
      tecla = 8;
      break;
      case 14:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 0;
        modoTurnomatic = 0;
      }
      tecla = 0;
      break;
    }
    break;

    case 2:
    // Barrido de la tercera columna
    switch (teclau) {
      case 7:
      if(isPulsed == 1){
        isPulsed = 0;
        incremento = 3;
        modoTurnomatic = 0;
      }
      tecla = 3;
      break;
      case 11:
      if(isPulsed == 1){
        incremento = 6;
        isPulsed = 0;
        modoTurnomatic = 0;
      }
      tecla = 6;
      break;
      case 13:
      if(isPulsed == 1){
        incremento = 9;
        isPulsed = 0;
        modoTurnomatic = 0;
      }
      tecla = 9;
      break;
      case 14:
      // #
      if(isPulsed == 1){
        isPulsed = 0;
        modoTurnomatic == 1;
        sound = sound_base;
        beep();
        incremento = 1;
      } else {
        isPulsed = 1;
      }
      tecla=0x23;
      break;
    }
    break;
  }

  // si teclau != 15 es que se ha pulsado una tecla
  if(teclau !=15){
    tecla_anterior=tecla_actual;
    tecla_actual=tecla;
    //Serial.println(tecla);
  }


  if (modoTurnomatic == 1){
    // modo turnomatic

    if(tecla_anterior == '*' && (tecla_actual >=0 && tecla_actual<=9))
    { // cambiar frecuencia sonido
      sound = sound_base + tecla_actual * 400;
      if(tecla_actual == 0) sound=0;
      tecla_anterior=-1;
      tecla_actual=-1;
    }
  }

}


void beep(){

  //noTone(ptone);
  tone(speaker,sound,duration);

}
