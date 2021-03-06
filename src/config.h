#ifndef TSP_CHALLENGE_KIWI_CONFIG_H
#define TSP_CHALLENGE_KIWI_CONFIG_H

class config
{
public:

    static double INITIAL_TEMP;
    static double COOLING_TEMP;

    static float GREEDY_SEARCH_EXP;
    static int GREEDY_SEARCH_KNBRS;
    static float GREEDY_SEARCH_RATIO; // 1 -> average, 0 -> min
	static int GREEDY_SEARCH_TIME;

#define K_NEIGHBOURS

};

#endif //TSP_CHALLENGE_KIWI_CONFIG_H
