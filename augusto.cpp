#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo bombaServo;

// --- PINOS ---
const int botoes[] = {2, 4, 5, 6, 7}; // Níveis 1-5
const int pinManual = 13; // Botão manual

// --- VARIÁVEIS ---
float volumeAtual = 0.0;
float volumeAlvo = 0.0;
const float capacidadeTanque = 10000.0;

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

// Último nível válido
int ultimoNivelValido = 0;

// Direção de movimento
bool subindo = true; // true = subir, false = descer

// Modo manual
bool manualAtivo = false;
bool ultimoEstadoManual = HIGH;

// Mensagem de erro
bool erroAtivo = false;
unsigned long tempoErro = 0;

// --- Funções auxiliares ---
int descobrirNivelAtual(float porcentagem) {
  for (int i = 0; i < 5; i++) {
    if (porcentagem <= niveis[i] * 100 + 1) {
      return i;
    }
  }
  return 4;
}

float arredondar10(float valor) {
  return (int)(valor / 10.0) * 10.0;
}

void setup() {
  lcd.init();
  lcd.backlight();
  bombaServo.attach(11);

  pinMode(pinManual, INPUT_PULLUP);

  for (int i = 0; i < 5; i++) {
    pinMode(botoes[i], INPUT_PULLUP);
  }

  // --- Mensagem de erro ao ligar/reiniciar ---
  lcd.setCursor(0, 0);
  lcd.print("Inicializando....      ");
  lcd.setCursor(0, 1);
  lcd.print("Aguarde!  ");
  delay(3000);
  lcd.clear();

  // --- Carrega volume e direção do EEPROM ---
  EEPROM.get(0, volumeAtual);
  EEPROM.get(4, subindo);

  if (isnan(volumeAtual) || volumeAtual < volumeMin) volumeAtual = volumeMin;
  if (volumeAtual > volumeMax) volumeAtual = volumeMax;

  nivelAtualIndex = descobrirNivelAtual((volumeAtual / capacidadeTanque) * 100);
  ultimoNivelValido = nivelAtualIndex;

  if (subindo) {
    volumeAlvo = capacidadeTanque * niveis[ultimoNivelValido];
  } else {
    float baseNivel = (ultimoNivelValido == 0) ? volumeMin : capacidadeTanque * niveis[ultimoNivelValido - 1];
    volumeAlvo = max(baseNivel, volumeMin);
  }
}

void loop() {

  if (millis() - tempoPisca > 300) {
    tempoPisca = millis();
    estadoPisca = !estadoPisca;
  }

  bool estadoBotaoManual = digitalRead(pinManual);
  if (estadoBotaoManual == LOW && ultimoEstadoManual == HIGH) {
    manualAtivo = !manualAtivo;
  }
  ultimoEstadoManual = estadoBotaoManual;

  float porcentagemAtual = (volumeAtual / capacidadeTanque) * 100;
  nivelAtualIndex = descobrirNivelAtual(porcentagemAtual);

  for (int i = 0; i < 5; i++) {
    if (digitalRead(botoes[i]) == LOW) {
      float alvoTemp = capacidadeTanque * niveis[i];

      if (manualAtivo) {
        volumeAlvo = alvoTemp;
        ultimoNivelValido = i;
        subindo = (volumeAlvo > volumeAtual);
      } else {
        if (i == nivelAtualIndex + 1 || i == nivelAtualIndex - 1) {
          volumeAlvo = alvoTemp;
          ultimoNivelValido = i;
          subindo = (volumeAlvo > volumeAtual);
        } else if (i != nivelAtualIndex) {
          volumeAlvo = capacidadeTanque * niveis[ultimoNivelValido];
          erroAtivo = true;
          tempoErro = millis();
        }
      }

      if (volumeAlvo > volumeMax) volumeAlvo = volumeMax;
      if (volumeAlvo < volumeMin) volumeAlvo = volumeMin;
    }
  }

  if (millis() - tempoAnterior > 50) {
    tempoAnterior = millis();

    if (volumeAtual < volumeAlvo) {
      volumeAtual += passo;
      bombaServo.write(180);
      subindo = true;
    } else if (volumeAtual > volumeAlvo) {
      volumeAtual -= passo;
      bombaServo.write(45);
      subindo = false;
    } else {
      bombaServo.write(0);
    }

    if (volumeAtual > volumeMax) volumeAtual = volumeMax;
    if (volumeAtual < volumeMin) volumeAtual = volumeMin;

    volumeAtual = arredondar10(volumeAtual);

    EEPROM.put(0, volumeAtual);
    EEPROM.put(4, subindo);
  }

  // --- LCD ---
  lcd.setCursor(0, 0);
  if (erroAtivo) {
    lcd.print("     !ERRO!           ");
    if (millis() - tempoErro > 2000) erroAtivo = false;
  } else {
    lcd.print("Nivel: ");
    lcd.print(nivelAtualIndex + 1);
    if (manualAtivo) lcd.print(" Manual");
    else lcd.print("             ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Vol:");
  lcd.print((int)volumeAtual);
  lcd.print(" A:");
  lcd.print((int)((volumeAlvo / capacidadeTanque) * 100));
  lcd.print("L   ");
}