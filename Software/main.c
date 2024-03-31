/* MAIN.C file
 * https://github.com/JacobNeergaard/powerunit
 */

#include "stm8s.h"

#define PERIOD 9000
#define SHORT_CLICK 1000
#define LONG_CLICK 10000

#define LEFTOUT_PORT GPIOD
#define LEFTOUT_PIN GPIO_PIN_6
#define RIGHTOUT_PORT GPIOC
#define RIGHTOUT_PIN GPIO_PIN_5

#define BTNLEFT_PORT GPIOA
#define BTNLEFT_PIN GPIO_PIN_3
#define BTNRIGHT_PORT GPIOB
#define BTNRIGHT_PIN GPIO_PIN_4

void Delay(uint16_t nCount);
void InitGPIO(void);
void SetOutput(uint8_t mode);

// Button thresholds
uint16_t left_threshold;
uint16_t right_threshold;

uint8_t hazard_count;
uint8_t party_count;
uint8_t special_last;
uint16_t special_timer;

// Input
uint8_t input;
#define IN_NONE 0;
#define IN_LEFT_SHORT 1;
#define IN_LEFT_LONG 2;
#define IN_RIGHT_SHORT 3;
#define IN_RIGHT_LONG 4;
#define IN_HAZARD 5;
#define IN_PARTY 6;

// Mode
uint8_t mode;
#define IDLE 0;
#define LEFT 1;
#define RIGHT 2;
#define HAZARD 3;
#define PARTY 4;

// State
uint8_t state;
#define OFF 0;
#define ON 1;

uint8_t count;
uint16_t timer;

void main(void) {
	InitGPIO();

	left_threshold = 0;
	right_threshold = 0;

	hazard_count = 0;
	party_count = 0;
	special_timer = 0;
	special_last = IDLE;

	timer = 0;
	count = 0;
	mode = IDLE;
	state = OFF;

	while(1) {
		// Input filtering
		if((BTNLEFT_PORT->IDR & BTNLEFT_PIN) == 0) {
			left_threshold++;
		}
		else {
			left_threshold = 0;
		}
		if((BTNRIGHT_PORT->IDR & BTNRIGHT_PIN) == 0) {
			right_threshold++;
		}
		else {
			right_threshold = 0;
		}

		if(left_threshold > 0 && right_threshold > 0) {
			left_threshold = 0;
			right_threshold = 0;
		}

		input = IN_NONE;
		if(left_threshold == 500) {
			input = IN_LEFT_SHORT;
		}
		else if(right_threshold == 500) {
			input = IN_RIGHT_SHORT;
		}
		else if(left_threshold >= 10000) {
			input = IN_LEFT_LONG;
			left_threshold = 10000;
		}
		else if(right_threshold >= 10000) {
			input = IN_RIGHT_LONG;
			right_threshold = 10000;
		}

		// Party / Hazard detect
		if(input == 1) {
			if(special_last == 2) {
				hazard_count++;
			}
			if(special_last == 1) {
				party_count++;
			}
			special_timer = 10000;
			special_last = LEFT;
		}
		if(input == 3) {
			if(special_last == 1) {
				hazard_count++;
			}
			special_timer = 10000;
			special_last = RIGHT;
		}

		if(special_timer > 0) {
			special_timer--;
		}
		else {
			special_last = IDLE;
			hazard_count = 0;
			party_count = 0;
		}

		if(hazard_count >= 2) {
			input = IN_HAZARD;
		}

		if(party_count >= 4) {
			input = IN_PARTY;
		}

		// Changing mode
		if(input == 5) {
			if(mode != 3) {
				mode = HAZARD;
				count = 10;
				timer = 0;
				state = OFF;
			}
		}
		else if(input == 6) {
			if(mode != 4) {
				mode = PARTY;
				count = 10;
				timer = 0;
				state = OFF;
			}
		}
		else if(input == 1) {
			if(mode == 2) {
				timer = 0;
				state = OFF;
			}
			mode = LEFT; // 1
			if(count > 3) {
				count = 0;
				state = ON;
			}
			else {
				count = 3;
			}
		}
		else if(input == 2) {
			mode = LEFT; // 1
			count = 100;
		}
		else if(input == 3) {
			if(mode == 1) {
				timer = 0;
				state = OFF;
			}
			mode = RIGHT; // 2
			if(count > 3) {
				count = 0;
				state = ON;
			}
			else {
				count = 3;
			}
		}
		else if(input == 4) {
			mode = RIGHT; // 2
			count = 100;
		}

		// Mode statemachine
		if(mode != 0 && mode != 4) {
			if(timer == 0) {
				if(state == 0) {
					timer = PERIOD;
					state = ON;
					SetOutput(mode);
				}
				else {
					state = OFF;
					SetOutput(0);
					if(count > 0 && mode != 3) count--;
					if(count == 0) {
						mode=IDLE; // 0
					}
					else {
						timer = PERIOD;
					}
				}
			}
			else {
				timer--;
			}
		}
		else if(mode == 4) {
			if(timer == 0) {
				switch(state) {
					case 0:
						SetOutput(1);
						timer = 1000;
						break;
					case 1:
						SetOutput(0);
						timer = 4000;
						break;
					case 2:
						SetOutput(1);
						timer = 1000;
						break;
					case 3:
						SetOutput(0);
						timer = 4000;
						break;
					case 4:
						SetOutput(2);
						timer = 1000;
						break;
					case 5:
						SetOutput(0);
						timer = 4000;
						break;
					case 6:
						SetOutput(2);
						timer = 1000;
						break;
					case 7:
						SetOutput(0);
						timer = 4000;
						break;
				}
				if(state++ == 16) state = 0;
			}
			else {
				timer--;
			}
		}
	}
}

void InitGPIO() {
	GPIO_Init(LEFTOUT_PORT, LEFTOUT_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
	GPIO_Init(RIGHTOUT_PORT, RIGHTOUT_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
	GPIO_Init(BTNLEFT_PORT, BTNLEFT_PIN, GPIO_MODE_IN_FL_NO_IT);
	GPIO_Init(BTNRIGHT_PORT, BTNRIGHT_PIN, GPIO_MODE_IN_FL_NO_IT);
}

void SetOutput(uint8_t pattern) {
	switch(pattern) {
		case 0:
			GPIO_WriteLow(LEFTOUT_PORT, LEFTOUT_PIN);
			GPIO_WriteLow(RIGHTOUT_PORT, RIGHTOUT_PIN);
			break;
		case 1:
			GPIO_WriteHigh(LEFTOUT_PORT, LEFTOUT_PIN);
			GPIO_WriteLow(RIGHTOUT_PORT, RIGHTOUT_PIN);
			break;
		case 2:
			GPIO_WriteLow(LEFTOUT_PORT, LEFTOUT_PIN);
			GPIO_WriteHigh(RIGHTOUT_PORT, RIGHTOUT_PIN);
			break;
		case 3:
			GPIO_WriteHigh(LEFTOUT_PORT, LEFTOUT_PIN);
			GPIO_WriteHigh(RIGHTOUT_PORT, RIGHTOUT_PIN);
			break;
	}
}

void Delay(uint16_t nCount) {
	while (nCount != 0) {
		nCount--;
	}
}
