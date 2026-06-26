# Relatório Final \- Trabalho Final de Computação Gráfica

## Integrantes da dupla

Aluno 1 \- Nome: Frederico Zucchetti Mattiello

Aluno 1  \- Cartão UFRGS: 581041

Aluno 2 \- Nome: André Gatti Wolff

Aluno 2 \- Cartão UFRGS: 578039

## Aplicação Desenvolvida

A aplicação consiste em um jogo de cobrança de faltas de futebol, onde o jogador posiciona a bola em um lugar do campo, devendo superar a barreira e o goleiro para pontuar, marcando gols. Quando o jogador marcar 3 gols, a partida acaba com uma vitória; quando o jogador desperdiça 5 tentativas, é “game over”. Os objetos virtuais consistem em: bola, zagueiros, goleiro, goleira (traves, travessão e rede), plano e estádio. 

## Uso de ferramentas de IA

O uso de IA em nosso trabalho consistiu, principalmente, em discussões para tirar dúvidas e/ou resolver bugs e geração de códigos preliminares. Em todos os casos utilizamos com cuidado e aos poucos, editando os códigos manualmente. Utilizamos ´Gemini´ e ´Claude´ majoritariamente. Essas ferramentas agilizaram bastante o trabalho. 

## Imagens mostrando o funcionamento da aplicação;

<img width="676" height="544" alt="Image" src="https://github.com/user-attachments/assets/dcc4f7b1-fd2e-486e-8824-1ccbd38fda5b" />
<img width="682" height="542" alt="Image" src="https://github.com/user-attachments/assets/8b357c4a-d023-4a7c-9364-ecba5c7ff1ef" />
## Manual de utilização da aplicação

Câmera top down \- Teclas ´WASD´ para mover bola.
Câmera terceira pessoa: Teclas ´WASD´ posicionam o ponto final da curva de bézier. ´Scroll Up´ e ´Scroll Down´ alteram a curva da bola. Tecla ´Espaço´ chuta a bola. Tecla ´Q´ para passar para a próxima rodada (quando o prompt aparecer).

## Passos necessários para compilação e execução da aplicação;

Usamos o comando ´make run´ para compilar e executar o código. Não há dependências externas. 

##  Contribuição de cada membro da dupla para o trabalho;

André \- Fui responsável pelas seguintes contribuições:  Câmera top-down, posicionamento da bola, iluminação, tracejado da bola, FSM para controlar lógica do jogo, colisão com os defensores, obj do estádio e obj defensores.

Frederico \- Fui responsável pelas seguintes contribuições: obj goleira, obj bola, câmera em terceira pessoa da bola, movimentação da bola com bezier, colisão com a goleira, lógica do gol e turnos, força do chute, “física da bola”, hud.


