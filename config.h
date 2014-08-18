#define F_CPU 16000000UL

#define LCD_DATA_PORT	PORTB
#define LCD_DATA_DDR	DDRB

#define LCD_AUX_PORT	PORTE
#define LCD_AUX_DDR	DDRE
#define LCD_RS		PE5
#define LCD_RW		PE6
#define LCD_ENA		PE7

#define LCD_LINES	4
#define LCD_CHARS	20

#define OUTPUT_DDR	DDRE
#define OUTPUT_PORT	PORTE
#define	OUTPUT_PIN	PE3

// this is in clock cycles (with clk/8 prescaler)
// that effectively means 0.5us with 16MHz osc
#define ONTIME_MIN	20
#define ONTIME_MAX	1000

// we want max rep. at 1kHz
// that means 1ms
// and that means at 16MHz CPU / 256
// (in clock oscillator cycles
//
#define PERIOD_MIN	63
#define PERIOD_MAX	65535

#define IDLE_PERIOD	1000
#define IDLE_PERIOD_8	255

#define ROTARYDDR       DDRD
#define ROTARYPORT      PORTD
#define ROTARYPIN       PIND
#define ROTARYA         PD4
#define ROTARYB         PD5

#define LCD_REFRESH     5000


#define BTNDDR  DDRD
#define BTNPORT PORTD
#define BTNPIN  PIND

#define LED_SHOOT       0x01

#define BTN_CANCEL      0x02
#define BTN_CONFIRM     0x40
#define BTN_SHOOT       0x80

#define NULL 0
