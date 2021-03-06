#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "AI_main.h"
#include "utils.h"
#include "possibilities.h"

CompactPlayer ai_player;



uint8_t somme_plateau(const CompactGameState* etat_jeu,CompactPlayer player)
{
    uint8_t somme = 0 ;
    somme += etat_jeu->bar[player];
    for (int i = 0 ; i < 24 ; i++ )
    {
        if (etat_jeu->board[i].owner == player)
            somme += etat_jeu->board[i].nbDames ;
    }
    somme += etat_jeu->out[player];
    return somme;
}

static long moyenne_combinaisons(long alpha_valeurs[21])
{
    long alpha = 0;
	/*	les arrangements possibles sont 1,1 ; 1,2 ; 1,3 ; ... 2,1 ; 2,2 ; ... 6,6
		donc 36 possibilités
		or on n'a calculé que pour les combinaisons disponibles ,donc les doublons du type 4,5 et 5,4 sont enlevés
		pour la moyenne il faut donc doubler la probabilité des sets non doubles
	*/
	// 1,1 ; 1,2 ; 1,3 ; 1,4 ; 1,5 ; 1,6 ; 2,2 ; 2,3 ; 2,4 ; 2,5 ; 2,6 ; 3,3 ;
    for (int i = 0 ; i < 21 ; i++)
    {
		if (i == 0 || i == 6 || i == 11 || i == 15 || i == 18 || i == 20) 
			// par constructio ndes combinaisons de dés, les doubles se situent aux positions 0,6,11,15,18,20 dans le tableau
			alpha += alpha_valeurs[i] ;
			// on ne compte qu'une fois pour les dés doubles
		else
			alpha += (alpha_valeurs[i]*2);
			// mais 2 fois pour les dés simples
    }

    alpha /= 36 ;

    return alpha;
}

void genererDes(unsigned char des[21][2])
{
    size_t i = 0 ;
    for (size_t x = 1 ; x <= 6 ; x++)
    {
        for (size_t y = x ; y <= 6 ;y++)
        {
            des[i][0] = x ;
            des[i++][1] = y ;
        }
    }
    // renvoie toutes les combinaisosn de des possibles

}

