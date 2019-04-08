/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"

#define BUT_PIO           PIOA
#define BUT_PIO_ID        ID_PIOA
#define BUT_PIO_IDX       4u
#define BUT_PIO_IDX_MASK  (1u << BUT_PIO_IDX)

#define HOUR        15
#define MINUTE      30
#define SECOND      50

volatile uint8_t flag_time = 1;

void RTC_init(void);


struct ili9488_opt_t g_ili9488_display_opt;

void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}

/**
*  Handle Interrupcao botao 1
*/
static void Button1_Handler(uint32_t id, uint32_t mask)
{

}

void button_init(void){
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_set_input(BUT_PIO, BUT_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);

	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrupção */
	pio_enable_interrupt(BUT_PIO, BUT_PIO_IDX_MASK);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIO_IDX_MASK, PIO_IT_FALL_EDGE, Button1_Handler);

	/* habilita interrupçcão do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);
}


void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}	
}

void RTC_Handler(void)
{

	uint32_t minute;
	uint32_t second;
	uint32_t hour;
	
	rtc_get_time(RTC, &hour, &minute, &second);
	uint32_t ul_status = rtc_get_status(RTC);

	/*
	*  Verifica por qual motivo entrou
	*  na interrupcao, se foi por segundo
	*  ou Alarm
	*/
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
			if(second == 60){
				second = 0;
				minute += 1;
			}
			rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
			
			rtc_set_time_alarm(RTC, 1, hour, 1, minute, 1, second+1);
	}
	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
}

void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC,  RTC_IER_ALREN);

}


int main(void) {
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	uint32_t minute;
	uint32_t second;
	uint32_t hour;
	
	button_init();
	RTC_init();
	board_init();
	sysclk_init();	
	configure_lcd();
	char bufferHour[32];
	char bufferMinute[32];	
	char bufferSecond[32];
	
	font_draw_text(&sourcecodepro_28, "OIMUNDO", 50, 50, 1);
	font_draw_text(&calibri_36, "Oi Mundo! #$!@", 50, 100, 1);
	
	while(1) {
		
		rtc_get_time(RTC, &hour, &minute, &second);
		
		sprintf(bufferHour, "%d", hour);
		sprintf(bufferMinute, "%d", minute);
		sprintf(bufferSecond, "%d", second);
		
		font_draw_text(&calibri_36, bufferHour, 40, 200, 1);
		font_draw_text(&calibri_36, ":", 80, 200, 1);
		font_draw_text(&calibri_36, bufferMinute, 120, 200, 1);
		font_draw_text(&calibri_36, ":", 160, 200, 1);
		font_draw_text(&calibri_36, bufferSecond, 200, 200, 1);
		
		if(!(pio_get(BUT_PIO, PIO_INPUT, BUT_PIO_IDX_MASK)))
		{
			delay_ms(100);
			
		}
		
	}
}