#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo bombaServo;

// --- PINOS ---
const int botoes[] = {2, 3, 4, 5, 6}; // Níveis 1-5
const int leds[] = {7, 8, 9, 10, 12};
const int pinManual = 13; // Botão para modo manual

// --- VARIÁVEIS ---
float volumeAtual = 0.0;
float volumeAlvo = 0.0;
const float capacidadeTanque = 10000.0; // 10 L se 1 unidade = 1 ml

// Limites
const float volumeMin = capacidadeTanque * 0.10;
const float volumeMax = capacidadeTanque * 0.90;

// Passo de 10 ml
const float passo = 10.0;

unsigned long tempoAnterior = 0;
unsigned long tempoPisca = 0;
bool estadoPisca = false;

// Níveis
float niveis[] = {0.10, 0.30, 0.50, 0.70, 0.90};
int nivelAtualIndex = 0;

// Modo manual
bool manualAtivo = false;
bool ultimoEstadoManual = HIGH;

// Mensagem de erro
bool erroAtivo = false;
unsigned long tempoErro = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  bombaServo.attach(11);

  pinMode(pinManual, INPUT_PULLUP);

  for (int i = 0; i < 5; i++) {
    pinMode(leds[i], OUTPUT);
    pinMode(botoes[i], INPUT_PULLUP);
  }

  EEPROM.get(0, volumeAtual);
  if (isnan(volumeAtual) || volumeAtual < volumeMin) volumeAtual = volumeMin;
  if (volumeAtual > volumeMax) volumeAtual = volumeMax;

  volumeAlvo = volumeAtual;
}

// --- DESCOBRE EM QUAL NÍVEL ESTÁ ---
int descobrirNivelAtual(float porcentagem) {
  for (int i = 0; i < 5; i++) {
    if (porcentagem <= niveis[i] * 100 + 1) {
      return i;
    }
  }
  return 4;
}

void loop() {

  // --- PISCA ---
  if (millis() - tempoPisca > 300) {
    tempoPisca = millis();
    estadoPisca = !estadoPisca;
  }

  // --- BOTÃO MANUAL (debounce simples) ---
  bool estadoBotaoManual = digitalRead(pinManual);
  if (estadoBotaoManual == LOW && ultimoEstadoManual == HIGH) {
    manualAtivo = !manualAtivo;
  }
  ultimoEstadoManual = estadoBotaoManual;

  float porcentagemAtual = (volumeAtual / capacidadeTanque) * 100;
  nivelAtualIndex = descobrirNivelAtual(porcentagemAtual);

  // --- BOTÕES DE NÍVEL ---
  for (int i = 0; i < 5; i++) {
    if (digitalRead(botoes[i]) == LOW) {
      float alvoTemp = capacidadeTanque * niveis[i];

      if (manualAtivo) {
        // Modo manual: qualquer nível permitido
        volumeAlvo = alvoTemp;
      } else {
        // Modo automático: só permite subir/descendo 1 nível
        if (i == nivelAtualIndex + 1 || i == nivelAtualIndex - 1) {
          volumeAlvo = alvoTemp;
        } else if (i != nivelAtualIndex) {
          // Tentou pular nível → mostrar erro
          erroAtivo = true;
          tempoErro = millis();
        }
      }

      // Limitar volumeAlvo
      if (volumeAlvo > volumeMax) volumeAlvo = volumeMax;
      if (volumeAlvo < volumeMin) volumeAlvo = volumeMin;
    }
  }

  // --- MOVIMENTO ---
  if (millis() - tempoAnterior > 50) {
    tempoAnterior = millis();

    if (volumeAtual < volumeAlvo) {
      volumeAtual += passo;
      bombaServo.write(180);
    } 
    else if (volumeAtual > volumeAlvo) {
      volumeAtual -= passo;
      bombaServo.write(45);
    } 
    else {
      bombaServo.write(0);
    }

    // TRAVAS
    if (volumeAtual > volumeMax) volumeAtual = volumeMax;
    if (volumeAtual < volumeMin) volumeAtual = volumeMin;

    EEPROM.put(0, volumeAtual);
  }

  // --- LEDS INVERTIDOS ---
  for (int i = 0; i < 5; i++) {
    int ledIndex = 4 - i;
    float limite = capacidadeTanque * niveis[i];

    if (i == 0 && porcentagemAtual <= 10.0) {
      digitalWrite(leds[ledIndex], estadoPisca);
    }
    else if (i == 4 && porcentagemAtual >= 90.0) {
      digitalWrite(leds[ledIndex], estadoPisca);
    }
    else {
      digitalWrite(leds[ledIndex], (volumeAtual >= limite) ? HIGH : LOW);
    }
  }

  // --- LCD ---
  lcd.setCursor(0, 0);
  if (erroAtivo) {
    lcd.print("     !ERRO!        ");
    if (millis() - tempoErro > 2000) {
      erroAtivo = false;
    }
  } else {
    lcd.print("Nivel: ");
    lcd.print(nivelAtualIndex + 1);
    if (manualAtivo) lcd.print(" Manual");
    else lcd.print("       ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Vol:");
  lcd.print((int)volumeAtual);
  lcd.print(" A:");
  lcd.print((int)((volumeAlvo / capacidadeTanque) * 100));
  lcd.print("L   ");
}