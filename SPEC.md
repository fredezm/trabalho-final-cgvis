# Especificação da Implementação

## Integrantes da dupla

- **Aluno 1 - Nome**: <mark>`Frederico Zucchetti Mattiello`</mark>
- **Aluno 1 - Cartão UFRGS**: <mark>`581041`</mark>

- **Aluno 2 - Nome**: <mark>`André Gatti Wolff`</mark>
- **Aluno 2 - Cartão UFRGS**: <mark>`578039`</mark>
## Detalhes do que será implementado

- **Título do trabalho**: <mark>`FUI RAPTADO PELO ABEL BRAGA E AGORA SOU OBRIGADO A BATER FALTAS PELO INTERNACIONAL`</mark>
- **Parágrafo curto descrevendo o que será implementado**: <mark>`Vamos implementar um jogo que o objetivo é cobrar faltas e acertar o gol. O objetivo é acertar as cobranças. Ao posicionar a bola, o jogador enfrentará alguns desafios: obstáculos e um temporizador. O jogador irá controlar o jogo usando seu mouse e teclado; o mouse controlará o posicionamento da bola e a câmera, enquanto que o teclado controlará a trajetória e força do chute.`</mark>

## Especificação visual

### Vídeos curtos (10 segundos cada) - Links

- **Vídeo 1: Câmera visão da bola**: <mark>`https://www.youtube.com/shorts/9RKJ2QLKOLY`</mark>
- **Vídeo 2: Câmera visão de cima**: <mark>`https://youtu.be/tqzlbn_Vjgc?si=xT-NXLEPco7OsZzl`</mark>
- **Vídeo 2: Bola batendo na barreira**: <mark>`https://www.youtube.com/shorts/FGAQR7nO1ag`</mark>

### Imagens

<mark>`https://drive.google.com/drive/u/2/folders/19kucK3grjm4vhRiNBlIvvZ16Wm9lYe5m`</mark>

## Especificação textual

Para cada um dos requisitos abaixo (detalhados no [Enunciado do Trabalho final - Moodle](https://moodle.ufrgs.br/mod/assign/view.php?id=6018620)), escreva um parágrafo **curto** explicando como este requisito será atendido, apontando itens específicos do vídeo/imagens que você incluiu acima que atendem estes requisitos.

### Malhas poligonais complexas
<mark>`Haverá duas malhas poligonais complexas: a bola e as traves (cilíndros).`</mark>

### Transformações geométricas controladas pelo usuário
<mark>`O jogador decide de onde será batida a falta. Após decidir de onde baterá, o jogo posiciona a barreira para tentar impedir o jogador de acertar.`</mark>

### Diferentes tipos de câmeras
<mark>`Visão de cima para posicionar a cobrança e visão em 3ª pessoa da bola para direcionar a cobrança e bater a falta.`</mark>

### Instâncias de objetos
<mark>`Haverá multiplas instâncias de objetos na barreira. Cada jogador da barreira será um objeto.`</mark>

### Testes de intersecção
<mark>`A bola não pode atravessar a barreira, a trave e o plano.`</mark>

### Modelos de Iluminação em todos os objetos
<mark>`Utilizaremos o Modelo de Iluminação de Phong.`</mark>

### Mapeamento de texturas em todos os objetos
<mark>`Sim, na bola, trave e campo.`</mark>

### Movimentação com curva Bézier cúbica
<mark>`A bola se movimentará usando uma curva de Bézier cúbica.`</mark>

### Animações baseadas no tempo ($\Delta t$)
<mark>`A bola se movimentará e a barreira ocasionalmente pulará.`</mark>

## Limitações esperadas

<mark>` - Goleiro: vamos tentar implementar parcialmente o goleiro. Vamos posicionar um objeto na posição do goleiro, porém não temos certeza se iremos animá-lo. Implementar uma inteligência artificial básica que seria capaz de alcançar a bola seria complicado e acho que não está no escopo da disciplina. `</mark>

<mark>`- Zagueiro: não haverá zagueiros além da barreira. A animação do corpo é complexa e acreditamos que ficaria muito tosco fazer isso parcialmente.`</mark>

<mark>` - Jogador batendo a falta: não implementaremos o jogador que baterá a falta. Mesma explicação dos zagueiros.`</mark>
