;******************************************************************************
;  File Name    : PolmoodSwitch2.asm
;  Version      : 1.0
;  Description  : low power 20 second timer bell switch buffer
;  Author       : RWB
;  Target       : Microchip PIC 12F683 Microcontroller
;  Compiler     : Microchip Assembler (MPASM)
;  IDE          : Microchip MPLAB IDE v8.00
;  Programmer   : PICKit2
;  Last Updated : 051223
;  Simple code with deliberate use of blocking delays to ensure events are sequential.  
; ************************************************************************************
#include <p12F683.inc>
     __config (_INTRC_OSC_NOCLKOUT & _WDT_OFF & _PWRTE_ON & _MCLRE_ON & _CP_OFF & _IESO_OFF & _FCMEN_OFF)

 ; PIC run at 31 KHz 130uS instruction cycle 

;-------------------------------------------------------------------------------------------;
;;pinouts; 12F683																			;
									; 1 5Volt												;
#define		ESP_RST			GPIO,5	; 2 OUTPUT active High									;
#define		BELL_SW_IN		GPIO,4	; 3 INPUT active LOW doorbell pulse		 				;
;			MCLR		    GPIO,3	; 4 NC													;
#define		ESP_BUSY		GPIO,2	; 5 INPUT Host controller RST 20 Second or Bell			;
#define		DLY_TIME	   	GPIO,1	; 4 INPUT Select delay time: 20/2 seconds				;
#define		BELL_SW_TRUE	GPIO,0	; 7 OUT, HIGH = DoorBell INPUT							;
					  				; 8 OV													;
																							;
;********* CONSTANT VALUES******************************************************************;
	cblock 0x20

		Delay1                  ; delay loop: Delay1 and Delay2
		Delay2                  ; delay loop: Delay1 and Delay2
		Temp1
		Temp2
		SHADio

	endc

;-------------------------------------------------------------------------------------------;
; RESET VECTOR -Held in reset for 72mS by PWRT
;-------------------------------------------------------------------------------------------;
;***************************************************************************************
		org	0x0000
; MAIN PROGRAM		SETUP 																	*						
;-------------------------------------------------------------------------------------------*
_main_start 	

	 bsf    STATUS,RP0    ; Select Registers at Bank 1-------------------------------------
     movlw  b'0000000'	  ; starts at 31.25KHz ??
     movwf  OSCCON        ; now running at 31KHz, low power instruction
;_osc_check
	; btfss  OSCCON,1		  ; wait for stable oscillator
	; goto  	_osc_check	  ; need a WDT in case this blocks
     movlw	0x00
     movwf  ANSEL         ; Make all ports as digital I/O
 	 movlw	b'00010110'   ; GP5 0 GP4 1 GP3 X GP2 1 GP1 1 GP0 0
	 movwf	TRISIO		  ; set I/O PORT A
	 movlw  b'00000000'	  ; GP4 falling edge Int enabled
	 movwf	OPTION_REG	  ; WPU_EN FE_INT
	 movlw  b'00001111'
	 movwf	WPU			  ; WPU's EN
     bcf    STATUS,RP0    ; Back to Registers at Bank 0----------------------------------

     movlw  b'10100001'  ; 		
     movwf  T1CON		  ;Set  Prescale=4, TMR1=on. 
     movlw  b'00000111'
     movwf  CMCON0        ; Turn off Comparator (GP0, GP1, GP2
	 movlw 	b'00100000'	  ; sets outputs
	 movwf	SHADio
	 movwf	GPIO

 			
		
;-------------------------------------------------------------------------------------------;SEtup
; Timer 1 used to set delay between ESP polls 
; CLOCK 31KHz ins_cycle 129uS Prescale 4x
; 30SECONDS - 1CF2 20S-68A1 10S-B450 5S-DA27 2S-F0DC
;-------------------------------------------------------------------------------------------;SEtup
_main_loop:		
		movlw	h'68'
		movwf	TMR1H
		movlw	h'A4'
		movwf	TMR1L
		bcf		PIR1,0			; clear TMR1 overflow flag 
		bsf		SHADio,5		; Clear ESP8266 reset
		movfw	SHADio
		movwf	GPIO								   	
	
_time_loop:
		btfsc 	GPIO,2			;ESP_BUSY GPIO,5	; 2 INPUT active High
		goto	_main_loop		; reset timer1 ensures 20 second delay from BUSY clear 
		btfss	GPIO,4			;BELL_IN  GPIO,4	; 3 INPUT active LOW doorbell pulse
		goto	_bell_switch
		btfss	PIR1,0			; Poll TMR1 overflow flag

 		goto	_time_loop		; just loop...
; reset timer 1 and then service outputs 
		movlw	h'68'
		movwf	TMR1H
		movlw	h'A4'
		movwf	TMR1L
		bcf		PIR1,0			; clear TMR1 overflow flag - 20 Seconds from here...
		bcf		SHADio,5		;GPIO,5	; 5 OUT Host controller RST 20 Second or Bell
		movfw	SHADio
		movwf	GPIO			; Reset ESP8266
		call 	Dly_10mS
		bsf		SHADio,5
		movfw	SHADio
		movwf	GPIO     		; Clear reset
		call	Dly_500mS		; ensure time to service routines 
		goto	_time_loop


_bell_switch:
		bsf		SHADio,0		;DOORBELL_TRUE	GPIO,0	; 7 OUT, HIGH = DoorBell INPUT
		movfw	SHADio
		movwf	GPIO
		call 	Dly_10mS
		bcf		SHADio,5		;ESP_RST GPIO,5	 OUT Host controller RST 
		movfw	SHADio
		movwf	GPIO
		call 	Dly_10mS
		bsf		SHADio,5			; Clear reset
		movfw	SHADio
		movwf	GPIO
; wireless doorbell - 5 pulses period 650mS *1500 mS* - Debounce 1800mS
;// Blocking delay allows ESP8266 to execute its code
		call 	Dly_1800
		bcf    	SHADio,0
		movfw	SHADio
		movwf	GPIO
		
		goto	_time_loop

;-----------------------------------------------------------------------------
;Delays

Dly_1800: ;1 second @125 KHz
     movlw  0x24
     movwf  Delay2
Loop1:
     movlw  0x80
     movwf  Delay1
Loop2:
     decfsz Delay1,f      ; Decrease Delay1, If zero skip the next instruction
     goto   Loop2    ; Not zero goto DelayLoop2
     decfsz Delay2,f      ; Decrease Delay2, If zero skip the next instruction
     goto   Loop1    ; Not zero goto DelayLoop1
     return               ; Return to the Caller
;----------------------------------------------------------------------------- 
Dly_500mS:
     movlw  0x03
     movwf  Delay2
Loop3:
     movlw  0xFF
     movwf  Delay1
Loop4:
     decfsz Delay1,f    ; Decrease Delay1, If zero skip the next instruction
     goto   Loop4    	; Not zero goto DelayLoop2
     decfsz Delay2,f    ; Decrease Delay2, If zero skip the next instruction
     goto   Loop3    	; Not zero goto DelayLoop1
     return             ; Return to the Caller
;-----------------------------------------------------------------------------     
Dly_10mS:
	movlw	0x18
	movwf	Delay1
Loop5:
	decfsz  Delay1,f
	goto	Loop5
    return               ; Return to the Caller

    
    end
