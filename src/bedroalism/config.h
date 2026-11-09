#ifndef BR_CONFIG_H
#define BR_CONFIG_H

// Color correction, applied to everything that is drawn
#define BR_EXPOSURE 1.2  // 0.5 dark ~ 3.0 bright
#define BR_GAMMA    1.25 // 0.3 low ~ 2.0 high, Newb uses 1.33

// Uchimura tonemap
#define BR_UCHIMURA_P 1.0  // max display brightness
#define BR_UCHIMURA_A 1.1  // contrast, 1.0 neutral ~ 1.3 punchy
#define BR_UCHIMURA_M 0.2  // where the straight section starts
#define BR_UCHIMURA_L 0.5  // length of the straight section
#define BR_UCHIMURA_C 1.3  // black tightness, 1.0 lifted ~ 1.6 deep
#define BR_UCHIMURA_B 0.0  // pedestal, keep 0.0 so black stays black on additive layers (sun, stars)

#endif
