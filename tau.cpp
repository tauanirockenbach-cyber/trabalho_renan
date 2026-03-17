#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int potPin = A0;

bool sistemaLigado = false;
bool estadoAnterior = false;

int nivelAgua = 50; // valor inicial
int nivelAnterior = 50; // último nível antes de desligar
const int decrescimento = 1;   // diminuição por loop
const int delayLoop = 100;     // intervalo do loop em ms
const int capacidadeTotal = 10000; // capacidade total em litros

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistema OFF");
}

void loop() {
  // Lê potenciômetro (0 a 1023)
  int leituraPot = analogRead(potPin);

  // Velocidade proporcional ao potenciômetro (quanto menor, mais devagar)
  int velocidade = map(leituraPot, 0, 1023, 1, 10);

  // Subida automática se nível ≤ 10%
  if (nivelAgua <= 10) {
    sistemaLigado = true;
    nivelAgua += 1; // subida lenta automática
    if (nivelAgua > 90) nivelAgua = 90;
  } 
  else if (sistemaLigado) {
    // Subida proporcional ao potenciômetro se nível > 10%
    if (velocidade > 0 && nivelAgua < 90) {
      nivelAgua += velocidade / 4; // passo proporcional
      if (nivelAgua > 90) nivelAgua = 90;
    }
    if (nivelAgua >= 90) {
      nivelAgua = 90;
      sistemaLigado = false;
      nivelAnterior = nivelAgua;
    }
  } 
  else {
    // Diminuição gradual se desligado e acima de 10%, mas só se pot > 0
    if (nivelAgua > 10 && leituraPot > 0) {
      nivelAgua -= decrescimento;
      if (nivelAgua < 10) nivelAgua = 10;
    }
    nivelAnterior = nivelAgua;
  }

  // Calcula litros com base na porcentagem
  long litros = (long)nivelAgua * capacidadeTotal / 100;

  // Atualiza display somente se houve mudança no estado
  if (sistemaLigado != estadoAnterior) {
    lcd.clear();
    estadoAnterior = sistemaLigado;
  }

  // Exibe status e nível no LCD
  lcd.setCursor(0, 0);
  lcd.print(sistemaLigado ? "Sistema ON " : "Sistema OFF");

  lcd.setCursor(0, 1);
  lcd.print(nivelAgua);
  lcd.print("% ");
  lcd.print(litros);
  lcd.print("L");

  // Delay proporcional à velocidade
  delay(delayLoop * (11 - velocidade));
}