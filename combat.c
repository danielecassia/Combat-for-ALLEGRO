// includes
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// variáveis globais
const float FPS = 60;

const int SCREEN_W = 1250;
const int SCREEN_H = 951;

const float THETA = M_PI / 4;
const float RAIO_CAMPO_FORCA = 50;

const float VEL_TANQUE = 2.5;
const float PASSO_ANGULO = M_PI / 90;

const float RAIO_TIRO = 3;
const float VEL_TIRO = 3.5;

const int MAX_JOGADORES = 2;
const int VENCEDOR;
bool condicao = false;
int pontos = 0;
int id = 0;

ALLEGRO_BITMAP *cenario = NULL;
ALLEGRO_BITMAP *gameover = NULL;

const int MAX_TAM = 1000;

// int TOTAL_ATIRADO = 0;

// STRUCTS
typedef struct Ponto
{
	float x, y;
} Ponto;

typedef struct Tanque
{
	Ponto centro;
	Ponto A, B, C;
	ALLEGRO_COLOR cor;

	float vel;
	float angulo;
	float x_comp, y_comp;
	float vel_angular;
	int pontuacao;
	int up;
	int down;
	int left;
	int right;
	int fire;
	int id;
} Tanque;

typedef struct Tiro
{
	Ponto centro;
	Tanque *quemDeu;

	float vel_tiro;
	float angulo_tiro;
	int contador;
	ALLEGRO_COLOR cor;
} Tiro;

struct lista_tiro
{
	Tiro elem;
	struct lista_tiro *lig;
};

typedef struct lista_tiro Atirados;

typedef struct
{
	int nelem;
	Atirados *head;
} ListaTiro;

void Remove_Prim(ListaTiro *L)
{
	/* supõe que a Lista não está vazia */
	Atirados *p = L->head;
	L->head = p->lig;
	free(p);
	L->nelem--;
}

void Elimina_Depois(ListaTiro *L, Atirados *k)
{
	Atirados *j = k->lig;
	k->lig = j->lig;
	free(j);
	L->nelem--;
}

bool RemoveTiro(Tiro v, ListaTiro *L)
{
	Atirados *p = L->head;
	Atirados *pa = NULL;
	while (p != NULL)
	{
		if (p->elem.angulo_tiro < v.angulo_tiro)
		{
			pa = p;
			p = p->lig;
		}
		else
		{
			if (p->elem.angulo_tiro > v.angulo_tiro)
				/* encontrou elemento com chave maior*/
				return false;
			else
			{
				/*encontrou o elemento*/
				if (pa == NULL)
					/*remove no inicio*/
					Remove_Prim(L);
				else
				{
					/*remove elemento p*/
					Elimina_Depois(L, pa);
				}
				return true;
			}
		}
	}
	/*não encontrou o elemento na lista*/
	return false;
}

typedef struct Bloco
{
	Ponto top_left;
	Ponto bottom_right;
	ALLEGRO_COLOR cor;
} Bloco;

//INICIALIZAÇÕES
//inicializando o tanque
void initTanque(Tanque *t1, int x, ALLEGRO_COLOR cor)
{
	t1->centro.x = x; // 60; //(SCREEN_W / 2) - 420;
	t1->centro.y = SCREEN_H / 2;
	t1->cor = cor; // al_map_rgb(106,90,205);

	t1->A.x = 0;
	t1->A.y = -RAIO_CAMPO_FORCA;

	float alpha = M_PI / 2 - THETA;
	float h = RAIO_CAMPO_FORCA * sin(alpha);
	float w = RAIO_CAMPO_FORCA * sin(THETA);

	t1->B.x = -w;
	t1->B.y = h;

	t1->C.x = w;
	t1->C.y = h;

	t1->vel = 0;
	t1->angulo = M_PI / 2;
	t1->x_comp = cos(t1->angulo);
	t1->y_comp = sin(t1->angulo);

	t1->vel_angular = 0;

	t1->pontuacao = 0;
}

//inicializando o bloco
void initBloco(Bloco *b)
{

	b->top_left.x = (SCREEN_W / 2) - 60;
	b->top_left.y = (SCREEN_H / 2) - 100;
	b->bottom_right.x = (SCREEN_W / 2) + 60;
	b->bottom_right.y = (SCREEN_H / 2) + 100;
	b->cor = al_map_rgb(75, 0, 130);
}

