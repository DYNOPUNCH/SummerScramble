/*
 * main.c <SummerScramble>
 * 
 * Summer Scramble's entry point
 * 
 */

#include "game.h"

int main(void)
{
	struct Game *game = GameNew();
	
	GameInit(game);
	
	while (1)
	{
		if (GameStep(game))
			break;
		
		GameDraw(game);
	}
	
	GameCleanup(game);
	
	GameDelete(game);
	
	return 0;
}