long alphabeta(CompactGameState etat_jeu, int profondeur,int profondeur_initial, long alpha, long beta, CompactPlayer joueur_calcule, CompactPlayer AI_player,AIListMoves *moves , const unsigned char des[2])
{
	// pour plus de détails sur l'algorithme, voir sur votre moteur de recherche préféré "alpha-beta pruning"

    assert(joueur_calcule >= 0);
    // on verifie que le joueur duquel on se place est correct ...

    assert(AI_player >= 0);
    // on verifie aussi que notre AI est correcte ...

    assert(moves);
    // que le pointeur de moves != NULL (sinon difficile de d'affecter une valeur à l'adresse NULL)

    assert(des[0] >= 1 && des[0] <= 6 && des[1] >= 1 && des[1] <= 6 );
    // on verifie que les dés envoyés sont valides

    assert(somme_plateau(&etat_jeu,CWHITE) == 15 );
    assert(somme_plateau(&etat_jeu,CBLACK) == 15 );

    if (profondeur == 0 || isGameFinished(&etat_jeu))
    {
		// c'est un noeud final, on renvoit une valeur de plateau
        int heuristic_value = getValueFromGameState(&etat_jeu,AI_player);
        return heuristic_value ;
    }

    unsigned char toutes_combinaisons_des[21][2] ;
    genererDes(toutes_combinaisons_des);
    // on genere toutes les combinaisons de des possibles pour la suite

    ArrayList *liste_possibilites = retrieveEveryPossibility(etat_jeu,joueur_calcule,des);
	// on génère tous les mouvements possibles à partir de l'état de jeu donné

    assert(liste_possibilites);

    long v;
    if (joueur_calcule == AI_player)
    {
		// on se place comme si nous devions faire un mouvement
        v = LONG_MIN ; // equivaut à moins l'infini
        for (size_t i = 0 ; beta > alpha && i < list_size(liste_possibilites) ; i++)
        {
            AIListMoves temp_moves;

            list_get(liste_possibilites, i, &temp_moves);
            long alpha_valeurs[21] ;
            // les valeurs de alpha-beta pour chaque combinaison de dé
            // par la suite on fera la moyenne de ces valeurs
            for (size_t combinaison_de = 0 ; combinaison_de < 21 ; combinaison_de++)
            {
				// calcul de alphabeta pour chaque combinaison de dé possible après un tour de jeu
                // set de dés utilisés pour le calcul ; i.e. (1,2) ou (5,6) ou (6,6) ...
                unsigned char set_de_actuel[2] = {
                    toutes_combinaisons_des[combinaison_de][0],
                    toutes_combinaisons_des[combinaison_de][1],
                };

                alpha_valeurs[combinaison_de] = alphabeta(	gameStateFromMovement(etat_jeu, temp_moves, joueur_calcule)
											,profondeur - 1 
											,profondeur_initial
                                            ,alpha
                                            ,beta
                                            ,opposing_player(joueur_calcule)
                                            ,AI_player
                                            ,moves
                                            ,set_de_actuel);
            }
            long alpha_calcul = moyenne_combinaisons(alpha_valeurs);

            if (v < alpha_calcul && profondeur_initial == profondeur)
            {
				// on vient de trouver une meilleure branche, et il s'avère qu'on est à la profondeur d'origine
				// on retourne donc le set de mouvement utilisé pour faire de calcul alphabeta
                *moves = temp_moves ;
            }

            v = max_long(v, alpha_calcul);
            alpha = max_long(alpha, v);
        }
    }
    else
    {
		// sinon on se place comme si l'ennemi doit faire un mouvement
        v = LONG_MAX ; // equivaut à plus l'infini
        for (size_t i = 0 ; beta > alpha && i < list_size(liste_possibilites) ; i++)
        {
            AIListMoves temp_moves;
            list_get(liste_possibilites, i, &temp_moves);
            long alpha_valeurs[21] ;
            for (size_t combinaison_de = 0 ; combinaison_de < 21 ; combinaison_de++)
            {
                // set de dés utilisés pour le calcul ; i.e. (1,2) ou (5,6) ou (6,6) ...
                unsigned char set_de_actuel[2] = {
                    toutes_combinaisons_des[combinaison_de][0],
                    toutes_combinaisons_des[combinaison_de][1],
                };

                alpha_valeurs[combinaison_de] = alphabeta(	gameStateFromMovement(etat_jeu, temp_moves, joueur_calcule)
                        ,profondeur - 1
                        ,profondeur_initial
                        ,alpha
                        ,beta
                        ,opposing_player(joueur_calcule)
                        ,AI_player
                        ,moves
                        ,set_de_actuel);
            }
            long alpha_calcul = moyenne_combinaisons(alpha_valeurs);

            v = min_long(v, alpha_calcul);
            beta = min_long(beta, v);
        }
    }

    list_free(liste_possibilites);
    return v;
}

// objectif : a partir d'un GameState, trouver le meilleur set de mouvement
AIListMoves getBestMoves(CompactGameState etat_jeu, CompactPlayer player,const unsigned char dices[2])
{
    /*
    solution : algorithme alpha beta
    similaire à l'algorithme minimax
    principe : on calcule toutes les possibilités jusqu'à une profondeur donnée
    a chaque feuille (noeud final), on retourne une valeur qui decrit en un entier l'etat du joueur
    plus elle est élevée, plus le joueur se porte bien dans la partie
    plus elle est faible, plus le joueur se trouve dans une position non avantageuse

    on cherche à maximiser nos gains et minimiser ceux de l'adversaire

    minimax cherche à toujours prendre le chemin le plus sur;
    on part du principe que l'ennemi fait toujours le meilleur choix

    alphabeta est une version améliorée de minimax qui évite de calculer toutes les branches,
    si l'une des branches aboutit sur une valeur plus faible qu'une branche frère antérieure,
    on arrete le calcul pour ce set de branche
    (plus de détail sur l'algorithme : chercher AlphaBeta Pruning sur un moteur de recherche)
    */
    assert(!isGameFinished(&etat_jeu));

    AIListMoves moves ;
    // appel theorique de alphabeta : alphabeta(Noeud,profondeur_de_base,-infini,+infini)
    alphabeta(    etat_jeu, // etat du jeu courant, necessaire pour calculer les possibilites
                2,
                2,// profondeur de calcul souhaité, attention, augmenter de 1 peut prendre beaucou plus de temps!
                LONG_MIN, // moins l'infini version machine
                LONG_MAX, // plus l'infini version machine
                player, // quel joueur nous sommes
                player, // quel joueur nous simulons le tour de jeu (au debut, nous, apr_s, l'adversaire)
                &moves,// il faut bien recuperer les mouvements qu'on a fait au final, c'est en envoyant ce pointeur qu'on le fait
                dices);// dés à disposition
    //
    return moves ;
}