//inicializando o tiro
void initTiro(Tiro *tr1, Tanque t1)
{

	printf("valor de A -------------- %f\n", t1.A.y);
	tr1->centro.x = t1.A.x;
	tr1->centro.y = t1.A.y;
	tr1->angulo_tiro = 0;
	tr1->contador = 0;
	tr1->vel_tiro = 0;
	tr1->quemDeu = NULL;
}

//FUNCÕES
// desenha o cenário
void desenhaCenario(int pontos)
{

	if (pontos < 5)
	{
		al_draw_bitmap(cenario, 0, 0, 0);
	}
	else
	{
		al_clear_to_color(al_map_rgb(0, 0, 0));
		al_draw_bitmap(gameover, 0, 0, 0);
	}
}

//modelo de rotação
void Rotate(Ponto *P, float Angle)
{

	float x = P->x, y = P->y;

	P->x = (x * cos(Angle)) - (y * sin(Angle));
	P->y = (y * cos(Angle)) + (x * sin(Angle));
}

//rotaciona o tanque
void rotacionaTanque(Tanque *t1)
{

	if (t1->vel_angular != 0)
	{

		Rotate(&t1->A, t1->vel_angular);
		Rotate(&t1->B, t1->vel_angular);
		Rotate(&t1->C, t1->vel_angular);

		t1->angulo += t1->vel_angular;
		t1->x_comp = cos(t1->angulo);
		t1->y_comp = sin(t1->angulo);
	}
}

bool colisaoCenario(Tanque *t1)
{
	float pontoX = t1->centro.x + t1->vel * t1->x_comp;
	float pontoY = t1->centro.y + t1->vel * t1->y_comp;

	float colisaoLateralEsquerda = pontoX - RAIO_CAMPO_FORCA - RAIO_TIRO < 0;
	float colisaoLateralDireita = pontoX + RAIO_CAMPO_FORCA + RAIO_TIRO > SCREEN_W;
	float colisaoSuperior = pontoY - RAIO_CAMPO_FORCA - RAIO_TIRO < 0;
	float colisaoInferior = pontoY + RAIO_CAMPO_FORCA + RAIO_TIRO > SCREEN_H;

	return !colisaoLateralEsquerda && !colisaoLateralDireita && !colisaoInferior && !colisaoSuperior;
}

float distancia(float x1, float x2, float y1, float y2)
{
	float distanciaX = x2 - x1;
	float distanciaY = y2 - y1;

	return sqrt((pow(distanciaX, 2) + pow(distanciaY, 2)));
}

bool colisaoBloco(Tanque t1, Bloco b)
{
	t1.centro.y += t1.vel * t1.y_comp;
	t1.centro.x += t1.vel * t1.x_comp;

	float distanciaMaxima = RAIO_CAMPO_FORCA + RAIO_TIRO + 1;
	float colisaoBlocoSupE = distancia(t1.centro.x, b.top_left.x, t1.centro.y, b.top_left.y);
	float colisaoBlocoSupD = distancia(t1.centro.x, b.bottom_right.x, t1.centro.y, b.top_left.y);
	float colisaoBlocoInfE = distancia(t1.centro.x, b.top_left.x, t1.centro.y, b.bottom_right.y);
	float colisaoBlocoInfD = distancia(t1.centro.x, b.bottom_right.x, t1.centro.y, b.bottom_right.y);

	///colisao com os vértices
	if (colisaoBlocoSupE <= distanciaMaxima || colisaoBlocoSupD <= distanciaMaxima || colisaoBlocoInfE <= distanciaMaxima || colisaoBlocoInfD <= distanciaMaxima)
	{
		return true;
	}
	// colisao com as laterais horizontais
	if (t1.centro.x >= b.top_left.x && t1.centro.x <= b.bottom_right.x)
	{

		if (t1.centro.y + distanciaMaxima >= b.top_left.y && t1.centro.y - distanciaMaxima <= b.bottom_right.y)
		{

			return true;
		}
	}
	//colisao com as laterais verticais
	if (t1.centro.y >= b.top_left.y && t1.centro.y <= b.bottom_right.y)
	{

		if (t1.centro.x + distanciaMaxima >= b.top_left.x && t1.centro.x - distanciaMaxima <= b.bottom_right.x)
		{

			return true;
		}
	}

	return false;
}

