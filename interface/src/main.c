#include<stdbool.h>
#include<assert.h>
#include<dlfcn.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
#include<unistd.h>

#include<SDL/SDL.h>
#include<SDL/SDL_ttf.h>

#include"backgammon.h"
#include"init.h"
#include"game.h"
#include"graph.h"
#include"logger.h"
#include"human.h"

void err(String str){
    fprintf(stderr, "%s -> %s\n",str, dlerror());
    exit(EXIT_FAILURE);
}

Player choose_start_player(unsigned int i)
{
    // Le paramêtre sert d'arrêt à la reccursion
    // Il est arrivé que le programme s'arrête sur une
    // segfault causé par un trop grand nombre d'appel récursif :
    // les valeurs des dés renvoyé par rand était toujours les mêmes
    // Si au bout de 10 lancés, les dés sont identiques, le joueur noir
    // commence par défaut
    unsigned char dice[2];
    roll_dice(dice);
    Player tmp = BLACK;
    if( dice[BLACK] > dice[WHITE])
        tmp = BLACK;
    else if( dice[WHITE] > dice[BLACK] )
        tmp = WHITE;
    else if( i<10 )
        tmp = choose_start_player(++i);
    return tmp;
}

int main(int ARGC, const char* ARGV[])
{
    init_logger();

    unsigned int target_score = 15 ;
    
    set_level("main_logger", LOG_LVL);
    set_level("refere_logger", LOG_LVL);
    set_level("score_logger", INFO);
    set_file("score_logger", "score.log");
    set_file("main_logger", NULL);
    set_simple_print("score_logger", true);

    if (ARGC >= 2)
    {
        char* pEnd;
        target_score = (int)strtol(ARGV[1],&pEnd,0);

        if (strcmp(pEnd,"\0") != 0)
        {
            //Erreur de target_score ...
            perror("ERREUR : Invalide parametre target_score (doit etre entier et 1er parametre)");
            exit(EXIT_FAILURE);
        }

        if (target_score <= 0)
        {
            //stupide de faire un score nul comme objectif ...
            perror("ERREUR : target_score negatif ou nul");
            exit(EXIT_FAILURE);
        }

        char mess[50];
        sprintf(mess, "Lecture de target score %d\n", target_score);
        logging("main_logger", mess, WARNING);
    }
    else
    {
        char mess[50];
        sprintf(mess, "Pas de target_score fourni, pas defaut %i\n",target_score);
        logging("main_logger", mess, WARNING);
    }

    // Utile pour l'affichage des joueurs
    const char* const enumToStr[] = {"NOBODY", "BLACK", "WHITE"};

    IA players[2];
    players[0].lib_path=(char*)"./strategy/bin/libpote.so-dev";
    players[1].lib_path=(char*)"./strategy/bin/libpote.so-dev";
    SDL_Surface* screen = initGraph();//graph
    drawBackground(screen);
    SDL_Flip(screen);
    sleep(2);
    // --- Initialisation des bibliothèques
    for(int i=0; i<2; ++i)
    {
        if (ARGC >= 3+i)
        {
            // Si c'est une IA
            players[i].lib_path=(char*)calloc(strlen(ARGV[2+i])+1,sizeof(char));
            strcpy(players[i].lib_path,ARGV[2+i]);
            players[i].func = (Functions*)malloc( sizeof(Functions) );
            players[i].lib_handle = NULL;
            init_lib( players[i].lib_path , &(players[i].lib_handle), players[i].func, err);
        }
        else
        {
            // Si c'est un humain
            players[i].func = StartScreen(screen);
            players[i].lib_path = NULL;
        }
        players[i].func->initLibrary( (players[i].name) );
        char mess[50];
        sprintf(mess, "%s I.A. : %s\n", enumToStr[i+1],players[i].name );
        logging("main_logger", mess, WARNING);
        players[i].match_won = 0;
    }

    // --- Initialisation du jeux
    SGameState state;
    init_state(&state);
    drawBoard(&state,screen);

    for(unsigned int i=0; i<24; ++i)
    {
        char mess[50];
        sprintf(mess, "case %2d : owner %6s, nbDames %d\n", i, enumToStr[state.board[i].owner+1], state.board[i].nbDames);
        logging("main_logger", mess, WARNING);
    }
    players[WHITE].func->startMatch(target_score);
    players[BLACK].func->startMatch(target_score);
    bool finished               = false;
    const unsigned int maxScore = target_score;
    unsigned int turn_num       = 1;
    srand(time(NULL));
    Player winner               = NOBODY;
    
    // --- Boucle principale
    while(!finished)
    {
        char mess[50];
        sprintf(mess, "Début de la manche %d\n", turn_num);
        logging("main_logger", mess, WARNING);
        
        Player current = choose_start_player(0);

        sprintf(mess, "%s commence\n", enumToStr[current+1]);
        logging("main_logger", mess, WARNING);
        
        for(unsigned int i=0; i<2; ++i)
        {
            players[i].func->startGame( (Player)i );
            players[i].tries = 3;
        }
        bool end_of_round      = false;
        state.stake            = 1;
        winner                 = NOBODY;
        Player lastStaker      = NOBODY;
        state.turn             = 1;
        init_board(&state);
        while(!end_of_round)
        {
            sprintf(mess,"Début du tour %d, Joueur : %s\n", state.turn, enumToStr[current+1]);
            logging("main_logger", mess, WARNING);

            // Si le joueur est humains, on lui renvoie sa couleur, ça permet d'éviter
            // des erreurs du au fait que la variable contenant la couleur du joueur humain soit
            // global (cf human.h). Cette erreur survenais lorsqu'il y a deux humains qui jouent
            // en même temps.
            if (ARGC < 3+current){
                players[current].func->startGame(current);
            }
            end_of_round = gamePlayTurn(&state, players, current, &lastStaker, &winner, screen);
    	    drawBackground(screen);
            drawBoard(&state,screen);//graph
            SDL_Delay(1000);

	    	sprintf(mess,"fin du tour %d\n", state.turn);
            logging("main_logger", mess, WARNING);
            
            current = (Player)(1-current);
            ++state.turn;
        }
        for(unsigned int i=0; i<2; ++i) players[i].func->endGame();
        if(winner==WHITE)
        {
            state.whiteScore += state.stake;
            finished          = state.whiteScore>=maxScore;
            players[WHITE].match_won++;
        }
        else
        {
            state.blackScore += state.stake;
            finished          = state.blackScore>=maxScore;
            players[BLACK].match_won++;
        }

        int score = state.blackScore;
        if(winner==WHITE) score = state.whiteScore;
        sprintf(mess, "gagnant : %s, gagne %d points (total %d )\n", enumToStr[winner+1], state.stake, score);
        logging("main_logger", mess, WARNING);
        sprintf(mess, "fin de la manche %d\n", turn_num);
        logging("main_logger", mess, WARNING);
        printf("gagnant : %s, gagne %d points (total %d )\n", enumToStr[winner+1], state.stake, score);
        
        ++turn_num;
    }
    for(unsigned int i=0; i<2; ++i) players[i].func->endMatch();

    // --- Fermeture des bibliothèques
    char mess[100];
    sprintf(mess,"%s:%s gagne avec %d match gagné\n", enumToStr[winner+1], players[winner].name, players[winner].match_won);
    logging("main_logger", mess, WARNING);
    printf("%s:%s gagne avec %d match gagné\n", enumToStr[winner+1], players[winner].name, players[winner].match_won);
    int score = winner==BLACK ? state.blackScore : state.whiteScore;
    sprintf(mess, "%s , %d\n", enumToStr[winner+1], score);
    logging("score_logger", mess, INFO);
    free_logger();

    endGraph();//graph
    for(int i=0; i<2; ++i)
    {
        free(players[i].func);
        if (ARGC >= 3+i){
            dlclose(players[i].lib_handle);
            free(players[i].lib_path);
        }
    }
    return EXIT_SUCCESS;
}