// pour un etat de jeu donné, renvoie un entier decrivant l'état du joueur player
// plus il est élevé, plus le joueur est dans une bonne position
// les états sont normalement symétriques ( value(J1) = - value(J2) )
int getValueFromGameState(const CompactGameState* etat_jeu, CompactPlayer player)
{

    const int BAR_VALUE = -55 ;
    const int OUT_VALUE = 35 ;
    const int INPLAY_VALUE_BASE = 0 ;
    const int INPLAY_VALUE_DELTA = 1 ;
    const int INPLAY_MALUS_ALONE = 10 ;
	const int INPLAY_MALUS_DELTA = 2 ;
	// Toutes ces valeurs ont été trouvées "au feeling", 
	//rien ne dit que les valeurs de ces constantes soient optimisées pour la stratégie

    int heuristic_value = 0 ;
    // calcul de la valeur heuristique en partant du principe qu'on est le joueur WHITE
    // (on multipliera par -1 si on est en réalité le joueur BLACK)

    heuristic_value += etat_jeu->bar[CWHITE]* BAR_VALUE;
    heuristic_value -= etat_jeu->bar[CBLACK]* BAR_VALUE;
    // prise en compte des pions sur la barre verticale

    heuristic_value += etat_jeu->out[CWHITE]* OUT_VALUE;
    heuristic_value -= etat_jeu->out[CBLACK]* OUT_VALUE;
    // prise en compte des pions sortis du jeu

    for (int i = 0 ; i < 24 ; i++)
    {
        CompactSquare current_square = etat_jeu->board[i] ;
        if (current_square.owner == CWHITE)
        {
            if (current_square.nbDames == 1)
                heuristic_value -= INPLAY_MALUS_ALONE + (INPLAY_MALUS_DELTA *(i+1));
                // on enleve des points si le pion est tout seul

            heuristic_value += current_square.nbDames * (INPLAY_VALUE_BASE + ((i+1) * INPLAY_VALUE_DELTA));
			// plus les pions sont avancés sur le terrain, plus ils valent de points
        }
        else if (current_square.owner == CBLACK)
        {
            if (current_square.nbDames == 1)
                heuristic_value += INPLAY_MALUS_ALONE + (INPLAY_MALUS_DELTA *(24-i)) ;
            heuristic_value -= current_square.nbDames * (INPLAY_VALUE_BASE + ((24-i) * INPLAY_VALUE_DELTA));
        }
    }

    if (player == CBLACK)
        heuristic_value *= -1;

    return heuristic_value ;
}

CompactGameState gameStateFromMovement(CompactGameState etat_jeu, AIListMoves mouvements,CompactPlayer player)
{
    // a partir d'un etat de jeu et de mouvements définis, renvoie un autre etat de jeux.
    // WARNING : CELA NE VERIFIE PAS QUE LE MOUVEMENT EST VALIDE !
    // A UTILISER AVEC PARCIMONIE !

    for (size_t i = 0 ; i < mouvements.nombre_mouvements ; i++)
    {
        CompactMove current_move = mouvements.mouvement[i] ;
        etat_jeu = apply_move(etat_jeu,player,current_move);
    }

    return etat_jeu;
}

bool isGameFinished(const CompactGameState* etat_jeu)
{
    return (etat_jeu->out[CWHITE] >= 15 || etat_jeu->out[CBLACK] >= 15 ) ;
}