bool colisaoTiroBloco(Tiro tr1, Bloco b)
{

	float distanciaMaxima = RAIO_TIRO;
	float colisaoBlocoSupE = distancia(tr1.centro.x, b.top_left.x, tr1.centro.y, b.top_left.y);
	float colisaoBlocoSupD = distancia(tr1.centro.x, b.bottom_right.x, tr1.centro.y, b.top_left.y);
	float colisaoBlocoInfE = distancia(tr1.centro.x, b.top_left.x, tr1.centro.y, b.bottom_right.y);
	float colisaoBlocoInfD = distancia(tr1.centro.x, b.bottom_right.x, tr1.centro.y, b.bottom_right.y);

	///colisao com os vértices
	if (colisaoBlocoSupE <= distanciaMaxima || colisaoBlocoSupD <= distanciaMaxima || colisaoBlocoInfE <= distanciaMaxima || colisaoBlocoInfD <= distanciaMaxima)
	{
		return true;
	}
	// colisao com as laterais horizontais
	if (tr1.centro.x >= b.top_left.x && tr1.centro.x <= b.bottom_right.x)
	{

		if (tr1.centro.y + distanciaMaxima >= b.top_left.y && tr1.centro.y - distanciaMaxima <= b.bottom_right.y)
		{
			return true;
		}
	}
	//colisao com as laterais verticais
	if (tr1.centro.y >= b.top_left.y && tr1.centro.y <= b.bottom_right.y)
	{

		if (tr1.centro.x + distanciaMaxima >= b.top_left.x && tr1.centro.x - distanciaMaxima <= b.bottom_right.x)
		{
			return true;
		}
	}

	return false;
}

bool colisaoTanque(Tanque t1, Tanque t2)
{
	t1.centro.y += t1.vel * t1.y_comp;
	t1.centro.x += t1.vel * t1.x_comp;

	float cateto1 = t1.centro.x - t2.centro.x;
	float cateto2 = t1.centro.y - t2.centro.y;
	float hipotenusa = sqrt((pow(cateto1, 2) + pow(cateto2, 2)));
	bool colisao = RAIO_CAMPO_FORCA + RAIO_CAMPO_FORCA >= hipotenusa;

	return !colisao;
}

//atualiza o tanque
void atualizaTanque(Tanque *t1, Bloco b, Tanque tanques[])
{

	rotacionaTanque(t1);

	if (colisaoCenario(t1) && !colisaoBloco(*t1, b))
	{
		t1->centro.x += t1->vel * t1->x_comp;
		t1->centro.y += t1->vel * t1->y_comp;
	}

	for (int i = 0; i < MAX_JOGADORES; i++)
	{
		if (tanques[i].up != t1->up)
		{
			if (!colisaoTanque(*t1, tanques[i]))
			{
				t1->centro.x -= t1->vel * t1->x_comp;
				t1->centro.y -= t1->vel * t1->y_comp;
			}
		}
	}
}

bool colisaoTiro(Tanque *t1, Tiro *tr1)
{
	float cateto1 = t1->centro.x - tr1->centro.x;
	float cateto2 = t1->centro.y - tr1->centro.y;
	float hipotenusa = sqrt((pow(cateto1, 2) + pow(cateto2, 2)));
	bool colisao = RAIO_CAMPO_FORCA + RAIO_TIRO >= hipotenusa;

	return colisao;
}

//desenha o tanque
void desenhaTanque(Tanque t1)
{

	al_draw_circle(t1.centro.x, t1.centro.y, RAIO_CAMPO_FORCA, t1.cor, 1);

	al_draw_filled_triangle(t1.A.x + t1.centro.x, t1.A.y + t1.centro.y,
							t1.B.x + t1.centro.x, t1.B.y + t1.centro.y,
							t1.C.x + t1.centro.x, t1.C.y + t1.centro.y,
							t1.cor);
}

//
void atirar(Tiro *tr1)
{
	if (tr1->contador == 1)
	{
		tr1->vel_tiro = VEL_TIRO;
		tr1->centro.x -= tr1->vel_tiro * cos(tr1->angulo_tiro);
		tr1->centro.y -= tr1->vel_tiro * sin(tr1->angulo_tiro);
	}
}

bool colisaoTiroCenario(Tiro tr1)
{

	if (tr1.centro.x - RAIO_TIRO < 0)
	{
		return true;
	}
	if (tr1.centro.x + RAIO_CAMPO_FORCA > SCREEN_W)
	{
		return true;
	}
	if (tr1.centro.y - RAIO_TIRO < 0)
	{
		return true;
	}
	if (tr1.centro.y + RAIO_TIRO > SCREEN_H)
	{
		return true;
	}

	return false;
}

