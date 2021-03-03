// Question4
// author: stf

#define SIZE 6000 

// Volatile keyword stops the compiler optimising
// the arrays away
volatile float a[SIZE];
volatile float b[SIZE];
volatile float c;

void dotProduct() {
   for(int i=0; i<SIZE; i++) {
      c += a[i] * b[i];
   }	
}

void randomise(){
   for(int i=0; i<SIZE; i++) {
      a[i] = esp_random();
      b[i] = esp_random();
   }	
}

void setup() {
  randomise();
}

void loop() {
  dotProduct();
  delayMicroseconds(750);
}
