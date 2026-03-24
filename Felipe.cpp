#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo bombaServo;

// --- PINOS ---
const int pinPotVazao = A0;      
const int botoes[] = {2, 3, 4, 5, 6};      
const int leds[] = {7, 8, 9, 10, 12};  

// --- VARIÁVEIS ---
float volumeAtual = 0.0;
float volumeAlvo = 0.0;
const float capacidadeTanque = 10000.0; 
unsigned long tempoAnterior = 0;
unsigned long tempoPisca = 0;
bool estadoPisca = false;

void setup() {
  lcd.init();
  lcd.backlight();
  bombaServo.attach(11);
  
  for (int i = 0; i < 5; i++) {
    pinMode(leds[i], OUTPUT);
    pinMode(botoes[i], INPUT_PULLUP);
  }

  EEPROM.get(0, volumeAtual);
  if(isnan(volumeAtual) || volumeAtual < 0) volumeAtual = 0;
  volumeAlvo = volumeAtual; 
}

void loop() {
  // 1. GERENCIADOR DO PISCA-PISCA (Inverte a cada 300ms)
  if (millis() - tempoPisca > 300) {
    tempoPisca = millis();
    estadoPisca = !estadoPisca;
  }

  // 2. LER VELOCIDADE (POTENCIÔMETRO)
  int leituraPot = analogRead(pinPotVazao);
  int velocidade = map(leituraPot, 0, 1023, 50, 500); 

  // 3. SELECIONAR NÍVEL ALVO (BOTÕES)
  if (digitalRead(botoes[2]) == LOW) volumeAlvo = capacidadeTanque * 0.10;
  if (digitalRead(botoes[3]) == LOW) volumeAlvo = capacidadeTanque * 0.30;
  if (digitalRead(botoes[4]) == LOW) volumeAlvo = capacidadeTanque * 0.50;
  if (digitalRead(botoes[5]) == LOW) volumeAlvo = capacidadeTanque * 0.70;
  if (digitalRead(botoes[6]) == LOW) volumeAlvo = capacidadeTanque * 0.90;

  // 4. LÓGICA DE MOVIMENTAÇÃO
  if (millis() - tempoAnterior > 200) { 
    tempoAnterior = millis();
    if (abs(volumeAtual - volumeAlvo) > 90) {
      if (volumeAtual < volumeAlvo) {
        volumeAtual += velocidade;
        bombaServo.write(180); 
      } else {
        volumeAtual -= velocidade;
        bombaServo.write(45);  
      }
    } else {
      volumeAtual = volumeAlvo;
      bombaServo.write(0);
    }
    
    if (volumeAtual < 0) volumeAtual = 0;
    if (volumeAtual > capacidadeTanque) volumeAtual = capacidadeTanque;
    EEPROM.put(0, volumeAtual);
  }

  // 5. ATUALIZAÇÃO DOS LEDS COM EFEITO PISCANTE
  float porcet = (volumeAtual / capacidadeTanque) * 100;

  for (int i = 0; i < 5; i++) {
    float limite = (capacidadeTanque / 5) * (i + 1);
    
    // Lógica para o LED de 10% (i=0) e 90% (i=4)
    if (i == 0 && porcet <= 10.0) {
      digitalWrite(leds[i], estadoPisca); // Pisca se estiver no mínimo
    } 
    else if (i == 4 && porcet >= 90.0) {
      digitalWrite(leds[i], estadoPisca); // Pisca se estiver no máximo
    } 
    else {
      // Funcionamento normal para os outros níveis
      digitalWrite(leds[i], (volumeAtual >= limite) ? HIGH : LOW);
    }
  }

  // 6. EXIBIÇÃO NO LCD
  lcd.setCursor(0, 0);
  if (porcet >= 90.0) {
    lcd.print("NIVEL MAXIMO!   ");
  } else if (porcet <= 10.0) {
    lcd.print("NIVEL MINIMO!   ");
  } else {
    lcd.print("Nivel: "); lcd.print(porcet, 1); lcd.print("%    ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Vol: "); lcd.print((int)volumeAtual); 
  lcd.print("L  Alvo:"); lcd.print((int)((volumeAlvo/capacidadeTanque)*100)); lcd.print("%");
}