//atualiza tiro
void atualizaTiro(Tiro *tr1, Tanque *t1, Bloco b)
{
	if (tr1->vel_tiro == 0)
	{
		tr1->centro.x = t1->A.x + t1->centro.x;
		tr1->centro.y = t1->A.y + t1->centro.y;
		tr1->angulo_tiro = t1->angulo;
	}
}

//
void desenhaTiro(Tiro tr1)
{
	al_draw_circle(tr1.centro.x, tr1.centro.y, RAIO_TIRO, tr1.cor, 5);
}

void desenhaBloco(Bloco b)
{

	al_draw_rectangle(b.top_left.x, b.top_left.y, b.bottom_right.x, b.bottom_right.y, b.cor, 5);
}

void atualizaHistorico(int id)
{
	//Arquivos
	FILE *historico, *historicoAtualizado;
	historico = fopen("historico.txt", "r");
	historicoAtualizado = fopen("historicoAtualizado.txt", "w");

	int pontuacaoTanque1 = 0;
	int pontuacaoTanque2 = 0;
	int pontuacaoTanque3 = 0;
	char buffer[MAX_TAM];

	// verifica se o arquivo pode ser aberto e retorna return
	if (historico == NULL)
	{
		printf("\nNão foi possível abrir o arquivo!\n");
	}

	fgets(buffer, MAX_TAM, historico);

	while (!feof(historico))
	{
		pontuacaoTanque1 = atoi(strtok(buffer, ", "));
		pontuacaoTanque2 = atoi(strtok(NULL, ", "));
		pontuacaoTanque3 = atoi(strtok(NULL, ", "));

		fgets(buffer, MAX_TAM, historico);
	}

	if (id == 1)
	{
		pontuacaoTanque1 += 1;
	}
	else if (id == 2)
	{
		pontuacaoTanque2 += 1;
	}
	else
	{
		pontuacaoTanque3 += 1;
	}

	fprintf(historicoAtualizado, "%d, %d, %d\n", pontuacaoTanque1, pontuacaoTanque2, pontuacaoTanque3);

	fclose(historico);

	fclose(historicoAtualizado);

	remove("historico.txt");
	rename("historicoAtualizado.txt", "historico.txt");
}

void acessarHistorico(ALLEGRO_FONT *size_50)
{
	char ch;
	char buffer[MAX_TAM];
	FILE *arquivo;
	arquivo = fopen("historico.txt", "r"); // modo de leitura

	int vitoriasJogador1 = 0;
	int vitoriasJogador2 = 0;
	int vitoriasJogador3 = 0;

	if (arquivo == NULL)
	{
		printf("Erro ao abrir o arquivo. \n");
	}

	fgets(buffer, MAX_TAM, arquivo);
	while (!feof(arquivo))
	{
		vitoriasJogador1 = atoi(strtok(buffer, ", "));
		vitoriasJogador2 = atoi(strtok(NULL, ", "));
		vitoriasJogador3 = atoi(strtok(NULL, ", "));
		fgets(buffer, MAX_TAM, arquivo);
	}

	printf("\n %d --- %d --- %d", vitoriasJogador1, vitoriasJogador2, vitoriasJogador3);
	char jogador1[50];
	sprintf(jogador1, "Jogador 1 ---------------- %d vitórias", vitoriasJogador1);
	char jogador2[50];
	sprintf(jogador2, "Jogador 2 ---------------- %d vitórias", vitoriasJogador2);
	char jogador3[50];
	sprintf(jogador3, "Jogador 3 ---------------- %d vitórias", vitoriasJogador3);

	al_draw_text(size_50, al_map_rgb(255, 255, 255), (SCREEN_H / 2) + 150, (SCREEN_W / 2) + 100, ALLEGRO_ALIGN_CENTRE, jogador1);
	al_draw_text(size_50, al_map_rgb(255, 255, 255), (SCREEN_H / 2) + 150, (SCREEN_W / 2) + 150, ALLEGRO_ALIGN_CENTRE, jogador2);
	al_draw_text(size_50, al_map_rgb(255, 255, 255), (SCREEN_H / 2) + 150, (SCREEN_W / 2) + 200, ALLEGRO_ALIGN_CENTRE, jogador3);
}

//main
int main(int argc, char **argv)
{

	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_SAMPLE *sampleTiro = NULL;
	ALLEGRO_SAMPLE *space = NULL;

	//----------------------- rotinas de inicializacao ---------------------------------------

	//inicializa o Allegro
	if (!al_init())
	{
		fprintf(stderr, "failed to initialize allegro!\n");
		return -1;
	}

	//inicializa o módulo de primitivas do Allegro
	if (!al_init_primitives_addon())
	{
		fprintf(stderr, "failed to initialize primitives!\n");
		return -1;
	}

	//inicializa o modulo que permite carregar imagens no jogo
	if (!al_init_image_addon())
	{
		fprintf(stderr, "failed to initialize image module!\n");
		return -1;
	}

	//cria um temporizador que incrementa uma unidade a cada 1.0/FPS segundos
	timer = al_create_timer(1.0 / FPS);
	if (!timer)
	{
		fprintf(stderr, "failed to create timer!\n");
		return -1;
	}

	//cria uma tela com dimensoes de SCREEN_W, SCREEN_H pixels
	display = al_create_display(SCREEN_W, SCREEN_H);
	if (!display)
	{
		fprintf(stderr, "failed to create display!\n");
		al_destroy_timer(timer);
		return -1;
	}

	//instala o teclado
	if (!al_install_keyboard())
	{
		fprintf(stderr, "failed to install keyboard!\n");
		return -1;
	}

	//inicializa o modulo allegro que carrega as fontes
	al_init_font_addon();

	//inicializa o modulo allegro que entende arquivos tff de fontes
	if (!al_init_ttf_addon())
	{
		fprintf(stderr, "failed to load tff font module!\n");
		return -1;
	}

	//carrega o arquivo arial.ttf da fonte Arial e define que sera usado o tamanho 32 (segundo parametro)
	ALLEGRO_FONT *size_32 = al_load_font("./fontes/guardian.ttf", 32, 1);
	ALLEGRO_FONT *size_100 = al_load_font("./fontes/zrnic.ttf", 100, 1);
	ALLEGRO_FONT *size_50 = al_load_font("./fontes/zrnic.ttf", 50, 1);
	if (size_32 == NULL)
	{
		fprintf(stderr, "font file does not exist or cannot be accessed!\n");
	}

	//cria a fila de eventos
	event_queue = al_create_event_queue();
	if (!event_queue)
	{
		fprintf(stderr, "failed to create event_queue!\n");
		al_destroy_display(display);
		return -1;
	}

	if (!al_install_audio())
	{
		fprintf(stderr, "failed to initialize audio!\n");
		return -1;
	}

	if (!al_init_acodec_addon())
	{
		fprintf(stderr, "failed to initialize audio codecs!\n");
		return -1;
	}

	sampleTiro = al_load_sample("./songs/tiro.ogg");
	space = al_load_sample("./songs/ambiente.ogg");

	if (!sampleTiro)
	{
		fprintf(stderr, "audio clip score not loaded!\n");
		return -1;
	}

	if (!al_reserve_samples(4))
	{
		fprintf(stderr, "Falha ao alocar canais de áudio.\n");
		return false;
	}

	//imagem

	cenario = al_load_bitmap("./imagens/cenario.jpg");
	if (!cenario)
	{
		fprintf(stderr, "Falha ao alocar canais de imagem.\n");
		return false;
	}

	gameover = al_load_bitmap("./imagens/gameover.jpg");
	if (!gameover)
	{
		fprintf(stderr, "Falha ao alocar canais de imagem.\n");
		return false;
	}

	//registra na fila os eventos de tela (ex: clicar no X na janela)
	al_register_event_source(event_queue, al_get_display_event_source(display));
	//registra na fila os eventos de tempo: quando o tempo altera de t para t+1
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	//registra na fila os eventos de teclado (ex: pressionar uma tecla)
	al_register_event_source(event_queue, al_get_keyboard_event_source());

	char score[10];

	//cria tanque

	
	Tanque tanques[MAX_JOGADORES];

	Tanque tanque1;
	tanque1.id = 1;
	tanque1.up = ALLEGRO_KEY_W;
	tanque1.down = ALLEGRO_KEY_S;
	tanque1.left = ALLEGRO_KEY_A;
	tanque1.right = ALLEGRO_KEY_D;
	tanque1.fire = ALLEGRO_KEY_SPACE;
	tanque1.cor = al_map_rgb(rand() % 256, rand() % 256, rand() % 256);

	Tanque tanque2;
	tanque2.id = 2;
	tanque2.up = ALLEGRO_KEY_UP;
	tanque2.down = ALLEGRO_KEY_DOWN;
	tanque2.left = ALLEGRO_KEY_LEFT;
	tanque2.right = ALLEGRO_KEY_RIGHT;
	tanque2.fire = ALLEGRO_KEY_ENTER;
	tanque2.cor = al_map_rgb(rand() % 256, rand() % 256, rand() % 256);

	Tanque tanque3;
	tanque3.id = 3;
	tanque3.up = ALLEGRO_KEY_U;
	tanque3.down = ALLEGRO_KEY_J;
	tanque3.left = ALLEGRO_KEY_H;
	tanque3.right = ALLEGRO_KEY_K;
	tanque3.fire = ALLEGRO_KEY_O;
	tanque3.cor = al_map_rgb(rand() % 256, rand() % 256, rand() % 256);

	tanques[0] = tanque1;
	tanques[1] = tanque2;
	tanques[2] = tanque3;

	for (int i = 0; i < MAX_JOGADORES; i++)
	{
		initTanque(&tanques[i], (SCREEN_W / (i + 1)) - 220, tanques[i].cor);
	}

	Bloco bloco_1;
	initBloco(&bloco_1);

	ListaTiro *atirados;
	atirados = malloc(sizeof(ListaTiro));
	atirados->nelem = 0;
	atirados->head = NULL;

	Tiro tiros[MAX_JOGADORES];

	for (int i = 0; i < MAX_JOGADORES; i++)
	{
		Tiro tiro;
		// tiro.cor = al_map_rgb(rand() % 256, rand() % 256, rand() % 256);
		tiro.cor = tanques[i].cor;
		tiros[i] = tiro;
		initTiro(&tiros[i], tanques[i]);
	}

	//inicia o temporizador
	al_start_timer(timer);

	int playing = 1;

	al_play_sample(space, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);

	while (playing)
	{
		ALLEGRO_EVENT ev;
		//espera por um evento e o armazena na variavel de evento ev
		al_wait_for_event(event_queue, &ev);

		//se o tipo de evento for um evento do temporizador, ou seja, se o tempo passou de t para t+1
		if (ev.type == ALLEGRO_EVENT_TIMER)
		{

			//chama funcoes
			desenhaCenario(pontos);
			if (score == NULL)
			{
				sprintf(score, "0");
			}

			for (int i = 0; i < MAX_JOGADORES; i++)
			{
				atualizaTanque(&tanques[i], bloco_1, tanques);
				desenhaTanque(tanques[i]);
			}
			desenhaBloco(bloco_1);
			for (int i = 0; i < MAX_JOGADORES; i++)
			{
				atualizaTiro(&tiros[i], &tanques[i], bloco_1);
				desenhaTiro(tiros[i]);
			}

			Atirados *p = atirados->head;
			while (p != NULL)
			{
				atirar(&p->elem);
				desenhaTiro(p->elem);
				if (colisaoTiroCenario(p->elem) || colisaoTiroBloco(p->elem, bloco_1))
				{
					condicao = true;
					RemoveTiro(p->elem, atirados);
				}
				for (int i = 0; i < MAX_JOGADORES; i++)
				{

					if (colisaoTiro(&tanques[i], &p->elem))
					{
						p->elem.quemDeu->pontuacao++;
						pontos = p->elem.quemDeu->pontuacao;
						id = p->elem.quemDeu->id;
						condicao = true;
						RemoveTiro(p->elem, atirados);
					}
					if (pontos == 5)
					{
						printf("GANHADOR %d", id);
						atualizaHistorico(id);
						playing = 0;
						break;
					}
				}

				if (playing == 0)
				{
					break;
				}
				p = p->lig;
			}

			if (playing == 0)
			{
				break;
			}

			for (int i = 0; i < MAX_JOGADORES; i++)
			{
				sprintf(score, "%d", tanques[i].pontuacao);
				al_draw_text(size_32, tanques[i].cor, (SCREEN_W / (i + 1)) - 220, 20, ALLEGRO_ALIGN_CENTRE, score);
			}

			//atualiza a tela (quando houver algo para mostrar)
			al_flip_display();

			// if (al_get_timer_count(timer) % (int)FPS == 0)
			// 	printf("\n%d segundos se passaram...", (int)(al_get_timer_count(timer) / FPS));
		}
		//se o tipo de evento for o fechamento da tela (clique no x da janela)
		else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
		{
			playing = 0;
			printf("\nSaindo...");
		}
		//se o tipo de evento for um pressionar de uma tecla
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN)
		{
			// imprime qual tecla foi
			printf("\ncodigo tecla: %d", ev.keyboard.keycode);

			// rotacao do tanque
			for (int i = 0; i < MAX_JOGADORES; i++)
			{
				int k = ev.keyboard.keycode;
				if (k == tanques[i].up)
				{
					tanques[i].vel -= VEL_TANQUE;
				}
				else if (k == tanques[i].down)
				{
					tanques[i].vel += VEL_TANQUE;
				}
				else if (k == tanques[i].right)
				{
					tanques[i].vel_angular += PASSO_ANGULO;
				}
				else if (k == tanques[i].left)
				{
					tanques[i].vel_angular -= PASSO_ANGULO;
				}
				else if (k == tanques[i].fire)
				{
					for (int i = 0; i < MAX_JOGADORES; i++)
					{
						if (k == tanques[i].fire)
						{
							if (tiros[i].contador == 0)
							{
								tiros[i].contador = 1;
								tiros[i].quemDeu = &tanques[i];
							}

							al_play_sample(sampleTiro, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
							Atirados *a;
							a = malloc(sizeof(Atirados));
							a->elem = tiros[i];
							a->lig = atirados->head;

							atirados->head = a;
							atirados->nelem++;

							// Cria novo tiro pronto para atirar
							Tiro novoTiro;
							//novoTiro.cor = al_map_rgb(rand() % 256, rand() % 256, rand() % 256);
							novoTiro.cor = tanques[i].cor;
							tiros[i] = novoTiro;
							initTiro(&tiros[i], tanques[i]);
							condicao = false;
						}
					}
				}
			}
		}

		//se o tipo de evento for um pressionar de uma tecla
		else if (ev.type == ALLEGRO_EVENT_KEY_UP)
		{
			//imprime qual tecla foi
			printf("\ncodigo tecla: %d", ev.keyboard.keycode);

			// aumentar a velocidade enquanto a tecla "x" estiver precionada
			for (int i = 0; i < MAX_JOGADORES; i++)
			{
				int k = ev.keyboard.keycode;
				if (k == tanques[i].up)
				{
					tanques[i].vel += VEL_TANQUE;
				}
				else if (k == tanques[i].down)
				{
					tanques[i].vel -= VEL_TANQUE;
				}
				else if (k == tanques[i].right)
				{
					tanques[i].vel_angular -= PASSO_ANGULO;
				}
				else if (k == tanques[i].left)
				{
					tanques[i].vel_angular += PASSO_ANGULO;
				}
				else if (k == tanques[i].fire)
				{
					// Tiro novoTiro;
					// tiros[i] = novoTiro;
					// initTiro(&tiros[i], tanques[i]);
				}
			}
		}

	} //fim do while

	char vencedor[50];
	sprintf(vencedor, "Jogador %d", id);
	char vencedor2[50];
	sprintf(vencedor2, "VENCEU");

	desenhaCenario(pontos);
	acessarHistorico(size_50);

	al_draw_text(size_100, al_map_rgb(255, 255, 255), (SCREEN_H / 2) + 153, (SCREEN_W / 2) - 397, ALLEGRO_ALIGN_CENTRE, vencedor);
	al_draw_text(size_100, al_map_rgb(72, 61, 139), (SCREEN_H / 2) + 150, (SCREEN_W / 2) - 400, ALLEGRO_ALIGN_CENTRE, vencedor);
	al_draw_text(size_100, al_map_rgb(255, 255, 255), (SCREEN_H / 2) + 153, (SCREEN_W / 2) - 297, ALLEGRO_ALIGN_CENTRE, vencedor2);
	al_draw_text(size_100, al_map_rgb(72, 61, 139), (SCREEN_H / 2) + 150, (SCREEN_W / 2) - 300, ALLEGRO_ALIGN_CENTRE, vencedor2);

	al_flip_display();
	al_rest(10);
	//procedimentos de fim de jogo (fecha a tela, limpa a memoria, etc)

	al_destroy_timer(timer);
	al_destroy_display(display);
	al_destroy_event_queue(event_queue);
	al_destroy_sample(sampleTiro);

	return 0;
}